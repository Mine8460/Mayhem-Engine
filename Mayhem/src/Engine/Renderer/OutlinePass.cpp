#include "Enpch.h"
#include "OutlinePass.h"

#include <glad/glad.h>

namespace Mayhem
{
	OutlinePass::~OutlinePass()
	{
		Release();
	}

	void OutlinePass::Release()
	{
		if (m_FBO)
			glDeleteFramebuffers(1, &m_FBO);
		if (m_MaskTexture)
			glDeleteTextures(1, &m_MaskTexture);

		m_FBO = 0;
		m_MaskTexture = 0;
		m_Width = 0;
		m_Height = 0;
	}

	void OutlinePass::Resize(uint32_t _width, uint32_t _height)
	{
		Release();

		m_Width = _width;
		m_Height = _height;

		// Un seul canal : 1 = objet sélectionné, 0 = rien
		glCreateTextures(GL_TEXTURE_2D, 1, &m_MaskTexture);
		glTextureStorage2D(m_MaskTexture, 1, GL_R8, m_Width, m_Height);
		glTextureParameteri(m_MaskTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_MaskTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(m_MaskTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_MaskTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glCreateFramebuffers(1, &m_FBO);
		glNamedFramebufferTexture(m_FBO, GL_COLOR_ATTACHMENT0, m_MaskTexture, 0);
		glNamedFramebufferDrawBuffer(m_FBO, GL_COLOR_ATTACHMENT0);
	}

	bool OutlinePass::BeginMask()
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PrevFBO);
		glGetIntegerv(GL_VIEWPORT, m_PrevViewport);

		const uint32_t width = (uint32_t)m_PrevViewport[2];
		const uint32_t height = (uint32_t)m_PrevViewport[3];
		if (width == 0 || height == 0)
			return false;

		if (width != m_Width || height != m_Height)
			Resize(width, height);

		m_DepthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
		glViewport(0, 0, m_Width, m_Height);

		const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearNamedFramebufferfv(m_FBO, GL_COLOR, 0, zero); // ne modifie pas la couleur de clear globale

		// Pas de test de profondeur : le contour reste visible à travers les autres objets
		glDisable(GL_DEPTH_TEST);
		return true;
	}

	void OutlinePass::EndMask()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)m_PrevFBO);
		glViewport(m_PrevViewport[0], m_PrevViewport[1], m_PrevViewport[2], m_PrevViewport[3]);

		if (m_DepthTestWasEnabled)
			glEnable(GL_DEPTH_TEST);
	}

	void OutlinePass::BeginComposite()
	{
		m_DepthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
		m_BlendWasEnabled = glIsEnabled(GL_BLEND);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// L'attachement 1 contient les entity IDs (sélection à la souris) : on ne l'écrase pas
		glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	void OutlinePass::EndComposite()
	{
		glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		if (!m_BlendWasEnabled)
			glDisable(GL_BLEND);
		if (m_DepthTestWasEnabled)
			glEnable(GL_DEPTH_TEST);
	}

	void OutlinePass::BindMaskTexture(uint32_t _slot) const
	{
		glBindTextureUnit(_slot, m_MaskTexture);
	}
}
