#include "Enpch.h"
#include "OverlayRenderState.h"

#include <glad/glad.h>

namespace Mayhem
{
	void OverlayRenderState::Begin()
	{
		m_BlendWasEnabled = glIsEnabled(GL_BLEND);
		m_DepthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

		GLboolean depthMask = GL_TRUE;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
		m_DepthMaskWas = depthMask;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);   // caché par les objets devant
		glDepthMask(GL_FALSE);     // mais n'écrit pas la profondeur

		glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // entity IDs intacts
	}

	void OverlayRenderState::End()
	{
		glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(m_DepthMaskWas ? GL_TRUE : GL_FALSE);

		if (!m_DepthTestWasEnabled)
			glDisable(GL_DEPTH_TEST);
		if (!m_BlendWasEnabled)
			glDisable(GL_BLEND);
	}
}
