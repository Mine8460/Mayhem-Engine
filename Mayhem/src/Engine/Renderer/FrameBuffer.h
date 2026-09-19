#pragma once

#include <Engine/Core/Core.h>

namespace Mayhem
{
	struct FrameBufferSpecification
	{
		uint32_t Width = 1280;
		uint32_t Height = 720;
		uint32_t Samples = 1;

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

		virtual uint32_t GetColorAttachmentRendererID() const = 0;

		static Ref<FrameBuffer> Create(const FrameBufferSpecification& _spec);
	};
}
