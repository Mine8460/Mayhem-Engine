// Contour de sélection, étape 2 : dessine le bord extérieur du masque par-dessus l'image
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // quad -0.5..0.5

out vec2 v_UV;

void main()
{
	v_UV = a_Position.xy + 0.5;
	gl_Position = vec4(a_Position.xy * 2.0, 0.0, 1.0); // plein écran
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec2 v_UV;

uniform sampler2D u_Mask;
uniform vec4 u_Color;
uniform float u_Width; // épaisseur en pixels

void main()
{
	// L'intérieur de l'objet n'est pas recouvert
	if (texture(u_Mask, v_UV).r > 0.5)
		discard;

	vec2 texel = 1.0 / vec2(textureSize(u_Mask, 0));
	int radius = int(ceil(u_Width));

	// Distance au pixel sélectionné le plus proche
	float minDistance = 1e6;
	for (int x = -radius; x <= radius; x++)
	{
		for (int y = -radius; y <= radius; y++)
		{
			if (texture(u_Mask, v_UV + vec2(x, y) * texel).r > 0.5)
				minDistance = min(minDistance, length(vec2(x, y)));
		}
	}

	// Bord adouci (anti-crénelage)
	float alpha = 1.0 - smoothstep(u_Width - 0.5, u_Width + 0.5, minDistance);
	if (alpha <= 0.0)
		discard;

	o_Color = vec4(u_Color.rgb, u_Color.a * alpha);
}
