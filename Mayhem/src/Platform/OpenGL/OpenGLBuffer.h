#pragma once

#include <Engine/Renderer/Buffer.h>

namespace Mayhem
{
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t _size);
		OpenGLVertexBuffer(float* _vertices, uint32_t _size);
		virtual ~OpenGLVertexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual void SetLayout(const BufferLayout& _layout) override { m_Layout = _layout; };
		virtual const BufferLayout& GetLayout(void) const override { return m_Layout; };

		virtual void SetData(const void* _data, uint32_t _size) override;
	private:
		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t* _indices, uint32_t _count);
		virtual ~OpenGLIndexBuffer();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual uint32_t GetCount() override { return m_Count; }

	private:
		uint32_t m_RendererID;
		uint32_t m_Count;
	};
}
