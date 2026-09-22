#include "Enpch.h"
#include "AssetsManager.h"

namespace Mayhem
{
	AssetsManager* AssetsManager::m_Instance = nullptr;

	AssetsManager* AssetsManager::GetInstance()
	{
		if (m_Instance == nullptr)
			m_Instance = new AssetsManager();

		return m_Instance;
	}
	AssetsManager::AssetsManager()
	{
	}

	AssetsManager::~AssetsManager()
	{
		m_Textures.clear();
	}

	Ref<Texture2D> AssetsManager::GetTexture(std::string _path)
	{
		if (m_Textures.contains(_path))
		{
			return m_Textures[_path];
		}

		m_Textures[_path] = Texture2D::CreateFromAssetsManager(_path);

		return m_Textures[_path];
	}
}

