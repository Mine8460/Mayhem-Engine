#pragma once

#include "Camera/OrthographicCamera.h"
#include "Camera/EditorCamera.h"
#include "Texture.h"
#include "SubTexture2D.h"
#include "Mesh.h"
#include "Material.h"
#include "ShadowMap.h"
#include "EnvironmentMap.h"
#include "OutlinePass.h"
#include "OverlayRenderState.h"

#include <glm/glm.hpp>
#include <vector>

namespace Mayhem
{
	struct SpriteRenderer;
	struct MeshRenderer;

	class SceneRenderer
	{
	public:
		static constexpr uint32_t CascadeCount = 4; // doit correspondre à CASCADE_COUNT dans Standard.glsl
		static constexpr uint32_t MaxLights = 32;   // point lights + spot lights ; doit correspondre à MAX_LIGHTS dans Standard.glsl
		static constexpr uint32_t LocalShadowLayers = 16; // faces d'ombre pour les lumières locales : un spot en prend 1, un point 6 ; doit correspondre à LOCAL_SHADOW_LAYERS dans Standard.glsl
		static constexpr uint32_t LocalShadowResolution = 1024;

		struct ShadowSettings
		{
			bool Enabled = true;
			float Distance = 100.0f;    // distance max (depuis la caméra) des ombres, en mètres
			float Bias = 0.002f;
			uint32_t Resolution = 2048; // par cascade
			float SplitLambda = 0.75f;  // 0 = cascades de même taille, 1 = très resserrées près de la caméra
			bool ShowCascades = false;  // debug : colore chaque cascade
		};

		struct SkySettings
		{
			bool Enabled = false;
			glm::vec3 ZenithColor = { 0.25f, 0.45f, 0.85f };
			glm::vec3 HorizonColor = { 0.70f, 0.80f, 0.95f };
			glm::vec3 GroundColor = { 0.35f, 0.32f, 0.30f };
			float SunSize = 1.5f;   // rayon apparent du soleil, en degrés
			float SunGlow = 0.6f;   // intensité du halo autour du soleil
			float Exposure = 1.0f;
			float EnvironmentIntensity = 1.0f; // force de l'éclairage ambiant et des reflets venant du ciel (IBL)
		};

		struct ColorSettings
		{
			bool ToneMapping = true; // ACES : compresse les fortes lumières au lieu de les écrêter en blanc
			float Exposure = 1.0f;   // luminosité globale de la scène éclairée
		};

		static void Init();
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& _cam);
		static void BeginScene(const EditorCamera& _cam);
		static void BeginScene(const Camera& _cam, const glm::mat4& _transform);
		// Les draws sont mis en file : c'est EndScene qui rend les passes d'ombre puis la passe principale
		static void EndScene();

		// ---------- Lumière ----------
		// À appeler entre BeginScene et EndScene (BeginScene remet la lumière par défaut).
		// Direction = axe -Z local de la transform, seule la rotation compte.
		static void SetDirectionalLight(const glm::mat4& _transform, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);
		static void SetDirectionalLightDirection(const glm::vec3& _direction, const glm::vec3& _color = glm::vec3(1.0f), float _intensity = 1.0f, float _ambient = 0.2f);

		// ---------- Lumières locales ----------
		// À appeler entre BeginScene et EndScene (BeginScene vide la liste).
		// Intensité 1 à 1 m = même éclairement qu'une lumière directionnelle d'intensité 1 ; décroît avec le carré de la distance
		// et s'éteint doucement à _range (mètres). Au-delà de MaxLights, les plus proches de la caméra sont gardées.
		// Ombres : _castShadows demande une ombre, accordée aux lumières les plus proches de la caméra tant qu'il reste
		// des faces (LocalShadowLayers). _shadowBias en mètres.
		static void AddPointLight(const glm::vec3& _position, const glm::vec3& _color, float _intensity, float _range,
			bool _castShadows = false, float _shadowBias = 0.05f);
		// _direction = sens dans lequel le cône éclaire. Angles TOTAUX du cône, en degrés.
		static void AddSpotLight(const glm::vec3& _position, const glm::vec3& _direction, const glm::vec3& _color, float _intensity,
			float _range, float _innerAngle, float _outerAngle, bool _castShadows = false, float _shadowBias = 0.05f);

		// ---------- Ombres ----------
		// À appeler entre BeginScene et EndScene (BeginScene remet les valeurs par défaut).
		static void SetShadowSettings(const ShadowSettings& _settings);

		// ---------- Ciel ----------
		// À appeler entre BeginScene et EndScene (BeginScene désactive le ciel).
		// Le soleil suit automatiquement la direction de la lumière directionnelle.
		static void SetSky(const SkySettings& _settings);

		// ---------- Couleur ----------
		// À appeler entre BeginScene et EndScene (BeginScene remet les valeurs par défaut).
		static void SetColorSettings(const ColorSettings& _settings);

		// ---------- Environnement (IBL) ----------
		// Éclairage ambiant diffus + reflets. Par défaut : le ciel s'il est activé, sinon la couleur ambiante de la lumière.
		// Cet appel impose un dégradé à la place (miniatures…). À appeler entre BeginScene et EndScene.
		// Couleurs en sRGB.
		static void SetEnvironmentGradient(const glm::vec3& _zenith, const glm::vec3& _horizon, const glm::vec3& _ground, float _intensity = 1.0f);

		// ---------- Contour de sélection (éditeur) ----------
		// Liste des entity IDs à entourer (vide = pas de contour). Persiste d'une frame à l'autre :
		// l'éditeur la met à jour à chaque frame.
		static void SetOutline(const std::vector<int>& _entityIDs,
			const glm::vec4& _color = glm::vec4(1.0f, 0.55f, 0.1f, 1.0f), float _width = 2.0f);

		// ---------- Grille infinie (éditeur) ----------
		// Persiste d'une frame à l'autre : l'éditeur l'active en édition, la coupe en Play.
		static void SetGridEnabled(bool _enabled);
		static bool IsGridEnabled();

		// ---------- Arêtes des modèles (éditeur) ----------
		// Dessine les triangles en lignes par-dessus les objets éclairés (pas les sprites).
		// Persiste d'une frame à l'autre, comme la grille.
		static void SetWireframe(bool _enabled, const glm::vec4& _color = glm::vec4(0.55f, 0.85f, 1.0f, 0.55f));
		static bool IsWireframeEnabled();

		// ---------- Rendus hors scène (miniatures…) ----------
		// Coupe grille, contour et arêtes pour les EndScene suivants (à remettre à false ensuite)
		static void SuspendOverlays(bool _suspended);
		// Pas d'ombres pour la scène en cours (à appeler entre BeginScene et EndScene)
		static void DisableShadows();

		// ---------- Components ----------
		static void DrawSprite(const glm::mat4& _transform, SpriteRenderer& _src, int _entityID = -1);
		static void DrawMeshRenderer(const glm::mat4& _transform, MeshRenderer& _src, int _entityID = -1);

		// ---------- Mesh / primitives 3D (material nullptr = matériau par défaut) ----------
		// Un seul matériau pour tous les sous-meshes
		static void DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		// Un matériau par slot (_materials[slot], nullptr ou absent = matériau par défaut)
		static void DrawMesh(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const std::vector<Ref<Material>>& _materials, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawPlane(const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawCube(const glm::mat4& _transform, const Ref<Material>& _material = nullptr, const glm::vec4& _tint = glm::vec4(1.0f), int _entityID = -1);
		static void DrawCube(const glm::vec3& _pos, const glm::vec3& _size, const glm::vec4& _color);

		// ---------- Quads 2D (matériau non éclairé par défaut) ----------
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const glm::vec4& _color);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec2& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::vec3& _pos, const glm::vec2& _size, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f);
		static void DrawQuad(const glm::mat4& _transform, const glm::vec4& _color, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<Texture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);
		static void DrawQuad(const glm::mat4& _transform, const Ref<SubTexture2D>& _texture, const glm::vec4& _color = glm::vec4(1.0f), float _tiling = 1.0f, int _entityID = -1);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
			uint32_t MeshCount = 0;
			uint32_t CulledCount = 0; // objets ignorés car hors du champ de la caméra
		};

		static Statistics GetStats();
		static void ResetStats();

		// Activé par défaut. Le désactiver sert à comparer les performances / déboguer.
		static void SetFrustumCulling(bool _enabled);
		static bool IsFrustumCullingEnabled();

	private:
		static void SetCamera(const glm::mat4& _view, const glm::mat4& _projection);
		static void PrepareShader(const Ref<Shader>& _shader);

		static void ComputeCascades();
		static void RenderShadowPass();
		static void RenderMainPass();
		static void RenderSkyPass();
		static void RenderOutlinePass();
		static void UpdateEnvironment();
		static void SelectLights();
		static void RenderLocalShadowPass();
		static void RenderGridPass();
		static void RenderWireframePass();

		// Point de passage unique de tous les draws (ajoute à la file)
		// _subMesh nullptr = le mesh entier
		static void Submit(const Ref<Mesh>& _mesh, const glm::mat4& _transform, const Ref<Material>& _material,
			const Ref<Texture2D>& _textureOverride, const glm::vec4& _tint, float _tilingMultiplier,
			const glm::vec4& _uvRect, int _entityID, const SubMesh* _subMesh = nullptr);
	};
}
