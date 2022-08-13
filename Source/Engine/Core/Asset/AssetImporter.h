#pragma once
#include <set>
#include "System/System.h"
#include "Texture.h"
#include "Mesh.h"

DECLARE_LOG_CATEGORY(Asset);

namespace Asset
{
	class AssetManager;

	template<typename T>
	class AssetRegistry
	{
	public:
		static_assert(std::is_base_of_v<IAsset, T>, "IAsset is not based of T");

		static constexpr AssetType Enum = AssetTypeTraits<T>::Enum;

		AssetRegistry()
		{
		}

		void DestroyAll()
		{
			// Destroy in opposite order
			RwLockWriteGuard Guard(Lock);
			Assets.clear();
			CachedHandles.clear();
			Index = 0;
			decltype(IndexQueue)().swap(IndexQueue);
		}

		auto begin() noexcept { return CachedHandles.begin(); }
		auto end() noexcept { return CachedHandles.end(); }

		size_t size() const noexcept { return CachedHandles.size(); }

		bool ValidateHandle(AssetHandle Handle) noexcept
		{
			return Handle.IsValid() && Handle.Type == Enum && Handle.Id < Assets.size();
		}

		template<typename... TArgs>
		AssetHandle Create(TArgs&&... Args)
		{
			RwLockWriteGuard Guard(Lock);

			u32 Index = GetAssetIndex();

			AssetHandle Handle = {};
			Handle.Type		   = Enum;
			Handle.State	   = false;
			Handle.Version	   = 0;
			Handle.Id		   = Index;

			auto& Asset	  = Assets[Index];
			Asset		  = std::make_unique<T>(std::forward<TArgs>(Args)...);
			Asset->Handle = Handle;

			CachedHandles[Index] = Handle;
			return Handle;
		}

		T* GetAsset(AssetHandle Handle)
		{
			return Assets[Handle.Id].get();
		}

		T* GetValidAsset(AssetHandle& Handle)
		{
			if (ValidateHandle(Handle))
			{
				RwLockReadGuard Guard(Lock);

				bool State = CachedHandles[Handle.Id].State;
				if (State)
				{
					Handle.State = State;
					return Assets[Handle.Id].get();
				}
			}

			return nullptr;
		}

		void Destroy(AssetHandle Handle)
		{
			if (ValidateHandle(Handle))
			{
				RwLockWriteGuard Guard(Lock);

				ReleaseAsset(Handle.Id);
				CachedHandles[Handle.Id].Invalidate();
				Assets[Handle.Id].reset();
			}
		}

		void UpdateHandleState(AssetHandle Handle)
		{
			if (ValidateHandle(Handle))
			{
				RwLockWriteGuard Guard(Lock);

				CachedHandles[Handle.Id].State = Handle.State;
			}
		}

		template<typename Functor>
		void EnumerateAsset(Functor F)
		{
			RwLockReadGuard Guard(Lock);

			auto begin = CachedHandles.begin();
			auto end   = CachedHandles.end();
			while (begin != end)
			{
				auto current = begin++;

				if constexpr (std::is_invocable_v<Functor, AssetHandle, T*>)
				{
					if (current->IsValid())
					{
						F(*current, Assets[current->Id].get());
					}
				}
			}
		}

	private:
		u32 GetAssetIndex()
		{
			u32 NewIndex;
			if (!IndexQueue.empty())
			{
				NewIndex = IndexQueue.front();
				IndexQueue.pop();
			}
			else
			{
				CachedHandles.emplace_back();
				Assets.emplace_back();
				NewIndex = Index++;
			}
			return NewIndex;
		}

		void ReleaseAsset(u32 Index)
		{
			IndexQueue.push(Index);
			Assets[Index] = nullptr;
		}

	private:
		mutable RwLock Lock;

		std::queue<u32> IndexQueue;
		u32				Index = 0;

		// CachedHandles are use to update any external handle's states
		std::vector<AssetHandle>		CachedHandles;
		std::vector<std::unique_ptr<T>> Assets;
	};

	class AssetImporter
	{
	public:
		bool SupportsExtension(const std::filesystem::path& Path)
		{
			std::wstring Extension = Path.extension().wstring();
			return std::ranges::find_if(
					   SupportedExtensions,
					   [&](const std::wstring& SupportedExtension)
					   {
						   return SupportedExtension == Extension;
					   }) != SupportedExtensions.end();
		}

	protected:
		std::set<std::wstring> SupportedExtensions;
	};

	class MeshImporter : public AssetImporter
	{
	public:
		MeshImporter();

		std::vector<AssetHandle> Import(AssetManager* AssetManager, const MeshImportOptions& Options);

		void			   Export(const std::filesystem::path& BinaryPath, const std::vector<Mesh*>& Meshes);
		std::vector<Mesh*> ImportExisting(AssetManager* AssetManager, const std::filesystem::path& BinaryPath, const MeshImportOptions& Options);

		struct ExportHeader
		{
			size_t NumMeshes;
		};

		struct MeshHeader
		{
			size_t NumVertices;
			size_t NumIndices;
			size_t NumMeshlets;
			size_t NumUniqueVertexIndices;
			size_t NumPrimitiveIndices;
		};
	};

	class TextureImporter : public AssetImporter
	{
	public:
		TextureImporter();

		AssetHandle Import(AssetManager* AssetManager, const TextureImportOptions& Options);
	};
} // namespace Asset
