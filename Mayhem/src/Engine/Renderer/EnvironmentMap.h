#pragma once

#include "Shader.h"
#include "Mesh.h"

#include <glm/glm.hpp>
#include <cstdint>

namespace Mayhem
{
	// Éclairage par l'environnement (IBL) : transforme un ciel en deux cubemaps pré-calculées
	//  - irradiance : la lumière ambiante diffuse reçue dans chaque direction (32x32)
	//  - pré-filtrée : les reflets, une mip par niveau de rugosité (128x128, 5 mips)
	// Les cubemaps ne sont recalculées que quand les réglages changent.
	// Implémentation OpenGL (comme ShadowMap).
	class EnvironmentMap
	{
	public:
		// Tout en radiance linéaire
		struct Settings
		{
			glm::vec3 Zenith = glm::vec3(0.1f);
			glm::vec3 Horizon = glm::vec3(0.1f);
			glm::vec3 Ground = glm::vec3(0.1f);
			glm::vec3 SunDirection = glm::vec3(0.0f, 1.0f, 0.0f); // vers le soleil : gère jour / nuit / coucher
			float Intensity = 1.0f;

			bool operator==(const Settings& _o) const
			{
				return Zenith == _o.Zenith && Horizon == _o.Horizon && Ground == _o.Ground
					&& SunDirection == _o.SunDirection && Intensity == _o.Intensity;
			}
		};

		EnvironmentMap();
		~EnvironmentMap();

		EnvironmentMap(const EnvironmentMap&) = delete;
		EnvironmentMap& operator=(const EnvironmentMap&) = delete;

		// Régénère les cubemaps si _settings a changé depuis le dernier appel.
		// Sauvegarde et restaure framebuffer, viewport et états GL : peut être appelé à tout moment hors d'un draw.
		void Update(const Settings& _settings);

		void BindIrradiance(uint32_t _slot) const;
		void BindPrefiltered(uint32_t _slot) const;

		// Dernière mip de la cubemap des reflets (rugosité 1)
		float GetPrefilterMaxLod() const;

	private:
		void Release();

	private:
		uint32_t m_FBO = 0;
		uint32_t m_EnvironmentCube = 0;
		uint32_t m_IrradianceCube = 0;
		uint32_t m_PrefilterCube = 0;

		Ref<Shader> m_SkyShader;
		Ref<Shader> m_IrradianceShader;
		Ref<Shader> m_PrefilterShader;
		Ref<Mesh> m_Quad;

		Settings m_Settings;
		bool m_Valid = false;
	};
}
