#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform vec4 u_UVRect;   // (minU, minV, maxU, maxV) -> sous-texture / spritesheet
uniform vec2 u_Tiling;
uniform vec2 u_Offset;

out vec3 v_WorldPosition;
out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
	vec4 worldPos = u_Transform * vec4(a_Position, 1.0);

	v_WorldPosition = worldPos.xyz;
	v_Normal = normalize(mat3(transpose(inverse(u_Transform))) * a_Normal);
	v_TexCoord = mix(u_UVRect.xy, u_UVRect.zw, a_TexCoord) * u_Tiling + u_Offset;

	gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec3 v_WorldPosition;
in vec3 v_Normal;
in vec2 v_TexCoord;

// Matériau
uniform sampler2D u_Texture;
uniform vec4 u_Color;            // albedo * teinte de l'instance
uniform float u_AlphaCutoff;
uniform int u_Lit;               // 0 = couleur brute, 1 = éclairé
uniform float u_SpecularStrength;
uniform float u_Shininess;
uniform vec3 u_EmissionColor;    // couleur * intensité

// Scène
uniform vec3 u_CameraPosition;
uniform vec3 u_LightDirection;   // direction dans laquelle la lumière voyage
uniform vec3 u_LightColor;       // couleur * intensité
uniform vec3 u_AmbientColor;

uniform int u_EntityID;

void main()
{
	vec4 albedo = texture(u_Texture, v_TexCoord) * u_Color;

	if (albedo.a < u_AlphaCutoff)
		discard;

	vec3 color = albedo.rgb;

	if (u_Lit == 1)
	{
		vec3 N = normalize(v_Normal);
		vec3 L = -u_LightDirection;
		vec3 V = normalize(u_CameraPosition - v_WorldPosition);
		vec3 H = normalize(L + V);

		float diffuse = max(dot(N, L), 0.0);
		float specular = diffuse > 0.0 ? pow(max(dot(N, H), 0.0), max(u_Shininess, 1.0)) * u_SpecularStrength : 0.0;

		color = albedo.rgb * (u_AmbientColor + u_LightColor * diffuse) + u_LightColor * specular;
	}

	color += u_EmissionColor;

	o_Color = vec4(color, albedo.a);
	o_EntityID = u_EntityID;
}
