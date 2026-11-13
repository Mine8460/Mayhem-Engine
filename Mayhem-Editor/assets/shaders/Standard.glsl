// Shader standard du moteur : PBR metallic / roughness (Cook-Torrance GGX)
// Textures : Albedo, Normal, Metallic, Roughness, Occlusion, Height (parallax), Emission
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Tangent;  // xyz = tangente, w = signe de la bitangente

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform vec4 u_UVRect;   // (minU, minV, maxU, maxV) -> sous-texture / spritesheet
uniform vec2 u_Tiling;
uniform vec2 u_Offset;

out vec3 v_WorldPosition;
out vec3 v_Normal;
out vec4 v_Tangent;
out vec2 v_TexCoord;

void main()
{
	vec4 worldPos = u_Transform * vec4(a_Position, 1.0);

	v_WorldPosition = worldPos.xyz;
	v_Normal = normalize(mat3(transpose(inverse(u_Transform))) * a_Normal);
	// La tangente suit la surface : on la transforme comme une position (sans translation)
	v_Tangent = vec4(mat3(u_Transform) * a_Tangent.xyz, a_Tangent.w);
	v_TexCoord = mix(u_UVRect.xy, u_UVRect.zw, a_TexCoord) * u_Tiling + u_Offset;

	gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 450 core

#define CASCADE_COUNT 4
#define MAX_LIGHTS 32   // doit correspondre à SceneRenderer::MaxLights
#define LOCAL_SHADOW_LAYERS 16  // doit correspondre à SceneRenderer::LocalShadowLayers
const float LOCAL_SHADOW_NEAR = 0.1; // doit correspondre à s_LocalShadowNear (SceneRenderer.cpp)
const float PI = 3.14159265359;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec3 v_WorldPosition;
in vec3 v_Normal;
in vec4 v_Tangent;
in vec2 v_TexCoord;

// ---------- Matériau ----------
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_OcclusionMap;
uniform sampler2D u_HeightMap;
uniform sampler2D u_EmissionMap;

uniform vec4 u_Color;              // albedo * teinte de l'instance
uniform float u_AlphaCutoff;
uniform int u_Lit;                 // 0 = couleur brute, 1 = éclairé

uniform float u_Metallic;          // multiplié par la MetallicMap
uniform float u_Roughness;         // multiplié par la RoughnessMap
uniform int u_RoughnessIsSmoothness;
uniform vec4 u_MetallicChannel;    // masque du canal lu (1 sur R, G, B ou A)
uniform vec4 u_RoughnessChannel;
uniform vec4 u_OcclusionChannel;
uniform vec4 u_HeightChannel;
uniform float u_OcclusionStrength;

uniform int u_HasNormalMap;
uniform float u_NormalStrength;
uniform int u_NormalFlipY;         // normal map "DirectX" : vert inversé

uniform int u_HasHeightMap;
uniform float u_HeightScale;

uniform vec3 u_EmissionColor;      // couleur * intensité (linéaire)

// ---------- Scène ----------
uniform mat4 u_View;
uniform vec3 u_CameraPosition;
uniform vec3 u_LightDirection;     // direction dans laquelle la lumière voyage
uniform vec3 u_LightColor;         // couleur * intensité

// Lumières locales (point lights et spot lights)
uniform int u_LightCount;
uniform vec4 u_LightsPositionRange[MAX_LIGHTS]; // xyz = position, w = portée
uniform vec4 u_LightsColorType[MAX_LIGHTS];     // rgb = couleur * intensité, w = 0 point / 1 spot
uniform vec4 u_LightsDirection[MAX_LIGHTS];     // xyz = sens du cône (spot), w = taille d'un texel d'ombre par mètre de distance
uniform vec4 u_LightsSpot[MAX_LIGHTS];          // x = cos(demi-angle externe), y = cos(demi-angle interne), z = première couche d'ombre (-1 = aucune), w = biais (m)

// Ombres des lumières locales : une couche par spot, six par point light (les faces d'un cube)
uniform sampler2DArrayShadow u_LocalShadowMap;
uniform mat4 u_LocalShadowMatrices[LOCAL_SHADOW_LAYERS];

// Éclairage par l'environnement (IBL) : voir EnvironmentMap
uniform samplerCube u_IrradianceMap;  // lumière diffuse reçue dans chaque direction
uniform samplerCube u_PrefilterMap;   // reflets, une mip par rugosité
uniform float u_PrefilterMaxLod;

// Ombres (cascades)
uniform sampler2DArrayShadow u_ShadowMap;
uniform mat4 u_LightSpaceMatrices[CASCADE_COUNT];
uniform vec4 u_CascadeSplits;      // distance (vue) de fin de chaque cascade
uniform vec4 u_CascadeTexelSizes;  // taille d'un texel d'ombre en mètres, par cascade
uniform int u_ShadowsEnabled;
uniform float u_ShadowBias;
uniform int u_ShowCascades;        // debug : colore chaque cascade

uniform int u_EntityID;

// Sortie couleur
uniform float u_Exposure;
uniform int u_ToneMapping;

// ---------- Espace couleur ----------
// Les textures de couleur (albedo, émission) et les couleurs de l'éditeur sont en sRGB.
// Les autres maps (normal, metallic, roughness, AO, height) sont des données : déjà linéaires.
vec3 SRGBToLinear(vec3 _c) { return pow(_c, vec3(2.2)); }
vec3 LinearToSRGB(vec3 _c) { return pow(_c, vec3(1.0 / 2.2)); }

// Tone mapping ACES (approximation de Narkowicz) : compresse les fortes intensités au lieu de les écrêter
vec3 ToneMapACES(vec3 _x)
{
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((_x * (a * _x + b)) / (_x * (c * _x + d) + e), 0.0, 1.0);
}

// =====================================================================
//  Ombres
// =====================================================================

float GetViewDepth()
{
	return -(u_View * vec4(v_WorldPosition, 1.0)).z;
}

// -1 = au-delà de la distance d'ombre
int GetCascadeIndex(float _depth)
{
	for (int i = 0; i < CASCADE_COUNT; i++)
	{
		if (_depth < u_CascadeSplits[i])
			return i;
	}
	return -1;
}

// 1 = éclairé, 0 = dans l'ombre
float SampleCascade(int _cascade, vec3 _N, vec3 _L)
{
	// Décale le point le long de la normale (proportionnel à la taille d'un texel) : évite l'acné
	vec3 worldPos = v_WorldPosition + _N * u_CascadeTexelSizes[_cascade] * 1.5;

	vec4 lightSpace = u_LightSpaceMatrices[_cascade] * vec4(worldPos, 1.0);
	vec3 p = lightSpace.xyz / lightSpace.w * 0.5 + 0.5;

	if (p.z > 1.0)
		return 1.0;

	// Biais plus fort sur les surfaces rasantes
	float bias = max(u_ShadowBias * (1.0 - dot(_N, _L)), u_ShadowBias * 0.1);

	// PCF 3x3 (+ filtrage matériel) : bords d'ombre doux
	vec2 texel = 1.0 / vec2(textureSize(u_ShadowMap, 0).xy);
	float lit = 0.0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
			lit += texture(u_ShadowMap, vec4(p.xy + vec2(x, y) * texel, float(_cascade), p.z - bias));
	}
	return lit / 9.0;
}

float ComputeShadow(vec3 _N, vec3 _L)
{
	if (u_ShadowsEnabled == 0)
		return 1.0;

	float depth = GetViewDepth();
	int cascade = GetCascadeIndex(depth);
	if (cascade < 0)
		return 1.0;

	float shadow = SampleCascade(cascade, _N, _L);

	// Fondu vers la cascade suivante sur les derniers 10 % : masque la jonction entre cascades.
	// Pour la dernière cascade, on fond vers "éclairé" : pas de coupure nette au bout de la distance d'ombre.
	float start = cascade == 0 ? 0.0 : u_CascadeSplits[cascade - 1];
	float end = u_CascadeSplits[cascade];
	float band = (end - start) * 0.1;
	float blend = smoothstep(end - band, end, depth);

	if (blend > 0.0)
	{
		float next = cascade < CASCADE_COUNT - 1 ? SampleCascade(cascade + 1, _N, _L) : 1.0;
		shadow = mix(shadow, next, blend);
	}

	return shadow;
}

// =====================================================================
//  Ombres des point lights et spot lights
// =====================================================================

// 1 = éclairé, 0 = dans l'ombre. _fromLight : de la lumière vers le fragment.
float SampleLocalShadow(int _i, vec3 _fromLight, float _dist, vec3 _N)
{
	float baseLayer = u_LightsSpot[_i].z;
	if (baseLayer < 0.0)
		return 1.0; // cette lumière ne projette pas d'ombre

	int layer = int(baseLayer + 0.5);
	if (u_LightsColorType[_i].w < 0.5)
	{
		// Point light : la face du cube est celle de l'axe dominant de la direction
		vec3 a = abs(_fromLight);
		int face;
		if (a.x >= a.y && a.x >= a.z)
			face = _fromLight.x > 0.0 ? 0 : 1;
		else if (a.y >= a.z)
			face = _fromLight.y > 0.0 ? 2 : 3;
		else
			face = _fromLight.z > 0.0 ? 4 : 5;
		layer += face;
	}

	// Décale le point le long de la normale (proportionnel à la taille d'un texel à cette distance) : évite l'acné
	vec3 worldPos = v_WorldPosition + _N * (_dist * u_LightsDirection[_i].w) * 1.5;

	vec4 lightSpace = u_LocalShadowMatrices[layer] * vec4(worldPos, 1.0);
	if (lightSpace.w <= LOCAL_SHADOW_NEAR)
		return 1.0;
	vec3 p = lightSpace.xyz / lightSpace.w * 0.5 + 0.5;
	if (p.z > 1.0)
		return 1.0;

	// Biais donné en mètres : la profondeur d'une projection en perspective n'est pas linéaire,
	// on le convertit en écart de profondeur à cette distance (dérivée de la projection)
	float farPlane = u_LightsPositionRange[_i].w;
	float bias = u_LightsSpot[_i].w * LOCAL_SHADOW_NEAR * farPlane
		/ ((farPlane - LOCAL_SHADOW_NEAR) * lightSpace.w * lightSpace.w);

	// PCF 3x3 : bords d'ombre doux
	vec2 texel = 1.0 / vec2(textureSize(u_LocalShadowMap, 0).xy);
	float lit = 0.0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
			lit += texture(u_LocalShadowMap, vec4(p.xy + vec2(x, y) * texel, float(layer), p.z - bias));
	}
	return lit / 9.0;
}

// =====================================================================
//  Parallax occlusion mapping (HeightMap)
// =====================================================================

// Profondeur sous la surface : 0 = sommet (blanc), 1 = creux (noir)
float SampleDepth(vec2 _uv, vec2 _dx, vec2 _dy)
{
	return 1.0 - dot(textureGrad(u_HeightMap, _uv, _dx, _dy), u_HeightChannel);
}

// Marche le long du rayon de vue en espace tangent jusqu'à toucher le relief
vec2 ParallaxOcclusion(vec2 _uv, vec3 _viewTS)
{
	// Dérivées calculées avant la boucle : l'échantillonnage dans une boucle à nombre
	// de tours variable ne peut pas calculer le bon niveau de mipmap tout seul
	vec2 dx = dFdx(_uv);
	vec2 dy = dFdy(_uv);

	// Plus de couches en vue rasante (là où le décalage est le plus grand)
	float layerCount = mix(32.0, 8.0, clamp(_viewTS.z, 0.0, 1.0));
	float layerDepth = 1.0 / layerCount;

	vec2 shift = _viewTS.xy / max(_viewTS.z, 0.1) * u_HeightScale;
	vec2 deltaUV = shift / layerCount;

	vec2 uv = _uv;
	float currentLayer = 0.0;
	float depth = SampleDepth(uv, dx, dy);

	for (int i = 0; i < 32 && currentLayer < depth; i++)
	{
		uv -= deltaUV;
		depth = SampleDepth(uv, dx, dy);
		currentLayer += layerDepth;
	}

	// Interpolation entre la dernière couche au-dessus et la première en-dessous du relief
	vec2 previousUV = uv + deltaUV;
	float after = depth - currentLayer;
	float before = SampleDepth(previousUV, dx, dy) - currentLayer + layerDepth;
	float weight = after / (after - before + 1e-5);
	return mix(uv, previousUV, clamp(weight, 0.0, 1.0));
}

// =====================================================================
//  BRDF (Cook-Torrance : distribution GGX, visibilité Smith-GGX corrélée, Fresnel de Schlick)
// =====================================================================

float D_GGX(float _NoH, float _a)
{
	float a2 = _a * _a;
	float d = _NoH * _NoH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

// Inclut déjà le 1 / (4 NoL NoV) de Cook-Torrance
float V_SmithGGXCorrelated(float _NoV, float _NoL, float _a)
{
	float a2 = _a * _a;
	float ggxV = _NoL * sqrt(_NoV * _NoV * (1.0 - a2) + a2);
	float ggxL = _NoV * sqrt(_NoL * _NoL * (1.0 - a2) + a2);
	return 0.5 / max(ggxV + ggxL, 1e-5);
}

vec3 F_Schlick(float _VoH, vec3 _F0)
{
	return _F0 + (1.0 - _F0) * pow(1.0 - _VoH, 5.0);
}

// Contribution (diffus + spéculaire) d'une lumière arrivant de la direction _L avec la radiance _radiance.
// Convention du moteur : une lumière d'intensité 1 éclaire un albedo blanc à 100 % (pas de division par PI
// sur le diffus), donc le spéculaire est multiplié par PI pour garder le même rapport diffus / spéculaire.
vec3 EvaluateLight(vec3 _L, vec3 _radiance, vec3 _N, vec3 _V, float _NoV, vec3 _F0, vec3 _diffuseColor, float _a)
{
	float NoL = max(dot(_N, _L), 0.0);
	if (NoL <= 0.0)
		return vec3(0.0);

	vec3 H = normalize(_L + _V);
	float NoH = max(dot(_N, H), 0.0);
	float VoH = max(dot(_V, H), 0.0);

	vec3 F = F_Schlick(VoH, _F0);
	vec3 specular = D_GGX(NoH, _a) * V_SmithGGXCorrelated(_NoV, NoL, _a) * F;
	vec3 kDirect = (1.0 - F) * _diffuseColor;

	return (kDirect + specular * PI) * _radiance * NoL;
}

// Fresnel pour la lumière ambiante : une surface rugueuse reflète moins aux angles rasants
vec3 F_SchlickRoughness(float _NoV, vec3 _F0, float _roughness)
{
	return _F0 + (max(vec3(1.0 - _roughness), _F0) - _F0) * pow(1.0 - _NoV, 5.0);
}

// Réflexion de l'environnement pré-intégrée, version analytique (Karis, "Mobile PBR")
// Remplace une LUT de BRDF tant qu'il n'y a pas d'IBL
vec3 EnvBRDFApprox(vec3 _F0, float _roughness, float _NoV)
{
	const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = _roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * _NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return _F0 * AB.x + AB.y;
}

void main()
{
	// ---------- Repère de la surface ----------
	vec3 N = normalize(v_Normal);
	if (!gl_FrontFacing)
		N = -N; // face arrière d'un matériau two-sided : éclairée comme une face avant

	vec3 V = normalize(u_CameraPosition - v_WorldPosition);

	// Repère tangent (TBN). Pas de tangente (mesh sans UV) = pas de normal map ni de parallax.
	vec3 T = v_Tangent.xyz - N * dot(N, v_Tangent.xyz); // Gram-Schmidt : interpolée, elle n'est plus perpendiculaire
	bool hasTangent = dot(T, T) > 1e-8;
	mat3 TBN = mat3(1.0);
	if (hasTangent)
	{
		T = normalize(T);
		vec3 B = cross(N, T) * (v_Tangent.w < 0.0 ? -1.0 : 1.0);
		TBN = mat3(T, B, N);
	}

	// ---------- UV (parallax) ----------
	vec2 uv = v_TexCoord;
	if (u_HasHeightMap == 1 && hasTangent)
	{
		vec3 viewTS = normalize(transpose(TBN) * V);
		uv = ParallaxOcclusion(uv, viewTS);
	}

	// ---------- Albedo ----------
	vec4 albedo = texture(u_AlbedoMap, uv) * u_Color;

	if (albedo.a < u_AlphaCutoff)
		discard;

	albedo.rgb = SRGBToLinear(albedo.rgb);
	vec3 color = albedo.rgb;

	if (u_Lit == 1)
	{
		// ---------- Normale ----------
		vec3 geometryN = N; // la normale du mesh sert aux ombres (la normal map ne déplace pas la géométrie)
		if (u_HasNormalMap == 1 && hasTangent)
		{
			vec3 n = texture(u_NormalMap, uv).xyz * 2.0 - 1.0;
			if (u_NormalFlipY == 1)
				n.y = -n.y;
			n.xy *= u_NormalStrength;
			N = normalize(TBN * normalize(n));
		}

		// ---------- Paramètres PBR ----------
		float metallic = clamp(u_Metallic * dot(texture(u_MetallicMap, uv), u_MetallicChannel), 0.0, 1.0);

		float roughnessSample = dot(texture(u_RoughnessMap, uv), u_RoughnessChannel);
		if (u_RoughnessIsSmoothness == 1)
			roughnessSample = 1.0 - roughnessSample;
		// Rugosité minimale : en dessous, le reflet du soleil devient un point qui scintille
		float roughness = clamp(u_Roughness * roughnessSample, 0.045, 1.0);
		float a = roughness * roughness; // rugosité "perceptuelle" -> alpha GGX

		float occlusion = mix(1.0, dot(texture(u_OcclusionMap, uv), u_OcclusionChannel), u_OcclusionStrength);

		// Réflectance à incidence normale : 4 % pour les diélectriques, la couleur pour les métaux
		vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
		vec3 diffuseColor = albedo.rgb * (1.0 - metallic);

		float NoV = max(dot(N, V), 1e-4);

		// ---------- Lumière directionnelle ----------
		vec3 L = -u_LightDirection;

		// Ombre testée avec la normale du mesh. Si la normal map tourne la surface vers la lumière
		// alors que le mesh lui tourne le dos, la face reste dans l'ombre.
		float geometryNoL = dot(geometryN, L);
		float shadow = (max(dot(N, L), 0.0) > 0.0 && geometryNoL > 0.0) ? ComputeShadow(geometryN, L) : 0.0;

		vec3 direct = EvaluateLight(L, u_LightColor, N, V, NoV, F0, diffuseColor, a) * shadow;

		// ---------- Point lights et spot lights ----------
		for (int i = 0; i < u_LightCount; i++)
		{
			vec3 toLight = u_LightsPositionRange[i].xyz - v_WorldPosition;
			float range = u_LightsPositionRange[i].w;
			float distanceSquared = dot(toLight, toLight);
			if (distanceSquared >= range * range)
				continue;

			float dist = sqrt(distanceSquared);
			vec3 Ll = toLight / max(dist, 1e-4);

			// Pas de lumière à travers le dos de la surface (la normal map ne doit pas la laisser passer)
			if (dot(geometryN, Ll) <= 0.0)
				continue;

			// Loi en inverse du carré, fondue vers 0 à la portée (fenêtre de Karis) : pas de coupure nette
			float attenuation = 1.0 / max(distanceSquared, 0.01);
			float window = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
			attenuation *= window * window;

			// Cône du spot : plein jusqu'à l'angle interne, fondu jusqu'à l'angle externe
			if (u_LightsColorType[i].w > 0.5)
			{
				float cosAngle = dot(-Ll, u_LightsDirection[i].xyz);
				float cosOuter = u_LightsSpot[i].x;
				float cosInner = u_LightsSpot[i].y;
				float cone = clamp((cosAngle - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
				attenuation *= cone * cone * (3.0 - 2.0 * cone);
			}

			if (attenuation <= 0.0)
				continue;

			// Ombre (seulement pour les lumières à qui le renderer en a accordé une)
			float localShadow = SampleLocalShadow(i, -toLight, dist, geometryN);
			if (localShadow <= 0.0)
				continue;
			attenuation *= localShadow;

			direct += EvaluateLight(Ll, u_LightsColorType[i].rgb * attenuation, N, V, NoV, F0, diffuseColor, a);
		}

		// ---------- Ambiante (IBL) ----------
		// Diffus : l'irradiance de l'environnement dans la direction de la normale.
		// Reflet : l'environnement pré-filtré à la rugosité de la surface, dans la direction du reflet.
		vec3 kS = F_SchlickRoughness(NoV, F0, roughness);
		vec3 kD = (1.0 - kS) * (1.0 - metallic);
		vec3 irradiance = texture(u_IrradianceMap, N).rgb;

		vec3 R = reflect(-V, N);
		vec3 prefiltered = textureLod(u_PrefilterMap, R, roughness * u_PrefilterMaxLod).rgb;

		vec3 ambientDiffuse = irradiance * albedo.rgb * kD;
		vec3 ambientSpecular = prefiltered * EnvBRDFApprox(F0, roughness, NoV);
		vec3 ambient = (ambientDiffuse + ambientSpecular) * occlusion;

		color = direct + ambient;

		if (u_ShowCascades == 1)
		{
			const vec3 debugColors[CASCADE_COUNT] = vec3[](
				vec3(1.0, 0.35, 0.35),  // 0 : rouge
				vec3(0.35, 1.0, 0.35),  // 1 : vert
				vec3(0.35, 0.35, 1.0),  // 2 : bleu
				vec3(1.0, 1.0, 0.35));  // 3 : jaune

			int cascade = GetCascadeIndex(GetViewDepth());
			if (cascade >= 0)
				color *= debugColors[cascade];
		}
	}

	// ---------- Emission ----------
	color += u_EmissionColor * SRGBToLinear(texture(u_EmissionMap, uv).rgb);

	// Exposition + tone mapping uniquement sur les objets éclairés :
	// un sprite non éclairé garde exactement les couleurs de sa texture
	if (u_Lit == 1)
	{
		color *= u_Exposure;
		if (u_ToneMapping == 1)
			color = ToneMapACES(color);
	}

	o_Color = vec4(LinearToSRGB(max(color, vec3(0.0))), albedo.a);
	o_EntityID = u_EntityID;
}
