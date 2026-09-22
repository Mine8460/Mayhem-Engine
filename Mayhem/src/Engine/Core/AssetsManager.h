#pragma once
#include <Engine/Renderer/Texture.h>
//#include <Engine/Core/Core.h>

namespace Mayhem
{
	class AssetsManager
	{
	public:
		static AssetsManager* GetInstance();

		AssetsManager();
		~AssetsManager();

		Ref<Texture2D> GetTexture(std::string _path);
	private:
		std::unordered_map<std::string, Ref<Texture2D>> m_Textures;
		static AssetsManager* m_Instance;

	};

}