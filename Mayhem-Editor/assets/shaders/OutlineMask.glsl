// Contour de sélection, étape 1 : masque des objets sélectionnés
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform vec4 u_UVRect;
uniform vec2 u_Tiling;
uniform vec2 u_Offset;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = mix(u_UVRect.xy, u_UVRect.zw, a_TexCoord) * u_Tiling + u_Offset;
	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Mask;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Alpha;
uniform float u_AlphaCutoff;

void main()
{
	// Respecte la transparence (un sprite détouré a un contour qui suit sa forme, pas son rectangle)
	float alpha = texture(u_Texture, v_TexCoord).a * u_Alpha;
	if (alpha < u_AlphaCutoff)
		discard;

	o_Mask = vec4(1.0);
}
