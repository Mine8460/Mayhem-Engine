#pragma once

#include <stdint.h>

namespace Mayhem
{
	enum class ShaderDataType
	{
		None = 0,
		Float,
		Float2,
		Float3,
		Float4,
		Mat3,
		Mat4,
		Int,
		Int2,
		Int3,
		Int4,
		Bool,
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType _type)
	{
		switch (_type)
		{
		case Mayhem::ShaderDataType::None: return 0;
		case Mayhem::ShaderDataType::Float: return sizeof(float);
		case Mayhem::ShaderDataType::Float2: return sizeof(float) * 2;
		case Mayhem::ShaderDataType::Float3: return sizeof(float) * 3;
		case Mayhem::ShaderDataType::Float4: return sizeof(float) * 4;
		case Mayhem::ShaderDataType::Mat3: return sizeof(float) * 3 * 3;
		case Mayhem::ShaderDataType::Mat4: return sizeof(float) * 4 * 4;
		case Mayhem::ShaderDataType::Int: return sizeof(int);
		case Mayhem::ShaderDataType::Int2: return sizeof(int) * 2;
		case Mayhem::ShaderDataType::Int3: return sizeof(int) * 3;
		case Mayhem::ShaderDataType::Int4: return sizeof(int) * 4;
		case Mayhem::ShaderDataType::Bool: return sizeof(bool);
		}

		ENGINE_CORE_ERROR("Unknown ShaderDataType!");
		return 0;
	}

	struct BufferElement
	{
		std::string Name;
		ShaderDataType Type;
		uint32_t Offset;
		uint32_t Size;
		bool Normalized;

		BufferElement() {}
		BufferElement(ShaderDataType _type, const std::string& _name, bool _normalized = false)
			: Name(_name), Type(_type), Size(ShaderDataTypeSize(_type)), Offset(0), Normalized(_normalized)
		{

		}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case Mayhem::ShaderDataType::None:	return 0;
			case Mayhem::ShaderDataType::Float:	return 1;
			case Mayhem::ShaderDataType::Float2:return 2;
			case Mayhem::ShaderDataType::Float3:return 3;
			case Mayhem::ShaderDataType::Float4:return 4;
			case Mayhem::ShaderDataType::Mat3:	return 3 * 3;
			case Mayhem::ShaderDataType::Mat4:	return 4 * 4;
			case Mayhem::ShaderDataType::Int:	return 1;
			case Mayhem::ShaderDataType::Int2:	return 2;
			case Mayhem::ShaderDataType::Int3:	return 3;
			case Mayhem::ShaderDataType::Int4:	return 4;
			case Mayhem::ShaderDataType::Bool:	return 1;
			}

			return 0;
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout() {}
		BufferLayout(const std::initializer_list<BufferElement>& _elements)
			: m_Elements(_elements)
		{
			CalculateOffsetAndStride();
		}

		inline uint32_t GetStride() const { return m_Stride; }
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); };
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); };
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); };
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); };
	private:
		void CalculateOffsetAndStride()
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void SetLayout(const BufferLayout& _layout) = 0;
		virtual const BufferLayout& GetLayout(void) const = 0;

		virtual void SetData(const void* _data, uint32_t _size) = 0;

		static VertexBuffer* Create(uint32_t _size);
		static VertexBuffer* Create(float* _vertices, uint32_t _size);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32_t GetCount() = 0;

		static IndexBuffer* Create(uint32_t* _indices, uint32_t _count);
	};
}
