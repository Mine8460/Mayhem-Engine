#pragma once

#include "Texture.h"
#include <glm/glm.hpp>

namespace Mayhem
{
	class SubTexture2D 
	{
	public:
		SubTexture2D(const Ref<Texture2D>& _texture, const glm::vec2& _min, const glm::vec2& _max);

		const Ref<Texture2D> GetTexture() const { return m_Texture; }
		const glm::vec2* GetTexCoords() const { return m_TexCoords; }

		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& _texture, const glm::vec2& _coords, const glm::vec2& _spriteSize);
		static std::vector<Ref<SubTexture2D>> CreateAllSpriteSheet(const Ref<Texture2D>& _texture, const glm::vec2& _spriteSize);
	private:
		Ref<Texture2D> m_Texture;

		glm::vec2 m_TexCoords[4];
	};
}