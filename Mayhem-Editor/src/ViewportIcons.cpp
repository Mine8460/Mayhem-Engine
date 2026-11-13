#include "ViewportIcons.h"

#include <algorithm>
#include <vector>
#include <cmath>

namespace Mayhem
{
	static constexpr float s_IconRadius = 15.0f; // pixels

	// =====================================================================
	//  Projection 3D -> écran
	// =====================================================================

	struct ScreenProjector
	{
		glm::mat4 ViewProjection;
		ImVec2 Min, Size;

		ImVec2 ToScreen(const glm::vec4& _clip) const
		{
			glm::vec2 ndc = glm::vec2(_clip) / _clip.w;
			return ImVec2(Min.x + (ndc.x * 0.5f + 0.5f) * Size.x,
			              Min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * Size.y);
		}

		// false si le point est derrière la caméra
		bool Project(const glm::vec3& _point, ImVec2& _out, float& _depth) const
		{
			glm::vec4 clip = ViewProjection * glm::vec4(_point, 1.0f);
			if (clip.w <= 1e-4f)
				return false;
			_out = ToScreen(clip);
			_depth = clip.w;
			return true;
		}

		// Segment 3D, coupé proprement s'il passe derrière la caméra
		void Line(ImDrawList* _dl, const glm::vec3& _a, const glm::vec3& _b, ImU32 _color, float _thickness = 1.5f) const
		{
			glm::vec4 a = ViewProjection * glm::vec4(_a, 1.0f);
			glm::vec4 b = ViewProjection * glm::vec4(_b, 1.0f);

			const float eps = 1e-4f;
			if (a.w < eps && b.w < eps)
				return;
			if (a.w < eps)
				a = glm::mix(a, b, (eps - a.w) / (b.w - a.w));
			else if (b.w < eps)
				b = glm::mix(b, a, (eps - b.w) / (a.w - b.w));

			_dl->AddLine(ToScreen(a), ToScreen(b), _color, _thickness);
		}
	};

	static glm::mat4 RemoveScale(const glm::mat4& _m)
	{
		glm::mat4 result = _m;
		for (int i = 0; i < 3; i++)
		{
			glm::vec3 axis = glm::vec3(_m[i]);
			float length = glm::length(axis);
			result[i] = glm::vec4(length > 1e-6f ? axis / length : axis, 0.0f);
		}
		return result;
	}

	// =====================================================================
	//  Dessins des icônes
	// =====================================================================

	using IconPainter = void(*)(ImDrawList*, ImVec2, float, ImU32);

	static void PaintCamera(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddRectFilled(ImVec2(c.x - r * 0.55f, c.y - r * 0.25f), ImVec2(c.x + r * 0.2f, c.y + r * 0.38f), col, 2.0f);
		dl->AddTriangleFilled(ImVec2(c.x + r * 0.2f, c.y + r * 0.06f), ImVec2(c.x + r * 0.58f, c.y - r * 0.22f), ImVec2(c.x + r * 0.58f, c.y + r * 0.34f), col);
		//dl->AddCircleFilled(ImVec2(c.x - r * 0.33f, c.y - r * 0.42f), r * 0.17f, col);
		//dl->AddCircleFilled(ImVec2(c.x - r * 0.02f, c.y - r * 0.42f), r * 0.17f, col);
	}

	static void PaintSun(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddCircleFilled(c, r * 0.28f, col);
		for (int i = 0; i < 8; i++)
		{
			float a = i * 3.14159265f / 4.0f;
			ImVec2 d(cosf(a), sinf(a));
			dl->AddLine(ImVec2(c.x + d.x * r * 0.42f, c.y + d.y * r * 0.42f), ImVec2(c.x + d.x * r * 0.66f, c.y + d.y * r * 0.66f), col, 1.6f);
		}
	}

	// Ampoule : globe + culot
	static void PaintBulb(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.12f), r * 0.34f, col);
		dl->AddRectFilled(ImVec2(c.x - r * 0.17f, c.y + r * 0.2f), ImVec2(c.x + r * 0.17f, c.y + r * 0.36f), col, 1.5f);
		dl->AddRectFilled(ImVec2(c.x - r * 0.11f, c.y + r * 0.4f), ImVec2(c.x + r * 0.11f, c.y + r * 0.5f), col, 1.0f);
	}

	// Projecteur : cône de lumière
	static void PaintSpot(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddTriangleFilled(ImVec2(c.x, c.y - r * 0.45f), ImVec2(c.x - r * 0.5f, c.y + r * 0.4f), ImVec2(c.x + r * 0.5f, c.y + r * 0.4f), col);
		dl->AddLine(ImVec2(c.x - r * 0.62f, c.y + r * 0.56f), ImVec2(c.x + r * 0.62f, c.y + r * 0.56f), col, 1.8f);
	}

	static void PaintCloud(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
	{
		dl->AddCircleFilled(ImVec2(c.x - r * 0.28f, c.y + r * 0.08f), r * 0.24f, col);
		dl->AddCircleFilled(ImVec2(c.x + r * 0.02f, c.y - r * 0.08f), r * 0.32f, col);
		dl->AddCircleFilled(ImVec2(c.x + r * 0.32f, c.y + r * 0.1f), r * 0.21f, col);
		dl->AddRectFilled(ImVec2(c.x - r * 0.28f, c.y + r * 0.08f), ImVec2(c.x + r * 0.32f, c.y + r * 0.32f), col);
	}

	// =====================================================================
	//  Aides visuelles des objets sélectionnés
	// =====================================================================

	static void DrawCameraFrustum(ImDrawList* _dl, const ScreenProjector& _proj, const glm::mat4& _cameraWorld, const glm::mat4& _cameraProjection, ImU32 _color)
	{
		glm::mat4 view = glm::inverse(RemoveScale(_cameraWorld));
		glm::mat4 inverseViewProj = glm::inverse(_cameraProjection * view);

		glm::vec3 corners[8];
		int i = 0;
		for (float z : { -1.0f, 1.0f })
			for (float y : { -1.0f, 1.0f })
				for (float x : { -1.0f, 1.0f })
				{
					glm::vec4 p = inverseViewProj * glm::vec4(x, y, z, 1.0f);
					corners[i++] = glm::vec3(p) / p.w;
				}

		// Indices : 0..3 = plan proche, 4..7 = plan lointain (ordre x, y)
		const int edges[12][2] = {
			{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },   // proche
			{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },   // lointain
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }    // côtés
		};
		for (const auto& e : edges)
			_proj.Line(_dl, corners[e[0]], corners[e[1]], _color, 1.2f);
	}

	static void DrawLightDirection(ImDrawList* _dl, const ScreenProjector& _proj, const glm::mat4& _lightWorld, ImU32 _color)
	{
		glm::vec3 position = glm::vec3(_lightWorld[3]);
		glm::vec3 direction = glm::normalize(glm::vec3(_lightWorld * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

		// Repère perpendiculaire à la direction
		glm::vec3 helper = std::abs(direction.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 u = glm::normalize(glm::cross(direction, helper));
		glm::vec3 v = glm::cross(direction, u);

		const float radius = 0.5f, length = 2.0f;
		const int segments = 24;

		// Disque
		for (int s = 0; s < segments; s++)
		{
			float a0 = s * 6.2831853f / segments, a1 = (s + 1) * 6.2831853f / segments;
			glm::vec3 p0 = position + (u * cosf(a0) + v * sinf(a0)) * radius;
			glm::vec3 p1 = position + (u * cosf(a1) + v * sinf(a1)) * radius;
			_proj.Line(_dl, p0, p1, _color);
		}

		// Rayons dans le sens de l'éclairage
		for (int s = 0; s < 8; s++)
		{
			float a = s * 6.2831853f / 8.0f;
			glm::vec3 start = position + (u * cosf(a) + v * sinf(a)) * radius;
			_proj.Line(_dl, start, start + direction * length, _color);
		}
		_proj.Line(_dl, position, position + direction * length * 1.25f, _color, 2.0f);
	}

	// Cercle de rayon _radius dans le plan (_u, _v), centré en _center
	static void DrawCircle3D(ImDrawList* _dl, const ScreenProjector& _proj, const glm::vec3& _center, const glm::vec3& _u, const glm::vec3& _v,
		float _radius, ImU32 _color)
	{
		const int segments = 48;
		for (int s = 0; s < segments; s++)
		{
			float a0 = s * 6.2831853f / segments, a1 = (s + 1) * 6.2831853f / segments;
			_proj.Line(_dl, _center + (_u * cosf(a0) + _v * sinf(a0)) * _radius, _center + (_u * cosf(a1) + _v * sinf(a1)) * _radius, _color, 1.2f);
		}
	}

	// Sphère de portée : trois grands cercles
	static void DrawPointLightRange(ImDrawList* _dl, const ScreenProjector& _proj, const glm::mat4& _lightWorld, float _range, ImU32 _color)
	{
		const glm::vec3 position = glm::vec3(_lightWorld[3]);
		DrawCircle3D(_dl, _proj, position, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), _range, _color);
		DrawCircle3D(_dl, _proj, position, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), _range, _color);
		DrawCircle3D(_dl, _proj, position, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), _range, _color);
	}

	// Cône d'éclairage : cercle à la portée (angle externe), cercle interne plus pâle, arêtes
	static void DrawSpotLightCone(ImDrawList* _dl, const ScreenProjector& _proj, const glm::mat4& _lightWorld,
		float _range, float _innerAngle, float _outerAngle, ImU32 _color)
	{
		const glm::vec3 position = glm::vec3(_lightWorld[3]);
		const glm::vec3 direction = glm::normalize(glm::vec3(_lightWorld * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
		const glm::vec3 helper = std::abs(direction.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::vec3 u = glm::normalize(glm::cross(direction, helper));
		const glm::vec3 v = glm::cross(direction, u);

		// Le cône s'arrête à _range sur l'axe : le rayon du cercle vaut range * tan(demi-angle)
		auto circleAt = [&](float _angle, ImU32 _col)
		{
			const float half = glm::radians(glm::clamp(_angle, 0.0f, 179.0f) * 0.5f);
			const float radius = _range * std::tan(half);
			DrawCircle3D(_dl, _proj, position + direction * _range, u, v, radius, _col);
			return radius;
		};

		const float outerRadius = circleAt(_outerAngle, _color);
		if (_innerAngle > 0.5f && _innerAngle < _outerAngle - 0.5f)
			circleAt(_innerAngle, (_color & 0x00FFFFFFu) | 0x60000000u);

		for (int s = 0; s < 4; s++)
		{
			float a = s * 6.2831853f / 4.0f;
			_proj.Line(_dl, position, position + direction * _range + (u * cosf(a) + v * sinf(a)) * outerRadius, _color, 1.2f);
		}
		_proj.Line(_dl, position, position + direction * _range, _color, 1.0f);
	}

	// =====================================================================

	Entity ViewportIcons::Draw(const Ref<Scene>& _scene, const glm::mat4& _viewProjection,
		const ImVec2& _viewportMin, const ImVec2& _viewportMax, Entity _selected)
	{
		ScreenProjector proj{ _viewProjection, _viewportMin, ImVec2(_viewportMax.x - _viewportMin.x, _viewportMax.y - _viewportMin.y) };

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->PushClipRect(_viewportMin, _viewportMax, true);

		const ImU32 helperColor = IM_COL32(230, 230, 230, 200);

		// ---------- Aides de l'objet sélectionné (sous les icônes) ----------
		if (_selected)
		{
			glm::mat4 world = _selected.GetWorldTransform();

			if (_selected.HasComponent<CameraComponent>())
				DrawCameraFrustum(dl, proj, world, _selected.GetComponent<CameraComponent>().m_Camera.GetProjection(), helperColor);

			if (_selected.HasComponent<DirectionalLight>())
				DrawLightDirection(dl, proj, world, IM_COL32(255, 220, 90, 220));

			if (_selected.HasComponent<PointLight>())
				DrawPointLightRange(dl, proj, world, _selected.GetComponent<PointLight>().m_Range, IM_COL32(255, 220, 90, 200));

			if (_selected.HasComponent<SpotLight>())
			{
				const SpotLight& spot = _selected.GetComponent<SpotLight>();
				DrawSpotLightCone(dl, proj, world, spot.m_Range, spot.m_InnerAngle, spot.m_OuterAngle, IM_COL32(255, 220, 90, 200));
			}
		}

		// ---------- Collecte des icônes ----------
		struct Icon
		{
			Entity Owner;
			ImVec2 Position;
			float Depth;
			IconPainter Paint;
			ImU32 Color;
		};
		std::vector<Icon> icons;

		auto collect = [&](entt::entity _handle, IconPainter _paint, ImU32 _color)
		{
			Entity entity{ _handle, _scene.get() };
			ImVec2 screen;
			float depth;
			if (proj.Project(glm::vec3(entity.GetWorldTransform()[3]), screen, depth))
				icons.push_back({ entity, screen, depth, _paint, _color });
		};

		// Une seule icône par entité, par priorité : caméra > lumière > ciel
		for (auto e : _scene->GetAllEntitiesWith<CameraComponent>())
			collect(e, PaintCamera, IM_COL32(235, 235, 240, 255));

		for (auto e : _scene->GetAllEntitiesWith<DirectionalLight>())
			if (!_scene->GetAllEntitiesWith<CameraComponent>().contains(e))
				collect(e, PaintSun, IM_COL32(255, 215, 80, 255));

		for (auto e : _scene->GetAllEntitiesWith<PointLight>())
			if (!_scene->GetAllEntitiesWith<CameraComponent>().contains(e) && !_scene->GetAllEntitiesWith<DirectionalLight>().contains(e))
				collect(e, PaintBulb, IM_COL32(255, 215, 80, 255));

		for (auto e : _scene->GetAllEntitiesWith<SpotLight>())
			if (!_scene->GetAllEntitiesWith<CameraComponent>().contains(e) && !_scene->GetAllEntitiesWith<DirectionalLight>().contains(e)
				&& !_scene->GetAllEntitiesWith<PointLight>().contains(e))
				collect(e, PaintSpot, IM_COL32(255, 215, 80, 255));

		for (auto e : _scene->GetAllEntitiesWith<Skybox>())
			if (!_scene->GetAllEntitiesWith<CameraComponent>().contains(e) && !_scene->GetAllEntitiesWith<DirectionalLight>().contains(e)
				&& !_scene->GetAllEntitiesWith<PointLight>().contains(e) && !_scene->GetAllEntitiesWith<SpotLight>().contains(e))
				collect(e, PaintCloud, IM_COL32(150, 200, 255, 255));

		// Du plus loin au plus proche : l'icône la plus proche est dessinée par-dessus
		std::sort(icons.begin(), icons.end(), [](const Icon& a, const Icon& b) { return a.Depth > b.Depth; });

		// ---------- Survol ----------
		Entity hovered{ entt::null, _scene.get() };
		if (ImGui::IsWindowHovered())
		{
			ImVec2 mouse = ImGui::GetMousePos();
			for (const Icon& icon : icons) // le dernier touché = le plus proche
			{
				float dx = mouse.x - icon.Position.x, dy = mouse.y - icon.Position.y;
				if (dx * dx + dy * dy <= s_IconRadius * s_IconRadius)
					hovered = icon.Owner;
			}
		}

		// ---------- Dessin ----------
		for (Icon& icon : icons)
		{
			const bool isSelected = _selected && icon.Owner == _selected;
			const bool isHovered = hovered && icon.Owner == hovered;

			dl->AddCircleFilled(icon.Position, s_IconRadius, isHovered ? IM_COL32(55, 55, 62, 230) : IM_COL32(30, 30, 35, 200), 24);
			if (isSelected)
				dl->AddCircle(icon.Position, s_IconRadius, SelectedColor, 24, 2.0f);
			else if (isHovered)
				dl->AddCircle(icon.Position, s_IconRadius, IM_COL32(200, 200, 210, 200), 24, 1.5f);

			icon.Paint(dl, icon.Position, s_IconRadius, isSelected ? SelectedColor : icon.Color);
		}

		dl->PopClipRect();

		if (hovered)
			ImGui::SetTooltip("%s", hovered.GetComponent<TagComponent>().Tag.c_str());

		return hovered;
	}
}
