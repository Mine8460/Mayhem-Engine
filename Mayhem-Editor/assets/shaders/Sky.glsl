// Ciel procédural + soleil (suit la direction de la DirectionalLight)
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // quad -0.5..0.5

out vec2 v_NDC;

void main()
{
	v_NDC = a_Position.xy * 2.0; // plein écran
	// Profondeur quasi maximale : le ciel n'apparaît que là où rien n'a été dessiné
	gl_Position = vec4(v_NDC, 0.999999, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec2 v_NDC;

uniform mat4 u_InverseViewProjection; // vue SANS translation : le ciel est infiniment loin

uniform vec3 u_SunDirection;  // vers le soleil (= -direction de la lumière)
uniform vec3 u_SunColor;
uniform float u_SunSize;      // rayon angulaire du disque, en radians
uniform float u_SunGlow;      // intensité du halo

uniform vec3 u_ZenithColor;
uniform vec3 u_HorizonColor;
uniform vec3 u_GroundColor;
uniform float u_Exposure;
uniform int u_ToneMapping;

// Les couleurs reçues sont déjà en linéaire (converties côté CPU)
vec3 SRGBToLinear(vec3 _c) { return pow(_c, vec3(2.2)); }
vec3 LinearToSRGB(vec3 _c) { return pow(_c, vec3(1.0 / 2.2)); }

vec3 ToneMapACES(vec3 _x)
{
	const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return clamp((_x * (a * _x + b)) / (_x * (c * _x + d) + e), 0.0, 1.0);
}

void main()
{
	vec4 p = u_InverseViewProjection * vec4(v_NDC, 1.0, 1.0);
	vec3 dir = normalize(p.xyz / p.w);
	vec3 sunDir = normalize(u_SunDirection);

	float sunHeight = sunDir.y;
	float cosToSun = dot(dir, sunDir);

	// 1 = plein jour, 0 = nuit
	float day = smoothstep(-0.15, 0.25, sunHeight);
	// 1 quand le soleil est proche de l'horizon (lever / coucher)
	float sunset = 1.0 - smoothstep(0.0, 0.4, abs(sunHeight));

	// ---------- Ciel ----------
	float up = clamp(dir.y, 0.0, 1.0);
	vec3 sky = mix(u_HorizonColor, u_ZenithColor, pow(up, 0.5));

	// Horizon orangé du côté du soleil quand il est bas
	vec3 sunsetColor = SRGBToLinear(vec3(1.0, 0.45, 0.15));
	float horizonBand = pow(1.0 - up, 4.0);
	float sunSide = 0.35 + 0.65 * max(cosToSun, 0.0);
	sky = mix(sky, sunsetColor, sunset * horizonBand * sunSide);

	// Nuit : on assombrit tout en gardant un bleu très sombre
	sky *= mix(0.03, 1.0, day);

	// ---------- Sol (sous l'horizon) ----------
	vec3 ground = u_GroundColor * mix(0.03, 1.0, day);
	vec3 color = mix(sky, ground, smoothstep(0.0, -0.02, dir.y));

	// ---------- Soleil ----------
	float angle = acos(clamp(cosToSun, -1.0, 1.0));
	float disc = 1.0 - smoothstep(u_SunSize * 0.8, u_SunSize, angle);
	float glow = pow(max(cosToSun, 0.0), 64.0) * u_SunGlow
	           + pow(max(cosToSun, 0.0), 8.0) * u_SunGlow * 0.25;

	float aboveHorizon = smoothstep(-0.02, 0.02, dir.y); // le sol cache le soleil
	color += u_SunColor * (disc * 10.0 + glow) * aboveHorizon;

	color *= u_Exposure;
	if (u_ToneMapping == 1)
		color = ToneMapACES(color);

	o_Color = vec4(LinearToSRGB(max(color, vec3(0.0))), 1.0);
	o_EntityID = -1; // le ciel n'est pas sélectionnable
}
