#pragma once

#include <Engine/Core/Core.h>
#include <glad/glad.h>

namespace Mayhem
{
	enum class FrameBufferTextureFormat
	{
		None = 0,

		// Color
		RGBA8,
		RED_INTEGER,

		// Depth/stencil
		DEPTH24STENCIL8,
		
		// Defaults
		Depth = DEPTH24STENCIL8,
	};

	struct FrameBufferTextureSpecification
	{
		FrameBufferTextureSpecification() = default;
		FrameBufferTextureSpecification(FrameBufferTextureFormat _format)
			: TextureFormat(_format) { }

			FrameBufferTextureFormat TextureFormat = FrameBufferTextureFormat::None;
			// Filtering/Wrap
	};

	struct FrameBufferAttachmentSpecification
	{
		FrameBufferAttachmentSpecification() = default;
		FrameBufferAttachmentSpecification(const std::initializer_list<FrameBufferTextureSpecification> _attachments)
			: Attachments(_attachments) { }

		std::vector<FrameBufferTextureSpecification> Attachments;
	};

	struct FrameBufferSpecification
	{
		uint32_t Width = 1280, Height = 720;
		uint32_t Samples = 1;
		FrameBufferAttachmentSpecification Attachment;
		bool SwapChainTarget = false;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual const FrameBufferSpecification& GetSpecification() const = 0;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Resize(uint32_t _w, uint32_t _h) = 0;
		virtual int ReadPixel(uint32_t _index, int _x, int _y) = 0;

		virtual void ClearAttachement(uint32_t _index, int _value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& _spec);
	};
}
