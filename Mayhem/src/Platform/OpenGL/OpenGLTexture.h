#pragma once

#include <Engine/Renderer/Texture.h>
#include <glad/glad.h>

namespace Mayhem
{
	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const std::string& _path);
		OpenGLTexture2D(uint32_t _w, uint32_t _h);
		virtual ~OpenGLTexture2D();
		virtual uint32_t GetRendererID() const { return m_RendererID; };

		virtual void SetData(void* _data, uint32_t _size) override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t _slot = 0) const override;

		virtual bool operator== (const Texture& _other) const override
		{
			return m_RendererID == ((OpenGLTexture2D&)_other).m_RendererID;
		}

		virtual glm::vec2* GetTextureCoords() override { return m_TextureCoords; }

	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		glm::vec2 m_TextureCoords[4] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
		};

		GLenum m_InternalFormat, m_Format;
	};
}
