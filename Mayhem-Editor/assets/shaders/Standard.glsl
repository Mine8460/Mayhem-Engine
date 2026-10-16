#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform vec4 u_UVRect;       // (minU, minV, maxU, maxV) -> sous-texture / spritesheet
uniform float u_TilingFactor;

out vec3 v_Normal;
out vec2 v_TexCoord;

void main()
{
	v_Normal = normalize(mat3(transpose(inverse(u_Transform))) * a_Normal);
	v_TexCoord = mix(u_UVRect.xy, u_UVRect.zw, a_TexCoord) * u_TilingFactor;
	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec3 v_Normal;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform int u_Lit;           // 0 = 2D (pas d'éclairage), 1 = 3D
uniform int u_EntityID;

// Lumière directionnelle
uniform vec3 u_LightDirection; // direction dans laquelle la lumière voyage (normalisée)
uniform vec3 u_LightColor;     // couleur * intensité
uniform vec3 u_AmbientColor;

void main()
{
	vec4 color = texture(u_Texture, v_TexCoord) * u_Color;

	if (u_Lit == 1)
	{
		vec3 N = normalize(v_Normal);
		vec3 L = -u_LightDirection; // vers la lumière
		float diffuse = max(dot(N, L), 0.0);
		color.rgb *= u_AmbientColor + u_LightColor * diffuse;
	}

	if (color.a < 0.01)
		discard; // évite que les pixels transparents des sprites écrivent dans le depth buffer

	o_Color = color;
	o_EntityID = u_EntityID;
}
