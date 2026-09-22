#pragma once

#include "Texture.h"
#include <glm/glm.hpp>

namespace Mayhem
{
	class SubTexture2D : Texture2D
	{
	public:
		SubTexture2D(const Ref<Texture2D>& _texture, const glm::vec2& _min, const glm::vec2& _max);

		const Ref<Texture2D> GetTexture() const { return m_Texture; }
		virtual glm::vec2* GetTextureCoords() override { return m_TexCoords; }

		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& _texture, const glm::vec2& _coords, const glm::vec2& _spriteSize);
		static std::vector<Ref<SubTexture2D>> CreateAllSpriteSheet(const Ref<Texture2D>& _texture, const glm::vec2& _spriteSize);
	
	
		virtual uint32_t GetWidth() const { return m_Texture->GetWidth(); }
		virtual uint32_t GetHeight() const { return m_Texture->GetHeight(); }
		virtual uint32_t GetRendererID() const { return m_Texture->GetRendererID(); }

		virtual void SetData(void* _data, uint32_t _size) { return m_Texture->SetData(_data, _size); }

		virtual void Bind(uint32_t _slot = 0) const { return m_Texture->Bind(_slot); }

		virtual bool operator== (const Texture& _other) const { return m_Texture->operator==(_other); }
	private:
		Ref<Texture2D> m_Texture;

		glm::vec2 m_TexCoords[4] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
		};
	};
}