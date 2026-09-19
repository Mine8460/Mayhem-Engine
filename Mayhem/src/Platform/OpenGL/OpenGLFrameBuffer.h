#pragma once
#include <Engine/Renderer/FrameBuffer.h>

namespace Mayhem
{
	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferSpecification& _spec);
		virtual ~OpenGLFrameBuffer();

		virtual const FrameBufferSpecification& GetSpecification() const override { return m_Spec; }
		virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;
		virtual void Resize(uint32_t _w, uint32_t _h) override;
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_ColorAttachment = 0;
		uint32_t m_DepthAttachment = 0;
		FrameBufferSpecification m_Spec;
	};
}
