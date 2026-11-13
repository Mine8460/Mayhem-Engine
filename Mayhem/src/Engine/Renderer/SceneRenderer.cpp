#include "Enpch.h"
#include "SceneRenderer.h"

#include "../Scene/Components.h" // SpriteRenderer, MeshRenderer
#include "Shader.h"
#include "RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>

#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>

namespace Mayhem
{
	static const glm::vec3 s_DefaultLightDirection = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.6f));
	static const glm::vec4 s_FullUV = { 0.0f, 0.0f, 1.0f, 1.0f }; // (minU, minV, maxU, maxV)

	static constexpr uint32_t s_ShadowMapSlot = Material::MapCount;     // slots 0..MapCount-1 = textures du matériau
	static constexpr uint32_t s_IrradianceSlot = Material::MapCount + 1;
	static constexpr uint32_t s_PrefilterSlot = Material::MapCount + 2;
	static constexpr uint32_t s_LocalShadowSlot = Material::MapCount + 3;
	static constexpr float s_LocalShadowNear = 0.1f;          // doit correspondre à LOCAL_SHADOW_NEAR dans Standard.glsl
	static constexpr float s_PointShadowFov = 92.0f;          // un peu plus que 90° : pas de couture entre les faces du cube
	static constexpr float s_ShadowCasterMargin = 100.0f;   // profondeur en plus côté lumière : capte les objets hors cadre qui projettent une ombre dedans

	// Un draw en attente (rendu dans EndScene)
	struct DrawCommand
	{
		Ref<Mesh> MeshPtr;
		glm::mat4 Transform;
		Ref<Material> MaterialPtr;
		Ref<Texture2D> TextureOverride;
		glm::vec4 Tint;
		float TilingMultiplier;
		glm::vec4 UVRect;
		int EntityID;
		uint32_t BaseIndex;  // plage d'indices dessinée (sous-mesh)
		uint32_t IndexCount;
		WorldBounds Bounds; // boîte englobante en monde (frustum culling)
	};

	// Une point light ou spot light prête pour le shader (couleurs déjà en linéaire * intensité)
	struct LocalLight
	{
		glm::vec4 PositionRange; // xyz = position, w = portée
		glm::vec4 ColorType;     // rgb = couleur * intensité, w = 0 point / 1 spot
		glm::vec4 Direction;     // xyz = sens du cône (spot)
		glm::vec4 Spot;          // x = cos(demi-angle externe), y = cos(demi-angle interne), z = première couche d'ombre (-1 = aucune), w = biais d'ombre (m)
		                         // Direction.w = taille d'un texel d'ombre par mètre de distance (biais le long de la normale)

		bool IsSpot = false;
		float OuterAngle = 45.0f;
		bool CastShadows = false;
		int ShadowLayer = -1;
		glm::mat4 ShadowMatrices[6]; // une par face (1 pour un spot, 6 pour un point)
	};

	struct Cascade
	{
		glm::mat4 LightSpaceMatrix = glm::mat4(1.0f);
		float SplitFar = 0.0f;   // distance (vue) de fin de la cascade
		float TexelSize = 0.0f;  // taille d'un texel d'ombre en mètres
	};

	struct SceneRendererData
	{
		Ref<Texture2D> WhiteTexture;

		Ref<Mesh> QuadMesh;
		Ref<Mesh> PlaneMesh;
		Ref<Mesh> CubeMesh;

		// Caméra de la scène en cours
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 ViewProjection = glm::mat4(1.0f);
		glm::vec3 CameraPosition = glm::vec3(0.0f);

		// Lumière
		glm::vec3 LightDirection = s_DefaultLightDirection; // direction dans laquelle la lumière voyage
		glm::vec3 LightColor = glm::vec3(1.0f);              // couleur * intensité
		glm::vec3 AmbientColor = glm::vec3(0.2f);

		// Lumières locales (point / spot)
		std::vector<LocalLight> Lights;

		// Ombres des lumières locales : un tableau de couches (une par spot, six par point light)
		Ref<ShadowMap> LocalShadow;

		// Ombres
		Ref<ShadowMap> Shadow;
		Ref<Shader> ShadowShader;
		SceneRenderer::ShadowSettings Shadows;
		Cascade Cascades[SceneRenderer::CascadeCount];

		// Ciel
		Ref<Shader> SkyShader;
		SceneRenderer::SkySettings Sky;

		// Éclairage par l'environnement (IBL)
		Ref<EnvironmentMap> Environment;
		struct EnvironmentOverride
		{
			bool Active = false;
			glm::vec3 Zenith = glm::vec3(0.0f), Horizon = glm::vec3(0.0f), Ground = glm::vec3(0.0f); // sRGB
			float Intensity = 1.0f;
		} EnvOverride;

		// Sortie couleur
		SceneRenderer::ColorSettings Color;

		// Contour de sélection
		Ref<OutlinePass> Outline;
		Ref<Shader> OutlineMaskShader;
		Ref<Shader> OutlineShader;
		std::unordered_set<int> OutlinedEntities;
		glm::vec4 OutlineColor = glm::vec4(1.0f, 0.55f, 0.1f, 1.0f);
		float OutlineWidth = 2.0f;

		// Grille
		Ref<Shader> GridShader;
		bool GridEnabled = false;

		// Arêtes
		Ref<Shader> WireframeShader;
		bool WireframeEnabled = false;
		glm::vec4 WireframeColor = glm::vec4(0.55f, 0.85f, 1.0f, 0.55f);

		bool OverlaysSuspended = false; // miniatures : ni grille, ni contour, ni arêtes

		std::vector<DrawCommand> DrawQueue;
		bool FrustumCulling = true;

		// Shaders qui ont déjà reçu les uniforms de scène pour cette passe
		std::unordered_set<Shader*> PreparedShaders;

		SceneRenderer::Statistics Stats;
	};

	static SceneRendererData s_Data;

	// Couleur choisie dans l'éditeur (sRGB) -> espace linéaire pour les calculs de lumière.
	// À appliquer AVANT de multiplier par une intensité.
	static glm::vec3 SRGBToLinear(const glm::vec3& _c)
	{
		return glm::pow(glm::max(_c, glm::vec3(0.0f)), glm::vec3(2.2f));
	}

	static glm::mat4 MakeTransform(const glm::vec3& _pos, const glm::vec3& _size)
	{
		return glm::translate(glm::mat4(1.0f), _pos) * glm::scale(glm::mat4(1.0f), _size);
	}

	// Masque (1 sur le canal choisi) : le shader lit le canal avec dot(texel, masque)
	static glm::vec4 ChannelMask(TextureChannel _channel)
	{
		glm::vec4 mask(0.0f);
		mask[(int)_channel] = 1.0f;
		return mask;
	}

	static glm::vec4 UVRectFromCoords(const glm::vec2* _tc)
	{
		// Coin 0 = min, coin 2 = max
		return { _tc[0].x, _tc[0].y, _tc[2].x, _tc[2].y };
	}

	void SceneRenderer::Init()
	{
		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_Data.QuadMesh = Mesh::CreateQuad();
		s_Data.PlaneMesh = Mesh::CreatePlane();
		s_Data.CubeMesh = Mesh::CreateCube();

		s_Data.Environment = Ref<EnvironmentMap>(new EnvironmentMap());
		s_Data.LocalShadow = Ref<ShadowMap>(new ShadowMap(LocalShadowResolution, LocalShadowLayers));
		s_Data.Shadow = Ref<ShadowMap>(new ShadowMap(ShadowSettings().Resolution, CascadeCount));
		s_Data.ShadowShader = Shader::Create("assets/shaders/Shadow.glsl");
		s_Data.SkyShader = Shader::Create("assets/shaders/Sky.glsl");

		s_Data.Outline = Ref<OutlinePass>(new OutlinePass());
		s_Data.OutlineMaskShader = Shader::Create("assets/shaders/OutlineMask.glsl");
		s_Data.OutlineShader = Shader::Create("assets/shaders/Outline.glsl");

		s_Data.GridShader = Shader::Create("assets/shaders/Grid.glsl");
		s_Data.WireframeShader = Shader::Create("assets/shaders/Wireframe.glsl");

		s_Data.DrawQueue.reserve(1024);
	}

	void SceneRenderer::Shutdown()
	{
		s_Data.DrawQueue.clear();
		s_Data.Environment.reset();
		s_Data.LocalShadow.reset();
		s_Data.Shadow.reset();
		s_Data.ShadowShader.reset();
		s_Data.SkyShader.reset();
		s_Data.Outline.reset();
		s_Data.OutlineMaskShader.reset();
		s_Data.OutlineShader.reset();
		s_Data.GridShader.reset();
		s_Data.WireframeShader.reset();
	}

	// ---------- Scène ----------

	void SceneRenderer::SetCamera(const glm::mat4& _view, const glm::mat4& _projection)
	{
		s_Data.View = _view;
		s_Data.Projection = _projection;
		s_Data.ViewProjection = _projection * _view;
		s_Data.CameraPosition = glm::vec3(glm::inverse(_view)[3]);

		// Pas de DirectionalLight dans la scène = pas de lumière directe ni d'ambiant : tout est sombre.
		// La scène en ajoute une avec SetDirectionalLight (donc retirer la lumière de la scène l'éteint vraiment).
		s_Data.LightDirection = s_DefaultLightDirection; // seulement pour orienter le ciel affiché
		s_Data.LightColor = glm::vec3(0.0f);
		s_Data.AmbientColor = glm::vec3(0.0f);
		s_Data.Shadows = ShadowSettings();
		s_Data.Shadows.Enabled = false; // rien à ombrer sans lumière directionnelle
		s_Data.Lights.clear();
		s_Data.Sky = SkySettings(); // pas de ciel si la scène n'a pas de Skybox
		s_Data.EnvOverride.Active = false;
		s_Data.Color = ColorSettings();

		s_Data.DrawQueue.clear(); // au cas où le EndScene précédent n'a pas été appelé
		s_Data.PreparedShaders.clear();
	}

	void SceneRenderer::BeginScene(const OrthographicCamera& _cam)
	{
		SetCamera(_cam.GetViewMatrix(), _cam.GetProjectionMatrix());
	}

	void SceneRenderer::BeginScene(const EditorCamera& _cam)
	{
		SetCamera(_cam.GetViewMatrix(), _cam.GetProjection());
	}

	void SceneRenderer::BeginScene(const Camera& _cam, const glm::mat4& _transform)
	{
		SetCamera(glm::inverse(_transform), _cam.GetProjection());
	}

	void SceneRenderer::EndScene()
	{
		UpdateEnvironment();
		SelectLights();

		if (s_Data.Shadows.Enabled)
		{
			ComputeCascades();
			RenderShadowPass();
		}

		RenderLocalShadowPass();

		RenderMainPass();

		// Après les objets : le ciel ne remplit que les pixels restés vides (test de profondeur)
		if (s_Data.Sky.Enabled)
			RenderSkyPass();

		// Les arêtes se dessinent sur les objets (donc avant la grille, qui se mélange par-dessus)
		if (s_Data.WireframeEnabled && !s_Data.OverlaysSuspended)
			RenderWireframePass();

		// Après le ciel (elle se mélange par-dessus), avant le contour
		if (s_Data.GridEnabled && !s_Data.OverlaysSuspended)
			RenderGridPass();

		// En dernier : le contour passe par-dessus tout (objets et ciel)
		if (!s_Data.OutlinedEntities.empty() && !s_Data.OverlaysSuspended)
			RenderOutlinePass();

		s_Data.DrawQueue.clear();
	}

	// ---------- Lumières locales ----------

	void SceneRenderer::AddPointLight(const glm::vec3& _position, const glm::vec3& _color, float _intensity, float _range,
		bool _castShadows, float _shadowBias)
	{
		LocalLight light;
		light.PositionRange = glm::vec4(_position, glm::max(_range, 0.2f));
		light.ColorType = glm::vec4(SRGBToLinear(_color) * glm::max(_intensity, 0.0f), 0.0f);
		light.Direction = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		light.Spot = glm::vec4(-1.0f, -1.0f, -1.0f, glm::max(_shadowBias, 0.0f));
		light.IsSpot = false;
		light.CastShadows = _castShadows;
		s_Data.Lights.push_back(light);
	}

	void SceneRenderer::AddSpotLight(const glm::vec3& _position, const glm::vec3& _direction, const glm::vec3& _color, float _intensity,
		float _range, float _innerAngle, float _outerAngle, bool _castShadows, float _shadowBias)
	{
		const float outer = glm::clamp(_outerAngle, 1.0f, 179.0f);
		const float inner = glm::clamp(_innerAngle, 0.0f, outer);
		const float cosOuter = std::cos(glm::radians(outer * 0.5f));
		const float cosInner = glm::max(std::cos(glm::radians(inner * 0.5f)), cosOuter + 1e-4f); // jamais égaux : le fondu divise par l'écart

		LocalLight light;
		light.PositionRange = glm::vec4(_position, glm::max(_range, 0.2f));
		light.ColorType = glm::vec4(SRGBToLinear(_color) * glm::max(_intensity, 0.0f), 1.0f);
		light.Direction = glm::vec4(glm::length(_direction) > 1e-5f ? glm::normalize(_direction) : glm::vec3(0.0f, 0.0f, -1.0f), 0.0f);
		light.Spot = glm::vec4(cosOuter, cosInner, -1.0f, glm::max(_shadowBias, 0.0f));
		light.IsSpot = true;
		light.OuterAngle = outer;
		light.CastShadows = _castShadows;
		s_Data.Lights.push_back(light);
	}

	// Trie les lumières (les plus proches de la caméra d'abord), garde les MaxLights premières,
	// puis attribue les faces d'ombre disponibles et calcule leurs matrices.
	void SceneRenderer::SelectLights()
	{
		for (LocalLight& light : s_Data.Lights)
			light.ShadowLayer = -1;

		if (s_Data.Lights.empty())
			return;

		const glm::vec3 camera = s_Data.CameraPosition;
		std::stable_sort(s_Data.Lights.begin(), s_Data.Lights.end(), [&camera](const LocalLight& _a, const LocalLight& _b)
		{
			return glm::distance(glm::vec3(_a.PositionRange), camera) < glm::distance(glm::vec3(_b.PositionRange), camera);
		});
		if (s_Data.Lights.size() > MaxLights)
			s_Data.Lights.resize(MaxLights);

		if (!s_Data.LocalShadow)
			return;

		static const glm::vec3 faceTargets[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
		static const glm::vec3 faceUps[6] = { { 0, -1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, -1, 0 }, { 0, -1, 0 } };

		const Frustum cameraFrustum(s_Data.ViewProjection);
		const float resolution = (float)s_Data.LocalShadow->GetResolution();
		uint32_t usedLayers = 0;

		for (LocalLight& light : s_Data.Lights)
		{
			if (!light.CastShadows)
				continue;

			const uint32_t needed = light.IsSpot ? 1u : 6u;
			if (usedLayers + needed > LocalShadowLayers)
				continue;

			// Sphère d'influence entièrement hors du champ de la caméra : aucune surface visible à ombrer
			const glm::vec3 position = glm::vec3(light.PositionRange);
			const float range = light.PositionRange.w;
			WorldBounds influence;
			influence.Center = position;
			influence.Extents = glm::vec3(range);
			if (s_Data.FrustumCulling && !cameraFrustum.Intersects(influence))
				continue;

			float tanHalfFov = 1.0f;
			if (light.IsSpot)
			{
				const glm::vec3 direction = glm::vec3(light.Direction);
				const glm::vec3 up = std::abs(direction.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
				const float fov = glm::clamp(light.OuterAngle + 4.0f, 5.0f, 170.0f); // marge : le PCF déborde du cône
				tanHalfFov = std::tan(glm::radians(fov * 0.5f));
				light.ShadowMatrices[0] = glm::perspective(glm::radians(fov), 1.0f, s_LocalShadowNear, range)
					* glm::lookAt(position, position + direction, up);
			}
			else
			{
				tanHalfFov = std::tan(glm::radians(s_PointShadowFov * 0.5f));
				const glm::mat4 projection = glm::perspective(glm::radians(s_PointShadowFov), 1.0f, s_LocalShadowNear, range);
				for (int f = 0; f < 6; f++)
					light.ShadowMatrices[f] = projection * glm::lookAt(position, position + faceTargets[f], faceUps[f]);
			}

			light.ShadowLayer = (int)usedLayers;
			usedLayers += needed;
			light.Spot.z = (float)light.ShadowLayer;
			light.Direction.w = 2.0f * tanHalfFov / resolution;
		}
	}

	void SceneRenderer::RenderLocalShadowPass()
	{
		if (!s_Data.LocalShadow || !s_Data.ShadowShader)
			return;

		bool any = false;
		for (const LocalLight& light : s_Data.Lights)
			any |= light.ShadowLayer >= 0;
		if (!any)
			return;

		s_Data.LocalShadow->BeginPass();
		s_Data.ShadowShader->Bind();
		RenderCommand::SetCullMode(RendererAPI::CullMode::None);

		for (const LocalLight& light : s_Data.Lights)
		{
			if (light.ShadowLayer < 0)
				continue;

			const int faces = light.IsSpot ? 1 : 6;
			for (int f = 0; f < faces; f++)
			{
				s_Data.LocalShadow->BindLayer((uint32_t)(light.ShadowLayer + f));
				s_Data.ShadowShader->SetUniformMat4("u_LightSpaceMatrix", light.ShadowMatrices[f]);
				const Frustum faceFrustum(light.ShadowMatrices[f]);

				for (const DrawCommand& cmd : s_Data.DrawQueue)
				{
					if (!cmd.MaterialPtr->m_Lit)
						continue;

					if (s_Data.FrustumCulling && !faceFrustum.Intersects(cmd.Bounds))
						continue;

					s_Data.ShadowShader->SetUniformMat4("u_Transform", cmd.Transform);
					RenderCommand::DrawIndexed(cmd.MeshPtr->GetVertexArray(), cmd.IndexCount, cmd.BaseIndex);
					s_Data.Stats.DrawCalls++;
				}
			}
		}

		s_Data.LocalShadow->EndPass();
	}

	// ---------- Environnement (IBL) ----------

	void SceneRenderer::SetEnvironmentGradient(const glm::vec3& _zenith, const glm::vec3& _horizon, const glm::vec3& _ground, float _intensity)
	{
		s_Data.EnvOverride.Active = true;
		s_Data.EnvOverride.Zenith = _zenith;
		s_Data.EnvOverride.Horizon = _horizon;
		s_Data.EnvOverride.Ground = _ground;
		s_Data.EnvOverride.Intensity = _intensity;
	}

	// Choisit ce que "voit" l'éclairage : dégradé imposé > ciel de la scène > couleur ambiante uniforme (ancien comportement)
	void SceneRenderer::UpdateEnvironment()
	{
		if (!s_Data.Environment)
			return;

		EnvironmentMap::Settings settings;
		if (s_Data.EnvOverride.Active)
		{
			settings.Zenith = SRGBToLinear(s_Data.EnvOverride.Zenith);
			settings.Horizon = SRGBToLinear(s_Data.EnvOverride.Horizon);
			settings.Ground = SRGBToLinear(s_Data.EnvOverride.Ground);
			settings.Intensity = s_Data.EnvOverride.Intensity;
		}
		else if (s_Data.Sky.Enabled)
		{
			settings.Zenith = SRGBToLinear(s_Data.Sky.ZenithColor);
			settings.Horizon = SRGBToLinear(s_Data.Sky.HorizonColor);
			settings.Ground = SRGBToLinear(s_Data.Sky.GroundColor);
			settings.SunDirection = -s_Data.LightDirection; // jour / nuit / coucher comme le ciel affiché
			settings.Intensity = glm::max(s_Data.Sky.EnvironmentIntensity, 0.0f);
		}
		else
		{
			settings.Zenith = settings.Horizon = settings.Ground = s_Data.AmbientColor;
		}

		s_Data.Environment->Update(settings);
	}

	// ---------- Arêtes ----------

	void SceneRenderer::SetWireframe(bool _enabled, const glm::vec4& _color)
	{
		s_Data.WireframeEnabled = _enabled;
		s_Data.WireframeColor = _color;
	}

	bool SceneRenderer::IsWireframeEnabled()
	{
		return s_Data.WireframeEnabled;
	}

	void SceneRenderer::SuspendOverlays(bool _suspended)
	{
		s_Data.OverlaysSuspended = _suspended;
	}

	void SceneRenderer::DisableShadows()
	{
		s_Data.Shadows.Enabled = false;
	}

	void SceneRenderer::RenderWireframePass()
	{
		if (!s_Data.WireframeShader)
			return;

		OverlayRenderState state;
		state.Begin();
		RenderCommand::SetCullMode(RendererAPI::CullMode::None);
		RenderCommand::SetWireframe(true);

		Ref<Shader>& shader = s_Data.WireframeShader;
		shader->Bind();
		shader->SetUniformMat4("u_ViewProjection", s_Data.ViewProjection);
		shader->SetUniformFloat4("u_Color", s_Data.WireframeColor);

		const Frustum cameraFrustum(s_Data.ViewProjection);
		for (const DrawCommand& cmd : s_Data.DrawQueue)
		{
			// Les sprites / quads non éclairés n'ont pas d'arêtes à montrer
			if (!cmd.MaterialPtr->m_Lit)
				continue;

			if (s_Data.FrustumCulling && !cameraFrustum.Intersects(cmd.Bounds))
				continue;

			shader->SetUniformMat4("u_Transform", cmd.Transform);
			RenderCommand::DrawIndexed(cmd.MeshPtr->GetVertexArray(), cmd.IndexCount, cmd.BaseIndex);
			s_Data.Stats.DrawCalls++;
		}

		RenderCommand::SetWireframe(false);
		state.End();
	}

	// ---------- Grille infinie ----------

	void SceneRenderer::SetGridEnabled(bool _enabled)
	{
		s_Data.GridEnabled = _enabled;
	}

	bool SceneRenderer::IsGridEnabled()
	{
		return s_Data.GridEnabled;
	}

	void SceneRenderer::RenderGridPass()
	{
		if (!s_Data.GridShader)
			return;

		OverlayRenderState state;
		state.Begin();
		RenderCommand::SetCullMode(RendererAPI::CullMode::None);

		Ref<Shader>& shader = s_Data.GridShader;
		shader->Bind();
		shader->SetUniformMat4("u_ViewProjection", s_Data.ViewProjection);
		shader->SetUniformMat4("u_InverseViewProjection", glm::inverse(s_Data.ViewProjection));
		shader->SetUniformFloat3("u_CameraPosition", s_Data.CameraPosition);

		RenderCommand::DrawIndexed(s_Data.QuadMesh->GetVertexArray(), s_Data.QuadMesh->GetIndexCount());
		s_Data.Stats.DrawCalls++;

		state.End();
	}

	// ---------- Contour de sélection ----------

	void SceneRenderer::SetOutline(const std::vector<int>& _entityIDs, const glm::vec4& _color, float _width)
	{
		s_Data.OutlinedEntities = std::unordered_set<int>(_entityIDs.begin(), _entityIDs.end());
		s_Data.OutlineColor = _color;
		s_Data.OutlineWidth = glm::max(_width, 0.5f);
	}

	void SceneRenderer::RenderOutlinePass()
	{
		if (!s_Data.Outline || !s_Data.OutlineMaskShader || !s_Data.OutlineShader)
			return;

		// 1. Masque : les objets sélectionnés, en blanc, sans test de profondeur
		if (!s_Data.Outline->BeginMask())
			return;

		RenderCommand::SetCullMode(RendererAPI::CullMode::None);

		Ref<Shader>& mask = s_Data.OutlineMaskShader;
		mask->Bind();
		mask->SetUniformMat4("u_ViewProjection", s_Data.ViewProjection);
		mask->SetUniformInt("u_Texture", 0);

		bool anyDrawn = false;
		for (const DrawCommand& cmd : s_Data.DrawQueue)
		{
			if (!s_Data.OutlinedEntities.count(cmd.EntityID))
				continue;

			Material& mat = *cmd.MaterialPtr;
			mask->SetUniformMat4("u_Transform", cmd.Transform);
			mask->SetUniformFloat4("u_UVRect", cmd.UVRect);
			mask->SetUniformFloat2("u_Tiling", mat.m_Tiling * cmd.TilingMultiplier);
			mask->SetUniformFloat2("u_Offset", mat.m_Offset);
			mask->SetUniformFloat("u_Alpha", mat.m_AlbedoColor.a * cmd.Tint.a);
			mask->SetUniformFloat("u_AlphaCutoff", mat.m_AlphaCutoff);

			Ref<Texture2D> texture = cmd.TextureOverride ? cmd.TextureOverride : mat.GetAlbedoMap();
			(texture ? texture : s_Data.WhiteTexture)->Bind(0);

			RenderCommand::DrawIndexed(cmd.MeshPtr->GetVertexArray(), cmd.IndexCount, cmd.BaseIndex);
			s_Data.Stats.DrawCalls++;
			anyDrawn = true;
		}

		s_Data.Outline->EndMask();

		// Rien de visible à entourer (ex : entité vide sélectionnée)
		if (!anyDrawn)
			return;

		// 2. Contour : passe plein écran par-dessus l'image
		s_Data.Outline->BeginComposite();

		Ref<Shader>& outline = s_Data.OutlineShader;
		outline->Bind();
		outline->SetUniformInt("u_Mask", 0);
		outline->SetUniformFloat4("u_Color", s_Data.OutlineColor);
		outline->SetUniformFloat("u_Width", s_Data.OutlineWidth);
		s_Data.Outline->BindMaskTexture(0);

		RenderCommand::DrawIndexed(s_Data.QuadMesh->GetVertexArray(), s_Data.QuadMesh->GetIndexCount());
		s_Data.Stats.DrawCalls++;

		s_Data.Outline->EndComposite();
	}

	// ---------- Ciel ----------

	void SceneRenderer::SetSky(const SkySettings& _settings)
	{
		s_Data.Sky = _settings;
	}

	void SceneRenderer::SetColorSettings(const ColorSettings& _settings)
	{
		s_Data.Color = _settings;
		s_Data.Color.Exposure = glm::max(_settings.Exposure, 0.0f);
	}

	void SceneRenderer::RenderSkyPass()
	{
		if (!s_Data.SkyShader)
			return;

		// Vue sans translation : le ciel ne bouge pas quand la caméra se déplace, seulement quand elle tourne
		glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(s_Data.View));
		glm::mat4 invViewProj = glm::inverse(s_Data.Projection * viewNoTranslation);

		const SkySettings& sky = s_Data.Sky;
		Ref<Shader>& shader = s_Data.SkyShader;

		shader->Bind();
		shader->SetUniformMat4("u_InverseViewProjection", invViewProj);
		shader->SetUniformFloat3("u_SunDirection", -s_Data.LightDirection);
		shader->SetUniformFloat3("u_SunColor", s_Data.LightColor);
		shader->SetUniformFloat("u_SunSize", glm::radians(glm::max(sky.SunSize, 0.01f)));
		shader->SetUniformFloat("u_SunGlow", sky.SunGlow);
		shader->SetUniformFloat3("u_ZenithColor", SRGBToLinear(sky.ZenithColor));
		shader->SetUniformFloat3("u_HorizonColor", SRGBToLinear(sky.HorizonColor));
		shader->SetUniformFloat3("u_GroundColor", SRGBToLinear(sky.GroundColor));
		shader->SetUniformFloat("u_Exposure", sky.Exposure * s_Data.Color.Exposure);
		shader->SetUniformInt("u_ToneMapping", s_Data.Color.ToneMapping ? 1 : 0);

		RenderCommand::SetCullMode(RendererAPI::CullMode::None);
		RenderCommand::DrawIndexed(s_Data.QuadMesh->GetVertexArray(), s_Data.QuadMesh->GetIndexCount());
		s_Data.Stats.DrawCalls++;
	}

	// ---------- Lumière ----------

	void SceneRenderer::SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color, float _intensity, float _ambient)
	{
		// w = 0 : on ignore la translation
		glm::vec3 forward = glm::vec3(_transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
		SetDirectionalLightDirection(forward, _color, _intensity, _ambient);
	}

	void SceneRenderer::SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color, float _intensity, float _ambient)
	{
		s_Data.LightDirection = glm::length(_direction) > 0.0001f ? glm::normalize(_direction) : s_DefaultLightDirection;
		const glm::vec3 linearColor = SRGBToLinear(_color);
		s_Data.LightColor = linearColor * _intensity;
		s_Data.AmbientColor = linearColor * _ambient;
	}

	// ---------- Ombres ----------

	void SceneRenderer::SetShadowSettings(const ShadowSettings& _settings)
	{
		s_Data.Shadows = _settings;
		s_Data.Shadows.Distance = glm::max(_settings.Distance, 1.0f);
		s_Data.Shadows.Bias = glm::max(_settings.Bias, 0.0f);
		s_Data.Shadows.SplitLambda = glm::clamp(_settings.SplitLambda, 0.0f, 1.0f);

		if (s_Data.Shadow)
			s_Data.Shadow->Resize(_settings.Resolution);
	}

	// Découpe le champ de vision de la caméra en tranches de profondeur, et calcule pour chaque
	// tranche une projection orthographique de la lumière qui l'englobe.
	void SceneRenderer::ComputeCascades()
	{
		const glm::mat4 invProj = glm::inverse(s_Data.Projection);
		const glm::mat4 invView = glm::inverse(s_Data.View);

		// Coins du frustum de la caméra en espace vue (plan near et plan far)
		const glm::vec2 ndcCorners[4] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
		glm::vec3 nearCorners[4], farCorners[4];
		for (int i = 0; i < 4; i++)
		{
			glm::vec4 n = invProj * glm::vec4(ndcCorners[i], -1.0f, 1.0f);
			glm::vec4 f = invProj * glm::vec4(ndcCorners[i], 1.0f, 1.0f);
			nearCorners[i] = glm::vec3(n) / n.w;
			farCorners[i] = glm::vec3(f) / f.w;
		}

		const float cameraNear = glm::max(-nearCorners[0].z, 0.01f);
		const float cameraFar = -farCorners[0].z;
		const float shadowFar = glm::clamp(s_Data.Shadows.Distance, cameraNear + 0.1f, glm::max(cameraFar, cameraNear + 0.1f));

		// Distances de découpe : mélange logarithmique / linéaire ("practical split scheme").
		// Le log donne beaucoup de précision près de la caméra, le linéaire évite des cascades lointaines énormes.
		float splits[CascadeCount];
		for (uint32_t i = 0; i < CascadeCount; i++)
		{
			float p = (float)(i + 1) / (float)CascadeCount;
			float logSplit = cameraNear * std::pow(shadowFar / cameraNear, p);
			float linearSplit = cameraNear + (shadowFar - cameraNear) * p;
			splits[i] = glm::mix(linearSplit, logSplit, s_Data.Shadows.SplitLambda);
		}

		// Point sur l'arête near->far du coin _corner, à la profondeur (vue) _depth
		auto cornerAtDepth = [&](int _corner, float _depth) -> glm::vec3
		{
			float nearDepth = -nearCorners[_corner].z;
			float farDepth = -farCorners[_corner].z;
			float t = (_depth - nearDepth) / (farDepth - nearDepth);
			return glm::mix(nearCorners[_corner], farCorners[_corner], t);
		};

		const glm::vec3 dir = s_Data.LightDirection;
		const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), dir, up);
		const float resolution = (float)s_Data.Shadow->GetResolution();

		float sliceNear = cameraNear;
		for (uint32_t c = 0; c < CascadeCount; c++)
		{
			const float sliceFar = splits[c];

			// 8 coins de la tranche, en espace monde
			glm::vec3 corners[8];
			for (int i = 0; i < 4; i++)
			{
				corners[i] = glm::vec3(invView * glm::vec4(cornerAtDepth(i, sliceNear), 1.0f));
				corners[i + 4] = glm::vec3(invView * glm::vec4(cornerAtDepth(i, sliceFar), 1.0f));
			}

			// Sphère englobante : sa taille ne dépend pas de l'orientation de la caméra,
			// donc les ombres ne "respirent" pas quand on tourne la vue
			glm::vec3 center(0.0f);
			for (const glm::vec3& corner : corners)
				center += corner;
			center /= 8.0f;

			float radius = 0.0f;
			for (const glm::vec3& corner : corners)
				radius = glm::max(radius, glm::length(corner - center));
			radius = std::ceil(radius * 16.0f) / 16.0f;

			// Aligne le centre sur la grille des texels : évite le scintillement quand la caméra se déplace
			const float texelSize = (2.0f * radius) / resolution;
			glm::vec3 lightCenter = glm::vec3(lightView * glm::vec4(center, 1.0f));
			lightCenter.x = std::floor(lightCenter.x / texelSize) * texelSize;
			lightCenter.y = std::floor(lightCenter.y / texelSize) * texelSize;

			glm::mat4 lightProjection = glm::ortho(
				lightCenter.x - radius, lightCenter.x + radius,
				lightCenter.y - radius, lightCenter.y + radius,
				-lightCenter.z - radius - s_ShadowCasterMargin,
				-lightCenter.z + radius);

			s_Data.Cascades[c].LightSpaceMatrix = lightProjection * lightView;
			s_Data.Cascades[c].SplitFar = sliceFar;
			s_Data.Cascades[c].TexelSize = texelSize;

			sliceNear = sliceFar;
		}
	}

	void SceneRenderer::RenderShadowPass()
	{
		if (!s_Data.Shadow || !s_Data.ShadowShader)
			return;

		s_Data.Shadow->BeginPass();
		s_Data.ShadowShader->Bind();

		// Pas de culling dans la passe d'ombre : un plan fin projette une ombre quel que soit le côté éclairé
		RenderCommand::SetCullMode(RendererAPI::CullMode::None);

		for (uint32_t c = 0; c < CascadeCount; c++)
		{
			s_Data.Shadow->BindLayer(c);
			s_Data.ShadowShader->SetUniformMat4("u_LightSpaceMatrix", s_Data.Cascades[c].LightSpaceMatrix);

			// Volume couvert par cette cascade (inclut la marge côté lumière pour les objets hors champ qui projettent une ombre)
			const Frustum cascadeFrustum(s_Data.Cascades[c].LightSpaceMatrix);

			for (const DrawCommand& cmd : s_Data.DrawQueue)
			{
				// Seuls les objets éclairés projettent une ombre (pas les sprites / UI)
				if (!cmd.MaterialPtr->m_Lit)
					continue;

				if (s_Data.FrustumCulling && !cascadeFrustum.Intersects(cmd.Bounds))
					continue;

				s_Data.ShadowShader->SetUniformMat4("u_Transform", cmd.Transform);
				RenderCommand::DrawIndexed(cmd.MeshPtr->GetVertexArray(), cmd.IndexCount, cmd.BaseIndex);
				s_Data.Stats.DrawCalls++;
			}
		}

		s_Data.Shadow->EndPass();
	}

	void SceneRenderer::RenderMainPass()
	{
		s_Data.PreparedShaders.clear();

		if (s_Data.Shadow)
			s_Data.Shadow->BindTexture(s_ShadowMapSlot);

		if (s_Data.Environment)
		{
			s_Data.Environment->BindIrradiance(s_IrradianceSlot);
			s_Data.Environment->BindPrefiltered(s_PrefilterSlot);
		}

		if (s_Data.LocalShadow)
			s_Data.LocalShadow->BindTexture(s_LocalShadowSlot);

		RendererAPI::CullMode currentCull = RendererAPI::CullMode::None;
		RenderCommand::SetCullMode(currentCull);

		const Frustum cameraFrustum(s_Data.ViewProjection);

		for (const DrawCommand& cmd : s_Data.DrawQueue)
		{
			// Hors du champ de la caméra : rien à dessiner
			if (s_Data.FrustumCulling && !cameraFrustum.Intersects(cmd.Bounds))
			{
				s_Data.Stats.CulledCount++;
				continue;
			}

			Material& mat = *cmd.MaterialPtr;
			const Ref<Shader>& shader = mat.GetShader();
			if (!shader)
				continue;

			// Back-face culling selon le matériau.
			// Une échelle négative (miroir) inverse le sens des triangles : on cache alors les faces avant à la place.
			RendererAPI::CullMode cull = RendererAPI::CullMode::None;
			if (!mat.m_TwoSided)
				cull = glm::determinant(glm::mat3(cmd.Transform)) < 0.0f ? RendererAPI::CullMode::Front : RendererAPI::CullMode::Back;

			if (cull != currentCull)
			{
				RenderCommand::SetCullMode(cull);
				currentCull = cull;
			}

			PrepareShader(shader);

			shader->SetUniformMat4("u_Transform", cmd.Transform);
			shader->SetUniformFloat4("u_Color", mat.m_AlbedoColor * cmd.Tint);
			shader->SetUniformFloat4("u_UVRect", cmd.UVRect);
			shader->SetUniformFloat2("u_Tiling", mat.m_Tiling * cmd.TilingMultiplier);
			shader->SetUniformFloat2("u_Offset", mat.m_Offset);
			shader->SetUniformFloat("u_AlphaCutoff", mat.m_AlphaCutoff);
			shader->SetUniformInt("u_Lit", mat.m_Lit ? 1 : 0);
			shader->SetUniformInt("u_EntityID", cmd.EntityID);

			// ---------- PBR ----------
			const Ref<Texture2D>& normalMap = mat.GetMap(MaterialMap::Normal);
			const Ref<Texture2D>& heightMap = mat.GetMap(MaterialMap::Height);
			const Ref<Texture2D>& roughnessMap = mat.GetMap(MaterialMap::Roughness);

			shader->SetUniformFloat("u_Metallic", glm::clamp(mat.m_Metallic, 0.0f, 1.0f));
			shader->SetUniformFloat("u_Roughness", glm::clamp(mat.m_Roughness, 0.0f, 1.0f));
			shader->SetUniformInt("u_RoughnessIsSmoothness", (roughnessMap && mat.m_RoughnessIsSmoothness) ? 1 : 0);
			shader->SetUniformFloat4("u_MetallicChannel", ChannelMask(mat.m_MetallicChannel));
			shader->SetUniformFloat4("u_RoughnessChannel", ChannelMask(mat.m_RoughnessChannel));
			shader->SetUniformFloat4("u_OcclusionChannel", ChannelMask(mat.m_OcclusionChannel));
			shader->SetUniformFloat4("u_HeightChannel", ChannelMask(mat.m_HeightChannel));
			shader->SetUniformFloat("u_OcclusionStrength", glm::clamp(mat.m_OcclusionStrength, 0.0f, 1.0f));
			shader->SetUniformInt("u_HasNormalMap", normalMap ? 1 : 0);
			shader->SetUniformFloat("u_NormalStrength", mat.m_NormalStrength);
			shader->SetUniformInt("u_NormalFlipY", mat.m_NormalFlipY ? 1 : 0);
			shader->SetUniformInt("u_HasHeightMap", (heightMap && mat.m_HeightScale > 0.0f) ? 1 : 0);
			shader->SetUniformFloat("u_HeightScale", mat.m_HeightScale);
			shader->SetUniformFloat3("u_EmissionColor", SRGBToLinear(mat.m_EmissionColor) * mat.m_EmissionIntensity);

			// ---------- Textures (une unité par map, blanc = valeur neutre) ----------
			// Albedo : texture de l'instance (sprite) > texture du matériau > blanc
			Ref<Texture2D> albedo = cmd.TextureOverride ? cmd.TextureOverride : mat.GetMap(MaterialMap::Albedo);
			(albedo ? albedo : s_Data.WhiteTexture)->Bind((uint32_t)MaterialMap::Albedo);
			for (uint32_t slot = (uint32_t)MaterialMap::Albedo + 1; slot < Material::MapCount; slot++)
			{
				const Ref<Texture2D>& map = mat.GetMap((MaterialMap)slot);
				(map ? map : s_Data.WhiteTexture)->Bind(slot);
			}

			RenderCommand::DrawIndexed(cmd.MeshPtr->GetVertexArray(), cmd.IndexCount, cmd.BaseIndex);
			s_Data.Stats.DrawCalls++;
		}

		// Remet l'état par défaut pour le reste du moteur (ImGui, autres renderers…)
		RenderCommand::SetCullMode(RendererAPI::CullMode::None);
	}

	// Envoie les uniforms "de scène" (caméra, lumière, ombres) une seule fois par shader et par passe
	void SceneRenderer::PrepareShader(const Ref<Shader>& _shader)
	{
		_shader->Bind();

		if (s_Data.PreparedShaders.count(_shader.get()))
			return;

		_shader->SetUniformMat4("u_ViewProjection", s_Data.ViewProjection);
		_shader->SetUniformMat4("u_View", s_Data.View);
		_shader->SetUniformFloat3("u_CameraPosition", s_Data.CameraPosition);
		_shader->SetUniformFloat3("u_LightDirection", s_Data.LightDirection);
		_shader->SetUniformFloat3("u_LightColor", s_Data.LightColor);
		_shader->SetUniformFloat3("u_AmbientColor", s_Data.AmbientColor);
		_shader->SetUniformInt("u_Texture", 0); // ancien nom de l'albedo (shaders perso)
		_shader->SetUniformInt("u_AlbedoMap", (int)MaterialMap::Albedo);
		_shader->SetUniformInt("u_NormalMap", (int)MaterialMap::Normal);
		_shader->SetUniformInt("u_MetallicMap", (int)MaterialMap::Metallic);
		_shader->SetUniformInt("u_RoughnessMap", (int)MaterialMap::Roughness);
		_shader->SetUniformInt("u_OcclusionMap", (int)MaterialMap::Occlusion);
		_shader->SetUniformInt("u_HeightMap", (int)MaterialMap::Height);
		_shader->SetUniformInt("u_EmissionMap", (int)MaterialMap::Emission);

		_shader->SetUniformInt("u_IrradianceMap", (int)s_IrradianceSlot);
		_shader->SetUniformInt("u_PrefilterMap", (int)s_PrefilterSlot);
		_shader->SetUniformFloat("u_PrefilterMaxLod", s_Data.Environment ? s_Data.Environment->GetPrefilterMaxLod() : 0.0f);

		// Lumières locales : 4 vec4 par lumière
		const int lightCount = (int)s_Data.Lights.size();
		_shader->SetUniformInt("u_LightCount", lightCount);
		for (int i = 0; i < lightCount; i++)
		{
			const std::string index = "[" + std::to_string(i) + "]";
			const LocalLight& light = s_Data.Lights[i];
			_shader->SetUniformFloat4("u_LightsPositionRange" + index, light.PositionRange);
			_shader->SetUniformFloat4("u_LightsColorType" + index, light.ColorType);
			_shader->SetUniformFloat4("u_LightsDirection" + index, light.Direction);
			_shader->SetUniformFloat4("u_LightsSpot" + index, light.Spot);

			if (light.ShadowLayer >= 0)
			{
				const int faces = light.IsSpot ? 1 : 6;
				for (int f = 0; f < faces; f++)
					_shader->SetUniformMat4("u_LocalShadowMatrices[" + std::to_string(light.ShadowLayer + f) + "]", light.ShadowMatrices[f]);
			}
		}
		_shader->SetUniformInt("u_LocalShadowMap", (int)s_LocalShadowSlot);

		_shader->SetUniformFloat("u_Exposure", s_Data.Color.Exposure);
		_shader->SetUniformInt("u_ToneMapping", s_Data.Color.ToneMapping ? 1 : 0);

		const bool shadows = s_Data.Shadows.Enabled && s_Data.Shadow;
		_shader->SetUniformInt("u_ShadowMap", (int)s_ShadowMapSlot);
		_shader->SetUniformInt("u_ShadowsEnabled", shadows ? 1 : 0);
		_shader->SetUniformFloat("u_ShadowBias", s_Data.Shadows.Bias);
		_shader->SetUniformInt("u_ShowCascades", (shadows && s_Data.Shadows.ShowCascades) ? 1 : 0);

		glm::vec4 splits(0.0f), texelSizes(0.0f);
		for (uint32_t c = 0; c < CascadeCount; c++)
		{
			_shader->SetUniformMat4("u_LightSpaceMatrices[" + std::to_string(c) + "]", s_Data.Cascades[c].LightSpaceMatrix);
			splits[c] = s_Data.Cascades[c].SplitFar;
			texelSizes[c] = s_Data.Cascades[c].TexelSize;
		}
		_shader->SetUniformFloat4("u_CascadeSplits", splits);
		_shader->SetUniformFloat4("u_CascadeTexelSizes", texelSizes);

		s_Data.PreparedShaders.insert(_shader.get());
	}

	// ---------- File de draws ----------

	void SceneRenderer::Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material,
		const Ref<Texture2D>& _textureOverride, const glm::vec4& _tint, float _tilingMultiplier,
		const glm::vec4& _uvRect, int _entityID, const SubMesh* _subMesh)
	{
		if (!_mesh)
			return;

		DrawCommand cmd;
		cmd.MeshPtr = _mesh;
		cmd.Transform = _transform;
		cmd.MaterialPtr = _material ? _material : Material::GetDefault();
		cmd.TextureOverride = _textureOverride;
		cmd.Tint = _tint;
		cmd.TilingMultiplier = _tilingMultiplier;
		cmd.UVRect = _uvRect;
		cmd.EntityID = _entityID;
		cmd.BaseIndex = _subMesh ? _subMesh->BaseIndex : 0;
		cmd.IndexCount = _subMesh ? _subMesh->IndexCount : _mesh->GetIndexCount();
		cmd.Bounds = WorldBounds::FromLocal(_subMesh ? _subMesh->Bounds : _mesh->GetBounds(), _transform);

		s_Data.DrawQueue.push_back(std::move(cmd));
	}

	// ---------- Components ----------

	void SceneRenderer::DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, int _entityID)
	{
		// Chargement paresseux (ex : après désérialisation où seul le chemin est connu)
		if (!_src.m_Material && !_src.m_MaterialPath.empty())
			_src.m_Material = Material::Load(_src.m_MaterialPath);

		const Ref<Material>& mat = _src.m_Material ? _src.m_Material : Material::GetDefaultUnlit();

		if (_src.m_Texture)
			Submit(s_Data.QuadMesh, _transform, mat, _src.m_Texture, _src.m_Color, 1.f,
				UVRectFromCoords(_src.m_Texture->GetTextureCoords()), _entityID);
		else
			Submit(s_Data.QuadMesh, _transform, mat, nullptr, _src.m_Color, 1.f, s_FullUV, _entityID);

		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawMeshRenderer(const glm::mat4& _transform, MeshRenderer& _src, int _entityID)
	{
		if (!_src.m_Mesh && !_src.m_MeshPath.empty())
			_src.SetMesh(_src.m_MeshPath);

		if (!_src.m_Mesh)
			return;

		for (const SubMesh& sub : _src.m_Mesh->GetSubMeshes())
		{
			// Chargement paresseux (scène désérialisée : seul le chemin est connu)
			const Ref<Material>& mat = _src.GetMaterial(sub.MaterialIndex);
			Submit(_src.m_Mesh, _transform, mat ? mat : Material::GetDefault(), nullptr, _src.m_Color, 1.0f, s_FullUV, _entityID, &sub);
		}
		s_Data.Stats.MeshCount++;
	}

	// ---------- Mesh / primitives 3D ----------

	void SceneRenderer::DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		if (!_mesh)
			return;

		const Ref<Material>& mat = _material ? _material : Material::GetDefault();
		for (const SubMesh& sub : _mesh->GetSubMeshes())
			Submit(_mesh, _transform, mat, nullptr, _tint, 1.0f, s_FullUV, _entityID, &sub);
		s_Data.Stats.MeshCount++;
	}

	void SceneRenderer::DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const std::vector<Ref<Material>>& _materials, const glm::vec4& _tint, int _entityID)
	{
		if (!_mesh)
			return;

		for (const SubMesh& sub : _mesh->GetSubMeshes())
		{
			const bool hasMaterial = sub.MaterialIndex < _materials.size() && _materials[sub.MaterialIndex];
			Submit(_mesh, _transform, hasMaterial ? _materials[sub.MaterialIndex] : Material::GetDefault(),
				nullptr, _tint, 1.0f, s_FullUV, _entityID, &sub);
		}
		s_Data.Stats.MeshCount++;
	}

	void SceneRenderer::DrawPlane(const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		DrawMesh(s_Data.PlaneMesh, _transform, _material, _tint, _entityID);
	}

	void SceneRenderer::DrawCube(const glm::mat4& _transform, const Ref<Material>& _material, const glm::vec4& _tint, int _entityID)
	{
		DrawMesh(s_Data.CubeMesh, _transform, _material, _tint, _entityID);
	}

	void SceneRenderer::DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color)
	{
		DrawCube(MakeTransform(_pos, _size), nullptr, _color);
	}

	// ---------- Quads 2D ----------

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4& _color)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _color);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _color);
	}

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad({ _pos.x, _pos.y, 1.0f }, _size, _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling)
	{
		DrawQuad(MakeTransform(_pos, { _size.x, _size.y, 1.0f }), _texture, _color, _tiling);
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const glm::vec4& _color, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), nullptr, _color, 1.0f, s_FullUV, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), _texture, _color, _tiling, s_FullUV, _entityID);
		s_Data.Stats.QuadCount++;
	}

	void SceneRenderer::DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color, float _tiling, int _entityID)
	{
		Submit(s_Data.QuadMesh, _transform, Material::GetDefaultUnlit(), _texture->GetTexture(), _color, _tiling,
			UVRectFromCoords(_texture->GetTextureCoords()), _entityID);
		s_Data.Stats.QuadCount++;
	}

	// ---------- Stats ----------

	SceneRenderer::Statistics SceneRenderer::GetStats()
	{
		return s_Data.Stats;
	}

	void SceneRenderer::ResetStats()
	{
		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.QuadCount = 0;
		s_Data.Stats.MeshCount = 0;
		s_Data.Stats.CulledCount = 0;
	}

	void SceneRenderer::SetFrustumCulling(bool _enabled)
	{
		s_Data.FrustumCulling = _enabled;
	}

	bool SceneRenderer::IsFrustumCullingEnabled()
	{
		return s_Data.FrustumCulling;
	}
}
