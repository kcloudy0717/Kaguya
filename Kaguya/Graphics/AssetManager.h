#pragma once
#include <Core/Synchronization/RWLock.h>
#include "RenderDevice.h"

#include "Asset/AsyncLoader.h"

class AssetManager
{
public:
	static void Initialize();
	static void Shutdown();
	static AssetManager& Instance();

	Descriptor GetDefaultWhiteTexture() { return m_SystemTextureSRVs[AssetTextures::DefaultWhite]; }
	Descriptor GetDefaultBlackTexture() { return m_SystemTextureSRVs[AssetTextures::DefaultBlack]; }
	Descriptor GetDefaultAlbedoTexture() { return m_SystemTextureSRVs[AssetTextures::DefaultAlbedo]; }
	Descriptor GetDefaultNormalTexture() { return m_SystemTextureSRVs[AssetTextures::DefaultNormal]; }
	Descriptor GetDefaultRoughnessTexture() { return m_SystemTextureSRVs[AssetTextures::DefaultRoughness]; }

	auto& GetImageCache()
	{
		return m_ImageCache;
	}

	auto& GetMeshCache()
	{
		return m_MeshCache;
	}

	void AsyncLoadImage(const std::filesystem::path& Path, bool sRGB);
	void AsyncLoadMesh(const std::filesystem::path& Path, bool KeepGeometryInRAM);
private:
	AssetManager();
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;
	~AssetManager();

	void CreateSystemTextures();

	static DWORD WINAPI ResourceUploadThreadProc(_In_ PVOID pParameter);
private:
	struct AssetTextures
	{
		// These textures can be created using small resources
		enum System
		{
			DefaultWhite,
			DefaultBlack,
			DefaultAlbedo,
			DefaultNormal,
			DefaultRoughness,
			NumSystemTextures
		};
	};

	Microsoft::WRL::ComPtr<ID3D12Heap> m_SystemTextureHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_SystemTextures[AssetTextures::NumSystemTextures];
	Descriptor m_SystemTextureSRVs[AssetTextures::NumSystemTextures];

	AsyncImageLoader m_AsyncImageLoader;
	AsyncMeshLoader m_AsyncMeshLoader;

	AssetCache<Asset::Image> m_ImageCache;
	AssetCache<Asset::Mesh> m_MeshCache;

	// Upload stuff to the GPU
	CriticalSection m_UploadCriticalSection;
	ConditionVariable m_UploadConditionVariable;
	ThreadSafeQueue<std::shared_ptr<Asset::Image>> m_ImageUploadQueue;
	ThreadSafeQueue<std::shared_ptr<Asset::Mesh>> m_MeshUploadQueue;

	wil::unique_handle m_Thread;
	std::atomic<bool> m_ShutdownThread = false;

	friend class AssetWindow;
};