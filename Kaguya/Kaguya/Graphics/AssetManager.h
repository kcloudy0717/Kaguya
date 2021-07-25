#pragma once
#include "RenderDevice.h"

#include <Core/Asset/AsyncLoader.h>

class AssetManager
{
public:
	static void Initialize();

	static void Shutdown();

	static AssetCache<Asset::Image>& GetImageCache() { return ImageCache; }

	static AssetCache<Asset::Mesh>& GetMeshCache() { return MeshCache; }

	static void AsyncLoadImage(const std::filesystem::path& Path, bool sRGB);

	static void AsyncLoadMesh(const std::filesystem::path& Path, bool KeepGeometryInRAM);

private:
	inline static AsyncImageLoader AsyncImageLoader;
	inline static AsyncMeshLoader  AsyncMeshLoader;

	inline static AssetCache<Asset::Image> ImageCache;
	inline static AssetCache<Asset::Mesh>  MeshCache;

	// Upload stuff to the GPU
	inline static std::mutex								Mutex;
	inline static std::condition_variable					ConditionVariable;
	inline static std::queue<std::shared_ptr<Asset::Image>> ImageUploadQueue;
	inline static std::queue<std::shared_ptr<Asset::Mesh>>	MeshUploadQueue;

	inline static wil::unique_handle Thread;
	inline static std::atomic<bool>	 Quit = false;

	friend class AssetWindow;
};
