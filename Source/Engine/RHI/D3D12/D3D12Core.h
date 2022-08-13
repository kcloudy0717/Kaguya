#pragma once
#include <queue>
#include "D3D12RHI.h"
#include "RHICore.h"
#include "System/System.h"
#include "Arc.h"

namespace D3D12RHIUtils
{
	template<typename T, typename U>
	constexpr T AlignUp(T Size, U Alignment)
	{
		return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
	}
} // namespace D3D12RHIUtils

namespace RHI
{
	class D3D12Device;
	class D3D12LinkedDevice;
	class D3D12Fence;

	enum class RHID3D12CommandQueueType
	{
		Direct,
		AsyncCompute,
		Copy,	// High frequency copies from upload to default heap
		Upload, // Data initialization during resource creation
	};

	class ExceptionD3D12 final : public Exception
	{
	public:
		ExceptionD3D12(std::string_view Source, int Line, HRESULT ErrorCode)
			: Exception(Source, Line)
			, ErrorCode(ErrorCode)
		{
		}

	private:
		const char* GetErrorType() const noexcept override;
		std::string GetError() const override;

	private:
		HRESULT ErrorCode;
	};

	struct D3D12NodeMask
	{
		D3D12NodeMask() noexcept
			: NodeMask(1)
		{
		}
		constexpr D3D12NodeMask(UINT NodeMask)
			: NodeMask(NodeMask)
		{
		}

		operator UINT() const noexcept
		{
			return NodeMask;
		}

		static D3D12NodeMask FromIndex(u32 GpuIndex) { return { 1u << GpuIndex }; }

		UINT NodeMask;
	};

	class D3D12DeviceChild
	{
	public:
		D3D12DeviceChild() noexcept = default;
		explicit D3D12DeviceChild(D3D12Device* Parent) noexcept
			: Parent(Parent)
		{
		}

		[[nodiscard]] auto GetParentDevice() const noexcept -> D3D12Device* { return Parent; }

	protected:
		D3D12Device* Parent = nullptr;
	};

	class D3D12LinkedDeviceChild
	{
	public:
		D3D12LinkedDeviceChild() noexcept = default;
		explicit D3D12LinkedDeviceChild(D3D12LinkedDevice* Parent) noexcept
			: Parent(Parent)
		{
		}

		[[nodiscard]] auto GetParentLinkedDevice() const noexcept -> D3D12LinkedDevice* { return Parent; }

	protected:
		D3D12LinkedDevice* Parent = nullptr;
	};

	// Represents a Fence and Value pair, similar to that of a coroutine handle
	// you can query the status of a command execution point and wait for it
	class D3D12SyncHandle
	{
	public:
		D3D12SyncHandle() noexcept = default;
		explicit D3D12SyncHandle(D3D12Fence* Fence, UINT64 Value) noexcept
			: Fence(Fence)
			, Value(Value)
		{
		}

		D3D12SyncHandle(std::nullptr_t) noexcept
		{
		}

		D3D12SyncHandle& operator=(std::nullptr_t) noexcept
		{
			Fence = nullptr;
			Value = 0;
			return *this;
		}

		explicit operator bool() const noexcept;

		[[nodiscard]] UINT64 GetValue() const noexcept;
		[[nodiscard]] bool	 IsComplete() const;
		void				 WaitForCompletion() const;

	private:
		friend class D3D12CommandQueue;

		D3D12Fence* Fence = nullptr;
		UINT64		Value = 0;
	};

	template<typename TResourceType>
	class CFencePool
	{
	public:
		CFencePool(bool ThreadSafe = false) noexcept
			: Mutex(ThreadSafe ? std::make_unique<std::mutex>() : nullptr)
		{
		}
		CFencePool(CFencePool&& CFencePool) noexcept
			: Pool(std::exchange(CFencePool.Pool, {}))
			, Mutex(std::exchange(CFencePool.Mutex, {}))
		{
		}
		CFencePool& operator=(CFencePool&& CFencePool) noexcept
		{
			if (this == &CFencePool)
			{
				return *this;
			}

			Pool  = std::exchange(CFencePool.Pool, {});
			Mutex = std::exchange(CFencePool.Mutex, {});
			return *this;
		}

		CFencePool(const CFencePool&)			 = delete;
		CFencePool& operator=(const CFencePool&) = delete;

		void ReturnToPool(TResourceType&& Resource, D3D12SyncHandle SyncHandle) noexcept
		{
			try
			{
				auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
				Pool.emplace_back(SyncHandle, std::move(Resource)); // throw( bad_alloc )
			}
			catch (std::bad_alloc&)
			{
				// Just drop the error
				// All uses of this pool use Arc, which will release the resource
			}
		}

		template<typename PFNCreateNew, typename... TArgs>
		TResourceType RetrieveFromPool(PFNCreateNew CreateNew, TArgs&&... Args) noexcept(false)
		{
			auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
			auto Head = Pool.begin();
			if (Head == Pool.end() || !Head->first.IsComplete())
			{
				return std::move(CreateNew(std::forward<TArgs>(Args)...));
			}

			assert(Head->second);
			TResourceType Resource = std::move(Head->second);
			Pool.erase(Head);
			return std::move(Resource);
		}

	protected:
		using TPoolEntry = std::pair<D3D12SyncHandle, TResourceType>;
		using TPool		 = std::list<TPoolEntry>;

		TPool						Pool;
		std::unique_ptr<std::mutex> Mutex;
	};

	template<typename T>
	concept DeferredDeleteResourceConcept = requires(T Resource)
	{
		Resource.Release();
	};

	template<DeferredDeleteResourceConcept T>
	class DeferredDeleteQueue
	{
	public:
		void Enqueue(D3D12SyncHandle GraphicsSyncHandle, D3D12SyncHandle ComputeSyncHandle, T Resource)
		{
			MutexGuard Guard(*QueueMutex);
			Queue.push(Entry{
				.GraphicsSyncHandle = GraphicsSyncHandle,
				.ComputeSyncHandle	= ComputeSyncHandle,
				.Resource			= Resource,
			});
		}

		void Clean()
		{
			MutexGuard Guard(*QueueMutex);
			while (!Queue.empty())
			{
				Entry& Entry = Queue.front();
				if (!Entry.GraphicsSyncHandle.IsComplete() ||
					!Entry.ComputeSyncHandle.IsComplete())
				{
					break;
				}
				Entry.Resource.Release();
				Queue.pop();
			}
		}

	private:
		struct Entry
		{
			D3D12SyncHandle GraphicsSyncHandle;
			D3D12SyncHandle ComputeSyncHandle;
			T				Resource;
		};

		std::queue<Entry>	   Queue;
		std::unique_ptr<Mutex> QueueMutex = std::make_unique<Mutex>();
	};

	class D3D12InputLayout
	{
	public:
		D3D12InputLayout() noexcept = default;
		D3D12InputLayout(size_t NumElements) { InputElements.reserve(NumElements); }

		void AddVertexLayoutElement(std::string_view SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot)
		{
			D3D12_INPUT_ELEMENT_DESC& Desc = InputElements.emplace_back();
			Desc.SemanticName			   = SemanticName.data();
			Desc.SemanticIndex			   = SemanticIndex;
			Desc.Format					   = Format;
			Desc.InputSlot				   = InputSlot;
			Desc.AlignedByteOffset		   = D3D12_APPEND_ALIGNED_ELEMENT;
			Desc.InputSlotClass			   = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			Desc.InstanceDataStepRate	   = 0;
		}

		std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
	};

	template<RHI_PIPELINE_STATE_TYPE PsoType>
	struct D3D12DescriptorTableTraits
	{
	};
	template<>
	struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Graphics>
	{
		static auto Bind() { return &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable; }
	};
	template<>
	struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Compute>
	{
		static auto Bind() { return &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable; }
	};
} // namespace RHI
