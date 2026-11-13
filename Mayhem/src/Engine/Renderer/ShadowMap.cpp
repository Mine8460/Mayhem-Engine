#include "Enpch.h"
#include "ShadowMap.h"

#include <glad/glad.h>

namespace Mayhem
{
	ShadowMap::ShadowMap(uint32_t _resolution, uint32_t _layerCount)
		: m_Resolution(_resolution), m_LayerCount(_layerCount)
	{
		Invalidate();
	}

	ShadowMap::~ShadowMap()
	{
		Release();
	}

	void ShadowMap::Release()
	{
		if (m_FBO)
			glDeleteFramebuffers(1, &m_FBO);
		if (m_DepthTexture)
			glDeleteTextures(1, &m_DepthTexture);

		m_FBO = 0;
		m_DepthTexture = 0;
	}

	void ShadowMap::Invalidate()
	{
		Release();

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_DepthTexture);
		glTextureStorage3D(m_DepthTexture, 1, GL_DEPTH_COMPONENT32F, m_Resolution, m_Resolution, m_LayerCount);

		// LINEAR + mode comparaison = filtrage PCF matériel (bords d'ombre adoucis)
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		// Hors de la shadow map = profondeur max = toujours éclairé
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(m_DepthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTextureParameterfv(m_DepthTexture, GL_TEXTURE_BORDER_COLOR, border);

		glCreateFramebuffers(1, &m_FBO);
		glNamedFramebufferTextureLayer(m_FBO, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0, 0);
		glNamedFramebufferDrawBuffer(m_FBO, GL_NONE); // pas de couleur
		glNamedFramebufferReadBuffer(m_FBO, GL_NONE);
	}

	void ShadowMap::Resize(uint32_t _resolution)
	{
		if (_resolution == 0 || _resolution == m_Resolution)
			return;

		m_Resolution = _resolution;
		Invalidate();
	}

	void ShadowMap::BeginPass()
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PrevFBO);
		glGetIntegerv(GL_VIEWPORT, m_PrevViewport);

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
		glViewport(0, 0, m_Resolution, m_Resolution);
		glDepthMask(GL_TRUE);

		// Décale la profondeur écrite pour limiter le "shadow acne" (rayures sur les surfaces éclairées)
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(2.0f, 4.0f);
	}

	void ShadowMap::BindLayer(uint32_t _layer)
	{
		glNamedFramebufferTextureLayer(m_FBO, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0, (GLint)_layer);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void ShadowMap::EndPass()
	{
		glDisable(GL_POLYGON_OFFSET_FILL);

		glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)m_PrevFBO);
		glViewport(m_PrevViewport[0], m_PrevViewport[1], m_PrevViewport[2], m_PrevViewport[3]);
	}

	void ShadowMap::BindTexture(uint32_t _slot) const
	{
		glBindTextureUnit(_slot, m_DepthTexture);
	}
}
