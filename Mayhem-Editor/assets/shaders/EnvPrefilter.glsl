// Pré-filtrage de l'environnement pour les reflets : une mip par niveau de rugosité (0 = miroir, dernière mip = très mat).
// Échantillonnage préférentiel du lobe GGX (importance sampling).
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_InverseView;

out vec3 v_Direction;

void main()
{
	vec2 ndc = a_Position.xy * 2.0;
	v_Direction = mat3(u_InverseView) * vec3(ndc, -1.0);
	gl_Position = vec4(ndc, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec3 v_Direction;

uniform samplerCube u_EnvironmentMap;
uniform float u_Roughness;        // rugosité perceptuelle de cette mip
uniform float u_SourceResolution; // taille d'une face de l'environnement source

const float PI = 3.14159265359;
const uint SAMPLE_COUNT = 256u;

float DistributionGGX(float _NoH, float _a)
{
	float a2 = _a * _a;
	float d = _NoH * _NoH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

vec2 Hammersley(uint _i, uint _n)
{
	return vec2(float(_i) / float(_n), float(bitfieldReverse(_i)) * 2.3283064365386963e-10);
}

vec3 ImportanceSampleGGX(vec2 _xi, vec3 _N, float _a)
{
	float phi = 2.0 * PI * _xi.x;
	float cosTheta = sqrt((1.0 - _xi.y) / (1.0 + (_a * _a - 1.0) * _xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	vec3 up = abs(_N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, _N));
	vec3 bitangent = cross(_N, tangent);
	return normalize(tangent * H.x + bitangent * H.y + _N * H.z);
}

void main()
{
	vec3 N = normalize(v_Direction);
	vec3 V = N; // hypothèse classique : la vue est alignée sur la normale
	float a = u_Roughness * u_Roughness;

	vec3 color = vec3(0.0);
	float weight = 0.0;

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		vec2 xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H = ImportanceSampleGGX(xi, N, a);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NoL = dot(N, L);
		if (NoL > 0.0)
		{
			// Mip de la source choisie selon la densité de l'échantillon : moins de bruit sur les mips floues
			float NoH = max(dot(N, H), 0.0);
			float HoV = max(dot(H, V), 0.0);
			float pdf = DistributionGGX(NoH, a) * NoH / (4.0 * HoV) + 0.0001;
			float texelSolidAngle = 4.0 * PI / (6.0 * u_SourceResolution * u_SourceResolution);
			float sampleSolidAngle = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
			float mip = u_Roughness == 0.0 ? 0.0 : 0.5 * log2(sampleSolidAngle / texelSolidAngle);

			color += textureLod(u_EnvironmentMap, L, mip).rgb * NoL;
			weight += NoL;
		}
	}

	o_Color = vec4(color / max(weight, 1e-4), 1.0);
}
