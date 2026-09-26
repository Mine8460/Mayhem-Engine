#type vertex
#version 330 core
			
layout(location = 0) in vec3 a_Position;
uniform mat4 u_Transform;
uniform vec4 a_Color;
uniform float a_TexIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
flat out float v_TexIndex;

void main()
{
	v_TexIndex = a_TexIndex;
	v_Color = a_Color;
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0) * u_Transform;
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec4 v_Color;
flat in float v_TexIndex;
			
uniform sampler2D u_Texture[32];

void main()
{
	color = texture(u_Texture[int(v_TexIndex)], vec2(0.0,0.0)) * v_Color;
}