#pragma once
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

	constexpr DXGI_FORMAT MakeSRGB(DXGI_FORMAT Format) noexcept
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		default:
			return Format;
		}
	}
} // namespace D3D12RHIUtils

namespace RHI
{
	enum class RHID3D12CommandQueueType
	{
		Direct,
		AsyncCompute,
		Copy1, // High frequency copies from upload to default heap
		Copy2, // Data initialization during resource creation
	};

	class D3D12Exception : public Exception
	{
	public:
		D3D12Exception(std::string_view File, int Line, HRESULT ErrorCode);

		const char* GetErrorType() const noexcept override;
		std::string GetError() const override;

	private:
		const HRESULT ErrorCode;
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

	class D3D12Device;

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

	class D3D12LinkedDevice;

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

	class D3D12Fence;
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
			: D3D12SyncHandle()
		{
		}

		D3D12SyncHandle& operator=(std::nullptr_t) noexcept
		{
			Fence = nullptr;
			Value = 0;
			return *this;
		}

		explicit operator bool() const noexcept;

		[[nodiscard]] auto GetValue() const noexcept -> UINT64;
		[[nodiscard]] auto IsComplete() const -> bool;
		auto			   WaitForCompletion() const -> void;

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

		CFencePool(const CFencePool&) = delete;
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

	template<typename TFunc>
	struct D3D12ScopedMap
	{
		D3D12ScopedMap(ID3D12Resource* Resource, UINT Subresource, D3D12_RANGE ReadRange, TFunc Func)
			: Resource(Resource)
		{
			void* Data = nullptr;
			if (SUCCEEDED(Resource->Map(Subresource, &ReadRange, &Data)))
			{
				Func(Data);
			}
			else
			{
				Resource = nullptr;
			}
		}
		~D3D12ScopedMap()
		{
			if (Resource)
			{
				Resource->Unmap(0, nullptr);
			}
		}

		ID3D12Resource* Resource = nullptr;
	};

	struct D3D12ScopedPointer
	{
		D3D12ScopedPointer() noexcept = default;
		D3D12ScopedPointer(ID3D12Resource* Resource)
			: Resource(Resource)
		{
			if (Resource)
			{
				if (FAILED(Resource->Map(0, nullptr, reinterpret_cast<void**>(&Address))))
				{
					Resource = nullptr;
				}
			}
		}
		D3D12ScopedPointer(D3D12ScopedPointer&& D3D12ScopedPointer) noexcept
			: Resource(std::exchange(D3D12ScopedPointer.Resource, {}))
			, Address(std::exchange(D3D12ScopedPointer.Address, nullptr))
		{
		}
		D3D12ScopedPointer& operator=(D3D12ScopedPointer&& D3D12ScopedPointer) noexcept
		{
			if (this != &D3D12ScopedPointer)
			{
				Resource = std::exchange(D3D12ScopedPointer.Resource, {});
				Address	 = std::exchange(D3D12ScopedPointer.Address, nullptr);
			}
			return *this;
		}
		~D3D12ScopedPointer()
		{
			if (Resource)
			{
				Resource->Unmap(0, nullptr);
			}
		}

		ID3D12Resource* Resource = nullptr;
		BYTE*			Address	 = nullptr;
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

	// Enum->String mappings
	inline D3D12_COMMAND_LIST_TYPE RHITranslateD3D12(RHID3D12CommandQueueType Type)
	{
		// clang-format off
		switch (Type)
		{
			using enum RHID3D12CommandQueueType;
		case Direct:		return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case AsyncCompute:	return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case Copy1:			[[fallthrough]];
		case Copy2:			return D3D12_COMMAND_LIST_TYPE_COPY;
		}
		// clang-format on
		return D3D12_COMMAND_LIST_TYPE();
	}

	inline LPCWSTR GetCommandQueueTypeString(RHID3D12CommandQueueType Type)
	{
		// clang-format off
		switch (Type)
		{
			using enum RHID3D12CommandQueueType;
		case Direct:		return L"3D";
		case AsyncCompute:	return L"Async Compute";
		case Copy1:			return L"Copy 1";
		case Copy2:			return L"Copy 2";
		}
		// clang-format on
		return L"<unknown>";
	}

	inline LPCWSTR GetCommandQueueTypeFenceString(RHID3D12CommandQueueType Type)
	{
		// clang-format off
		switch (Type)
		{
			using enum RHID3D12CommandQueueType;
		case Direct:		return L"3D Fence";
		case AsyncCompute:	return L"Async Compute Fence";
		case Copy1:			return L"Copy 1 Fence";
		case Copy2:			return L"Copy 2 Fence";
		}
		// clang-format on
		return L"<unknown>";
	}

	inline LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op)
	{
#define STRINGIFY(x) \
	case x:          \
		return L#x

		switch (Op)
		{
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETMARKER);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDEVENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCH);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTILES);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PRESENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA);
		}
		return L"<unknown>";
#undef STRINGIFY
	}

	inline LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type)
	{
#define STRINGIFY(x) \
	case x:          \
		return L#x

		switch (Type)
		{
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_FENCE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_RESOURCE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PASS);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_METACOMMAND);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_INVALID);
		}
		return L"<unknown>";
#undef STRINGIFY
	}
} // namespace RHI
