#include "Enpch.h"
#include "SubTexture2D.h"

namespace Mayhem
{

	SubTexture2D::SubTexture2D(const Ref<Texture2D>& _texture, const glm::vec2& _min, const glm::vec2& _max)
		: m_Texture(_texture)
	{
		m_TexCoords[0] = { _min.x, _min.y };
		m_TexCoords[1] = { _max.x, _min.y };
		m_TexCoords[2] = { _max.x, _max.y };
		m_TexCoords[3] = { _min.x, _max.y };
	}

	Ref<SubTexture2D> SubTexture2D::CreateFromCoords(const Ref<Texture2D>& _texture, const glm::vec2& _coords, const glm::vec2& _spriteSize)
	{
		// SpriteSheet Render
		float x = _coords.x, y = _coords.y;
		float spriteW = _spriteSize.x, spriteH = _spriteSize.y;
		float imageW = _texture->GetWidth(), imageH = _texture->GetHeight();

		float xPos = (x * spriteW) / imageW;
		float yPos = (y * spriteH) / imageH;

		float spriteXPercent = spriteW / imageW;
		float spriteYPercent = spriteH / imageH;

		const glm::vec2 min = { xPos, yPos };
		const glm::vec2 max = { xPos + spriteXPercent, yPos + spriteYPercent };

		return MakeRef<SubTexture2D>(_texture, min, max);
	}

	std::vector<Ref<SubTexture2D>> SubTexture2D::CreateAllSpriteSheet(const Ref<Texture2D>& _texture, const glm::vec2& _spriteSize)
	{
		float spriteW = _spriteSize.x, spriteH = _spriteSize.y;
		float imageW = _texture->GetWidth(), imageH = _texture->GetHeight();

		float spriteXPercent = spriteW / imageW;
		float spriteYPercent = spriteH / imageH;

		std::vector<Ref<SubTexture2D>> subTextures;

		for (float x = 0; x <= imageW; x += spriteW)
		{
			for (float y = 0; y <= imageH; y += spriteH)
			{
				float xPos = (x * spriteW) / imageW;
				float yPos = (y * spriteH) / imageH;

				const glm::vec2 min = { xPos, yPos };
				const glm::vec2 max = { xPos + spriteXPercent,yPos + spriteYPercent };

				subTextures.push_back(MakeRef<SubTexture2D>(_texture, min, max));
			}
		}

		return subTextures;
	}

}
