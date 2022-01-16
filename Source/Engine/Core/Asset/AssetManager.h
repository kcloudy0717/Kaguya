#pragma once
#include "AsyncImporter.h"
#include "AssetCache.h"

class AssetManager
{
public:
	static void Initialize(D3D12Device* Device);

	static void Shutdown();

	template<AssetType Type, typename... TArgs>
	static typename AssetTypeTraits<Type>::Type* CreateAsset(TArgs&&... Args)
	{
		if constexpr (Type == AssetType::Mesh)
		{
			AssetHandle Handle = MeshCache.Create(std::forward<TArgs>(Args)...);
			return MeshCache.GetAsset(Handle);
		}
		else if constexpr (Type == AssetType::Texture)
		{
			AssetHandle Handle = TextureCache.Create(std::forward<TArgs>(Args)...);
			return TextureCache.GetAsset(Handle);
		}

		return nullptr;
	}

	static AssetCache<AssetType::Mesh, Mesh>&		GetMeshCache() { return MeshCache; }
	static AssetCache<AssetType::Texture, Texture>& GetTextureCache() { return TextureCache; }

	static AssetType GetAssetTypeFromExtension(const std::filesystem::path& Path);

	static void AsyncLoadImage(const TextureImportOptions& Options);

	static void AsyncLoadMesh(const MeshImportOptions& Options);

	static void RequestUpload(Texture* Texture);

	static void RequestUpload(Mesh* Mesh);

private:
	static void UploadTexture(Texture* AssetTexture, D3D12LinkedDevice* Device);
	static void UploadMesh(Mesh* AssetMesh, D3D12LinkedDevice* Device);

	inline static AsyncMeshImporter	   MeshImporter;
	inline static AsyncTextureImporter TextureImporter;

	inline static AssetCache<AssetType::Mesh, Mesh>		  MeshCache;
	inline static AssetCache<AssetType::Texture, Texture> TextureCache;

	// Upload stuff to the GPU
	inline static std::mutex			  Mutex;
	inline static std::condition_variable ConditionVariable;
	inline static std::queue<Mesh*>		  MeshUploadQueue;
	inline static std::queue<Texture*>	  TextureUploadQueue;

	inline static std::jthread		Thread;
	inline static std::atomic<bool> Quit = false;

	friend class AssetWindow;
};
