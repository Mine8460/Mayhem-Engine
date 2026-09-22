#pragma once

#include <stdint.h>
#include <string>
#include <Engine/Core/Core.h>
#include <glm/glm.hpp>

namespace Mayhem
{
	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(void* _data, uint32_t _size) = 0;

		virtual void Bind(uint32_t _slot = 0) const = 0;

		virtual bool operator== (const Texture& _other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		static Ref<Texture2D> Create(uint32_t _w, uint32_t _h);
		static Ref<Texture2D> Create(const std::string& _path);

		virtual std::string GetPath() = 0;
		virtual glm::vec2* GetTextureCoords() { return nullptr; }
		
	private:
		static Ref<Texture2D> CreateFromAssetsManager(const std::string& _path);

		friend class AssetsManager;
	};
}
