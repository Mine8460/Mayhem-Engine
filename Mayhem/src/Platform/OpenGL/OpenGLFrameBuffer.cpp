#include "Enpch.h"
#include "OpenGLFrameBuffer.h"
#include <glad/glad.h>

namespace Mayhem
{
	static const uint32_t s_MaxFrameBufferSize = 8192;

	namespace Utils
	{
		static GLenum TextureTarget(bool _multisampled)
		{
			return _multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool _multisampled, uint32_t* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(_multisampled), count, outID);
		}

		static void BindTexture(bool _multisampled, uint32_t _id)
		{
			glBindTexture(TextureTarget(_multisampled), _id);
		}

		static void AttachColorTexture(uint32_t _id, int _samples, GLenum _internalFormat, GLenum _format, uint32_t _w, uint32_t _h, int _i)
		{
			bool multisampled = _samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _samples, _internalFormat, _w, _h, GL_FALSE);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, _w, _h, 0, _format, GL_UNSIGNED_BYTE, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _i, TextureTarget(multisampled), _id, 0);
		}

		static void AttachDepthTexture(uint32_t _id, int _samples, GLenum _format, GLenum _attachType, uint32_t _w, uint32_t _h)
		{
			bool multisampled = _samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _samples, _format, _w, _h, GL_FALSE);
			}
			else
			{
				glTexStorage2D(GL_TEXTURE_2D, 1, _format, _w, _h);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, _attachType, TextureTarget(multisampled), _id, 0);
		}

		static bool IsDepthFormat(FrameBufferTextureFormat _format)
		{
			switch (_format)
			{
			case FrameBufferTextureFormat::DEPTH24STENCIL8: return true;
			}
			return false;
		}

		static GLenum MayhemTextureFormatToGL(FrameBufferTextureFormat _format)
		{
			switch (_format)
			{
			case Mayhem::FrameBufferTextureFormat::None: return GL_NONE;
			case Mayhem::FrameBufferTextureFormat::RGBA8: return GL_RGBA8;
			case Mayhem::FrameBufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
			case Mayhem::FrameBufferTextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
			}

			return GL_NONE;
		}
	}

	OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpecification& _spec)
		: m_Spec(_spec)
	{
		for (auto format : m_Spec.Attachment.Attachments)
		{
			if (!Utils::IsDepthFormat(format.TextureFormat))
				m_ColorAttachmentsSpecs.emplace_back(format);
			else
				m_DepthAttachmentSpec = format;
		}
		Invalidate();
	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteTextures(1, &m_DepthAttachment);

		m_ColorAttachments.clear();
		m_DepthAttachment = 0;
	}

	void OpenGLFrameBuffer::Invalidate()
	{
		if (m_RendererID)
		{
			glDeleteFramebuffers(1, &m_RendererID);
			glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
			glDeleteTextures(1, &m_DepthAttachment);

			m_ColorAttachments.clear();
			m_DepthAttachment = 0;
		}

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		bool multisample = m_Spec.Samples > 1;

		// Attachements
		if (m_ColorAttachmentsSpecs.size())
		{
			m_ColorAttachments.resize(m_ColorAttachmentsSpecs.size());
			Utils::CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

			for (int i = 0; i < m_ColorAttachments.size(); i++)
			{
				Utils::BindTexture(multisample, m_ColorAttachments[i]);
				switch (m_ColorAttachmentsSpecs[i].TextureFormat)
				{
				case FrameBufferTextureFormat::RGBA8:
					Utils::AttachColorTexture(m_ColorAttachments[i], m_Spec.Samples, GL_RGBA8, GL_RGBA, m_Spec.Width, m_Spec.Height, i);
					break;
				case FrameBufferTextureFormat::RED_INTEGER:
					Utils::AttachColorTexture(m_ColorAttachments[i], m_Spec.Samples, GL_R32I, GL_RED_INTEGER, m_Spec.Width, m_Spec.Height, i);
					break;
				}
			}
		}

		if (m_DepthAttachmentSpec.TextureFormat != FrameBufferTextureFormat::None)
		{
			Utils::CreateTextures(multisample, &m_DepthAttachment, 1);
			Utils::BindTexture(multisample, m_DepthAttachment);

			switch (m_DepthAttachmentSpec.TextureFormat)
			{
			case FrameBufferTextureFormat::DEPTH24STENCIL8:
				Utils::AttachDepthTexture(m_DepthAttachment, m_Spec.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_ATTACHMENT, m_Spec.Width, m_Spec.Height);
				break;
			}
		}

		if (m_ColorAttachments.size() > 1)
		{
			MAYHEM_CORE_ASSERT(m_ColorAttachments.size() <= 4, "We do not support more than 4");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_ColorAttachments.size(), buffers);
		}
		else if (m_ColorAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Spec.Width, m_Spec.Height);
	}

	void OpenGLFrameBuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::Resize(uint32_t _w, uint32_t _h)
	{
		if (_w <= 0 || _w > s_MaxFrameBufferSize)
		{
			MAYHEM_CORE_WARN("Attempt to resize FrameBuffer to an invalide Size");
			return;
		}
		if (_h <= 0 || _h > s_MaxFrameBufferSize)
		{
			MAYHEM_CORE_WARN("Attempt to resize FrameBuffer to an invalide Size");
			return;
		}

		glViewport(0, 0, _w, _h);

		m_Spec.Width = _w;
		m_Spec.Height = _h;

		Invalidate();
	}

	int OpenGLFrameBuffer::ReadPixel(uint32_t _index, int _x, int _y)
	{
		MAYHEM_CORE_ASSERT(_index < m_ColorAttachments.size(), "");

		glReadBuffer(GL_COLOR_ATTACHMENT0 + _index);
		int pixelData;
		glReadPixels(_x, _y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
		return pixelData;
	}

	void OpenGLFrameBuffer::ClearAttachement(uint32_t _index, int _value)
	{
		MAYHEM_CORE_ASSERT(_index < m_ColorAttachments.size(), "");

		auto& spec = m_ColorAttachmentsSpecs[_index];
		glClearTexImage(m_ColorAttachments[_index], 0,
			Utils::MayhemTextureFormatToGL(spec.TextureFormat), GL_INT, &_value);
	}

}
