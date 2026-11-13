// Capture du ciel dans une cubemap (une face par draw) : radiance linéaire, SANS le disque du soleil.
// Le soleil éclaire déjà la scène par la lumière directionnelle (le compter ici le doublerait).
// Même dégradé que Sky.glsl : garder les deux synchronisés.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // quad -0.5..0.5

uniform mat4 u_InverseView; // orientation de la face de la cubemap

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

uniform vec3 u_SunDirection;  // vers le soleil
uniform vec3 u_ZenithColor;   // déjà en linéaire
uniform vec3 u_HorizonColor;
uniform vec3 u_GroundColor;
uniform float u_Intensity;

vec3 SRGBToLinear(vec3 _c) { return pow(_c, vec3(2.2)); }

void main()
{
	vec3 dir = normalize(v_Direction);
	vec3 sunDir = normalize(u_SunDirection);

	float sunHeight = sunDir.y;
	float cosToSun = dot(dir, sunDir);

	float day = smoothstep(-0.15, 0.25, sunHeight);
	float sunset = 1.0 - smoothstep(0.0, 0.4, abs(sunHeight));

	float up = clamp(dir.y, 0.0, 1.0);
	vec3 sky = mix(u_HorizonColor, u_ZenithColor, pow(up, 0.5));

	vec3 sunsetColor = SRGBToLinear(vec3(1.0, 0.45, 0.15));
	float horizonBand = pow(1.0 - up, 4.0);
	float sunSide = 0.35 + 0.65 * max(cosToSun, 0.0);
	sky = mix(sky, sunsetColor, sunset * horizonBand * sunSide);

	sky *= mix(0.03, 1.0, day);

	vec3 ground = u_GroundColor * mix(0.03, 1.0, day);
	vec3 color = mix(sky, ground, 1.0 - smoothstep(-0.02, 0.0, dir.y));

	o_Color = vec4(color * u_Intensity, 1.0);
}
