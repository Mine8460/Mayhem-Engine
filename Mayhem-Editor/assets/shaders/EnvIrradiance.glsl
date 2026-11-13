// Convolution de l'environnement par un lobe diffus (cosinus) : l'éclairage ambiant reçu dans chaque direction.
// Convention du moteur : pour un environnement uniforme de radiance L, le résultat vaut L
// (donc diffus = albedo * irradiance, comme l'ancienne couleur ambiante).
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

const float PI = 3.14159265359;

void main()
{
	vec3 N = normalize(v_Direction);
	vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);

	const float delta = 0.05;
	vec3 irradiance = vec3(0.0);
	float count = 0.0;

	for (float phi = 0.0; phi < 2.0 * PI; phi += delta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += delta)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 sampleDir = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			// Mip 2 : lisse le bruit de l'échantillonnage régulier
			irradiance += textureLod(u_EnvironmentMap, sampleDir, 2.0).rgb * cos(theta) * sin(theta);
			count += 1.0;
		}
	}

	o_Color = vec4(PI * irradiance / count, 1.0);
}
