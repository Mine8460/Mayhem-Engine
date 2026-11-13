// Grille infinie de l'éditeur, sur le plan y = 0
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // quad -0.5..0.5

out vec2 v_NDC;

void main()
{
	v_NDC = a_Position.xy * 2.0;
	gl_Position = vec4(v_NDC, 0.0, 1.0); // plein écran
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

in vec2 v_NDC;

uniform mat4 u_ViewProjection;
uniform mat4 u_InverseViewProjection;
uniform vec3 u_CameraPosition;

vec3 Unproject(vec2 _ndc, float _depth)
{
	vec4 p = u_InverseViewProjection * vec4(_ndc, _depth, 1.0);
	return p.xyz / p.w;
}

// Intensité (0..1) d'une ligne de grille de taille _cell, avec anti-crénelage via fwidth
float GridLines(vec2 _coord, float _cell)
{
	vec2 c = _coord / _cell;
	vec2 derivative = fwidth(c);
	vec2 grid = abs(fract(c - 0.5) - 0.5) / derivative;
	float line = min(grid.x, grid.y);
	return 1.0 - min(line, 1.0);
}

// Atténue une grille quand ses lignes deviennent trop serrées à l'écran (évite le fourmillement)
float DensityFade(vec2 _coord, float _cell)
{
	vec2 derivative = fwidth(_coord / _cell);
	return 1.0 - smoothstep(0.15, 0.4, max(derivative.x, derivative.y));
}

void main()
{
	// Rayon de la caméra pour ce pixel, intersecté avec le plan y = 0
	vec3 nearPoint = Unproject(v_NDC, -1.0);
	vec3 farPoint = Unproject(v_NDC, 1.0);
	vec3 ray = farPoint - nearPoint;

	if (abs(ray.y) < 1e-6)
		discard;

	float t = -nearPoint.y / ray.y;
	if (t <= 0.0 || t > 1.0) // sol derrière la caméra ou au-delà du far plane
		discard;

	vec3 position = nearPoint + ray * t;

	// Profondeur réelle du point : les objets passent devant / derrière la grille
	vec4 clip = u_ViewProjection * vec4(position, 1.0);
	gl_FragDepth = (clip.z / clip.w) * 0.5 + 0.5;

	vec2 coord = position.xz;

	// ---------- Lignes ----------
	float minor = GridLines(coord, 1.0) * DensityFade(coord, 1.0);
	float major = GridLines(coord, 10.0) * DensityFade(coord, 10.0);

	vec4 color = vec4(0.0);
	color = mix(color, vec4(0.55, 0.55, 0.60, 0.25), minor);
	color = mix(color, vec4(0.65, 0.65, 0.70, 0.55), major);

	// ---------- Axes ----------
	vec2 axisWidth = fwidth(coord) * 1.5;
	float axisX = 1.0 - min(abs(position.z) / axisWidth.y, 1.0); // ligne z = 0 : axe X
	float axisZ = 1.0 - min(abs(position.x) / axisWidth.x, 1.0); // ligne x = 0 : axe Z
	color = mix(color, vec4(0.90, 0.25, 0.25, 0.9), axisX);
	color = mix(color, vec4(0.25, 0.45, 0.95, 0.9), axisZ);

	// ---------- Fondus ----------
	// Distance : la grille s'étend plus loin quand la caméra est haute
	float height = abs(u_CameraPosition.y);
	float fadeEnd = max(60.0, height * 25.0);
	float distanceToCamera = length(position.xz - u_CameraPosition.xz);
	color.a *= 1.0 - smoothstep(fadeEnd * 0.35, fadeEnd, distanceToCamera);

	// Angle rasant : évite le moiré près de l'horizon
	float grazing = abs(normalize(ray).y);
	color.a *= smoothstep(0.0, 0.12, grazing);

	if (color.a <= 0.002)
		discard;

	o_Color = color;
}
