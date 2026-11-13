#include "Enpch.h"
#include "EnvironmentMap.h"

#include "RenderCommand.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Mayhem
{
	static constexpr uint32_t s_EnvironmentSize = 128;
	static constexpr uint32_t s_IrradianceSize = 32;
	static constexpr uint32_t s_PrefilterSize = 128;
	static constexpr uint32_t s_PrefilterLevels = 5;

	static uint32_t CreateCubemap(uint32_t _size, uint32_t _levels)
	{
		uint32_t id = 0;
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
		glTextureStorage2D(id, _levels, GL_RGBA16F, _size, _size);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, _levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		return id;
	}

	// Orientation de chaque face d'une cubemap (convention OpenGL : +X, -X, +Y, -Y, +Z, -Z)
	static glm::mat4 FaceInverseView(uint32_t _face)
	{
		static const glm::vec3 targets[6] = {
			{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
		};
		static const glm::vec3 ups[6] = {
			{ 0, -1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, -1, 0 }, { 0, -1, 0 }
		};
		return glm::inverse(glm::lookAt(glm::vec3(0.0f), targets[_face], ups[_face]));
	}

	EnvironmentMap::EnvironmentMap()
	{
		m_SkyShader = Shader::Create("assets/shaders/EnvSky.glsl");
		m_IrradianceShader = Shader::Create("assets/shaders/EnvIrradiance.glsl");
		m_PrefilterShader = Shader::Create("assets/shaders/EnvPrefilter.glsl");
		m_Quad = Mesh::CreateQuad();

		m_EnvironmentCube = CreateCubemap(s_EnvironmentSize, 5); // mips : sert de source aux deux convolutions
		m_IrradianceCube = CreateCubemap(s_IrradianceSize, 1);
		m_PrefilterCube = CreateCubemap(s_PrefilterSize, s_PrefilterLevels);

		glCreateFramebuffers(1, &m_FBO);
	}

	EnvironmentMap::~EnvironmentMap()
	{
		Release();
	}

	void EnvironmentMap::Release()
	{
		if (m_FBO) glDeleteFramebuffers(1, &m_FBO);
		if (m_EnvironmentCube) glDeleteTextures(1, &m_EnvironmentCube);
		if (m_IrradianceCube) glDeleteTextures(1, &m_IrradianceCube);
		if (m_PrefilterCube) glDeleteTextures(1, &m_PrefilterCube);
		m_FBO = m_EnvironmentCube = m_IrradianceCube = m_PrefilterCube = 0;
	}

	void EnvironmentMap::BindIrradiance(uint32_t _slot) const
	{
		glBindTextureUnit(_slot, m_IrradianceCube);
	}

	void EnvironmentMap::BindPrefiltered(uint32_t _slot) const
	{
		glBindTextureUnit(_slot, m_PrefilterCube);
	}

	float EnvironmentMap::GetPrefilterMaxLod() const
	{
		return (float)(s_PrefilterLevels - 1);
	}

	void EnvironmentMap::Update(const Settings& _settings)
	{
		if (m_Valid && _settings == m_Settings)
			return;
		if (!m_SkyShader || !m_IrradianceShader || !m_PrefilterShader)
			return;

		m_Settings = _settings;
		m_Valid = true;

		// ---------- Sauvegarde de l'état GL ----------
		GLint previousFBO = 0;
		GLint previousViewport[4] = { 0, 0, 0, 0 };
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);
		glGetIntegerv(GL_VIEWPORT, previousViewport);
		const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
		const GLboolean blend = glIsEnabled(GL_BLEND);
		const GLboolean cull = glIsEnabled(GL_CULL_FACE);
		const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // pas de couture visible entre les faces

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

		auto drawFace = [&](const Ref<Shader>& _shader, uint32_t _cube, uint32_t _mip, uint32_t _size, uint32_t _face)
		{
			glNamedFramebufferTextureLayer(m_FBO, GL_COLOR_ATTACHMENT0, _cube, _mip, _face);
			glViewport(0, 0, _size, _size);
			_shader->SetUniformMat4("u_InverseView", FaceInverseView(_face));
			RenderCommand::DrawIndexed(m_Quad->GetVertexArray(), m_Quad->GetIndexCount());
		};

		// ---------- 1. Le ciel dans une cubemap ----------
		m_SkyShader->Bind();
		m_SkyShader->SetUniformFloat3("u_SunDirection", _settings.SunDirection);
		m_SkyShader->SetUniformFloat3("u_ZenithColor", _settings.Zenith);
		m_SkyShader->SetUniformFloat3("u_HorizonColor", _settings.Horizon);
		m_SkyShader->SetUniformFloat3("u_GroundColor", _settings.Ground);
		m_SkyShader->SetUniformFloat("u_Intensity", _settings.Intensity);
		for (uint32_t face = 0; face < 6; face++)
			drawFace(m_SkyShader, m_EnvironmentCube, 0, s_EnvironmentSize, face);
		glGenerateTextureMipmap(m_EnvironmentCube);

		// ---------- 2. Irradiance (éclairage diffus) ----------
		glBindTextureUnit(0, m_EnvironmentCube);
		m_IrradianceShader->Bind();
		m_IrradianceShader->SetUniformInt("u_EnvironmentMap", 0);
		for (uint32_t face = 0; face < 6; face++)
			drawFace(m_IrradianceShader, m_IrradianceCube, 0, s_IrradianceSize, face);

		// ---------- 3. Reflets pré-filtrés, une mip par rugosité ----------
		m_PrefilterShader->Bind();
		m_PrefilterShader->SetUniformInt("u_EnvironmentMap", 0);
		m_PrefilterShader->SetUniformFloat("u_SourceResolution", (float)s_EnvironmentSize);
		for (uint32_t mip = 0; mip < s_PrefilterLevels; mip++)
		{
			m_PrefilterShader->SetUniformFloat("u_Roughness", (float)mip / (float)(s_PrefilterLevels - 1));
			const uint32_t size = s_PrefilterSize >> mip;
			for (uint32_t face = 0; face < 6; face++)
				drawFace(m_PrefilterShader, m_PrefilterCube, mip, size, face);
		}

		// ---------- Restauration ----------
		glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)previousFBO);
		glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
		if (depthTest) glEnable(GL_DEPTH_TEST);
		if (blend) glEnable(GL_BLEND);
		if (cull) glEnable(GL_CULL_FACE);
		if (scissor) glEnable(GL_SCISSOR_TEST);
	}
}
