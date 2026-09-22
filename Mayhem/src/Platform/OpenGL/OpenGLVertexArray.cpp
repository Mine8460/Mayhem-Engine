#include "Enpch.h"
#include "OpenGLVertexArray.h"

#include <glad/glad.h>

namespace Mayhem
{
	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType _type)
	{
		switch (_type)
		{
			case Mayhem::ShaderDataType::None:	return GL_NONE;
			case Mayhem::ShaderDataType::Float: return GL_FLOAT;
			case Mayhem::ShaderDataType::Float2:return GL_FLOAT;
			case Mayhem::ShaderDataType::Float3:return GL_FLOAT;
			case Mayhem::ShaderDataType::Float4:return GL_FLOAT;
			case Mayhem::ShaderDataType::Mat3:return GL_FLOAT;
			case Mayhem::ShaderDataType::Mat4:return GL_FLOAT;
			case Mayhem::ShaderDataType::Int:return GL_INT;
			case Mayhem::ShaderDataType::Int2:return GL_INT;
			case Mayhem::ShaderDataType::Int3:return GL_INT;
			case Mayhem::ShaderDataType::Int4:return GL_INT;
			case Mayhem::ShaderDataType::Bool:return GL_BOOL;
			default: return GL_NONE;
		}
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_RendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_RendererID);
	}

	void OpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& _buffer)
	{
		glBindVertexArray(m_RendererID);
		_buffer->Bind();

		uint32_t index = 0;
		for (const auto& element : _buffer->GetLayout())
		{
			glEnableVertexAttribArray(index);

			if (element.Type == ShaderDataType::Int || element.Type == ShaderDataType::Int2 || element.Type == ShaderDataType::Int3 || element.Type == ShaderDataType::Int4)
			{
				glVertexAttribIPointer(index,
					element.GetComponentCount(),
					ShaderDataTypeToOpenGLBaseType(element.Type),
					_buffer->GetLayout().GetStride(),
					(const void*)element.Offset);
			}
			else
			{
			glVertexAttribPointer(index,
				element.GetComponentCount(),
				ShaderDataTypeToOpenGLBaseType(element.Type),
				element.Normalized ? GL_TRUE : GL_FALSE,
				_buffer->GetLayout().GetStride(),
				(const void*)element.Offset);
			}


			index++;
		}

		m_VertexBuffers.push_back(_buffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& _buffer)
	{
		glBindVertexArray(m_RendererID);
		_buffer->Bind();

		m_IndexBuffer = _buffer;
	}

}
