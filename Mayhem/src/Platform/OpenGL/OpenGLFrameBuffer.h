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
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { return m_ColorAttachments[index]; }
		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;
		virtual void Resize(uint32_t _w, uint32_t _h) override;
		virtual int ReadPixel(uint32_t _index, int _x, int _y) override;
	private:

		uint32_t m_RendererID = 0;
		FrameBufferSpecification m_Spec;

		std::vector<FrameBufferTextureSpecification> m_ColorAttachmentsSpecs;
		FrameBufferTextureSpecification m_DepthAttachmentSpec;
	
		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};
}
