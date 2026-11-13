#include "Enpch.h"
#include "OpenGLTexture.h"
#include <stb_image.h>


namespace Mayhem
{

	OpenGLTexture2D::OpenGLTexture2D(const std::string& _path) : m_Path(_path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		// Images en niveaux de gris (roughness, height, AO… souvent sur 1 ou 2 canaux) : converties en RGBA,
		// sinon elles seraient lues comme du RGB et on lirait au-delà du buffer.
		int fileChannels = 0;
		stbi_info(_path.c_str(), &width, &height, &fileChannels);
		const int desiredChannels = fileChannels == 3 ? 3 : 4;

		stbi_uc* data = stbi_load(_path.c_str(), &width, &height, &channels, desiredChannels);
		if (data == nullptr)
		{
			MAYHEM_CORE_ERROR("Failed to load the image !!!");
			MAYHEM_CORE_ERROR(stbi_failure_reason());
			MAYHEM_CORE_ERROR(_path.c_str());

			// Texture 1x1 magenta : bien visible, et pas de crash plus loin
			static uint32_t s_Missing = 0xffff00ff;
			width = height = 1;
			m_Width = m_Height = 1;
			m_InternalFormat = GL_RGBA8;
			m_Format = GL_RGBA;
			glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
			glTextureStorage2D(m_RendererID, 1, m_InternalFormat, 1, 1);
			glTextureSubImage2D(m_RendererID, 0, 0, 0, 1, 1, m_Format, GL_UNSIGNED_BYTE, &s_Missing);
			return;
		}
		m_Width = width;
		m_Height = height;

		if (desiredChannels == 4)
		{
			m_InternalFormat = GL_RGBA8;
			m_Format = GL_RGBA;
		}
		else
		{
			m_InternalFormat = GL_RGB8;
			m_Format = GL_RGB;
		}

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);

		// Mipmaps : indispensables pour les normal / roughness maps vues de loin (sinon elles scintillent)
		uint32_t levels = 1;
		for (uint32_t size = glm::max(m_Width, m_Height); size > 1; size >>= 1)
			levels++;

		glTextureStorage2D(m_RendererID, levels, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Lignes RGB pas forcément alignées sur 4 octets (largeur impaire)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_Format, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glGenerateTextureMipmap(m_RendererID);

		stbi_image_free(data);
	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t _w, uint32_t _h) : m_Width(_w), m_Height(_h)
	{
		m_InternalFormat = GL_RGBA8;
		m_Format = GL_RGB;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);

		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::SetData(void* _data, uint32_t _size)
	{
		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_Format, GL_UNSIGNED_BYTE, _data);
	}

	void OpenGLTexture2D::Bind(uint32_t _slot) const
	{
		glBindTextureUnit(_slot, m_RendererID);
	}

}
