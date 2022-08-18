#pragma once
#include "D3D12Types.h"

namespace RHI
{
	// https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html#subresource-state-tracking
	class CResourceState
	{
	public:
		enum class ETrackingMode
		{
			PerResource,
			PerSubresource
		};

		CResourceState() noexcept = default;
		explicit CResourceState(u32 NumSubresources, D3D12_RESOURCE_STATES InitialResourceState);

		[[nodiscard]] auto begin() const noexcept { return SubresourceStates.begin(); }
		[[nodiscard]] auto end() const noexcept { return SubresourceStates.end(); }

		[[nodiscard]] bool IsUninitialized() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNINITIALIZED; }
		[[nodiscard]] bool IsUnknown() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNKNOWN; }

		// Returns true if all subresources have the same state
		[[nodiscard]] bool IsUniform() const noexcept { return TrackingMode == ETrackingMode::PerResource; }

		[[nodiscard]] D3D12_RESOURCE_STATES GetSubresourceState(u32 Subresource) const;

		void SetSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State);

	private:
		ETrackingMode					   TrackingMode	 = ETrackingMode::PerResource;
		D3D12_RESOURCE_STATES			   ResourceState = D3D12_RESOURCE_STATE_UNINITIALIZED;
		std::vector<D3D12_RESOURCE_STATES> SubresourceStates;
	};

	class D3D12Resource : public D3D12LinkedDeviceChild
	{
	public:
		D3D12Resource() noexcept = default;
		D3D12Resource(
			D3D12LinkedDevice*	  Parent,
			Arc<ID3D12Resource>&& Resource,
			D3D12_CLEAR_VALUE	  ClearValue,
			D3D12_RESOURCE_STATES InitialResourceState);
		D3D12Resource(
			D3D12LinkedDevice*				 Parent,
			D3D12_HEAP_PROPERTIES			 HeapProperties,
			D3D12_RESOURCE_DESC				 Desc,
			D3D12_RESOURCE_STATES			 InitialResourceState,
			std::optional<D3D12_CLEAR_VALUE> ClearValue);

		D3D12Resource(D3D12Resource&&) noexcept = default;
		D3D12Resource& operator=(D3D12Resource&&) noexcept = default;

		D3D12Resource(const D3D12Resource&) = delete;
		D3D12Resource& operator=(const D3D12Resource&) = delete;

		operator bool() const noexcept
		{
			return !!Resource;
		}

		[[nodiscard]] ID3D12Resource*			 GetResource() const { return Resource.Get(); }
		[[nodiscard]] D3D12_CLEAR_VALUE			 GetClearValue() const noexcept { return ClearValue.has_value() ? *ClearValue : D3D12_CLEAR_VALUE{}; }
		[[nodiscard]] const D3D12_RESOURCE_DESC& GetDesc() const noexcept { return Desc; }
		[[nodiscard]] UINT16					 GetMipLevels() const noexcept { return Desc.MipLevels; }
		[[nodiscard]] UINT16					 GetArraySize() const noexcept { return Desc.DepthOrArraySize; }
		[[nodiscard]] UINT8						 GetPlaneCount() const noexcept { return PlaneCount; }
		[[nodiscard]] UINT						 GetNumSubresources() const noexcept { return NumSubresources; }
		[[nodiscard]] CResourceState&			 GetResourceState() { return ResourceState; }

		// https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions
		// https://devblogs.microsoft.com/directx/a-look-inside-d3d12-resource-state-barriers/
		// Can this resource be promoted to State from common
		[[nodiscard]] bool ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const noexcept;

		// Can this resource decay back to common
		// 4 Cases:
		// 1. Resources being accessed on a Copy queue, or
		// 2. Buffer resources on any queue type, or
		// 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set, or
		// 4. Any resource implicitly promoted to a read-only state.
		[[nodiscard]] bool ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType) const noexcept;

	private:
		[[nodiscard]] Arc<ID3D12Resource> InitializeResource(
			D3D12_HEAP_PROPERTIES			 HeapProperties,
			D3D12_RESOURCE_DESC				 Desc,
			D3D12_RESOURCE_STATES			 InitialResourceState,
			std::optional<D3D12_CLEAR_VALUE> ClearValue) const;

		[[nodiscard]] UINT CalculateNumSubresources() const;

	protected:
		// TODO: Add support for custom heap properties for UMA GPUs

		Arc<ID3D12Resource>				 Resource;
		std::optional<D3D12_CLEAR_VALUE> ClearValue;
		D3D12_RESOURCE_DESC				 Desc			 = {};
		UINT8							 PlaneCount		 = 0;
		UINT							 NumSubresources = 0;
		CResourceState					 ResourceState;
	};

	class D3D12ASBuffer : public D3D12Resource
	{
	public:
		D3D12ASBuffer() noexcept = default;
		D3D12ASBuffer(
			D3D12LinkedDevice* Parent,
			UINT64			   SizeInBytes);

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;
	};

	class D3D12Buffer : public D3D12Resource
	{
	public:
		D3D12Buffer() noexcept = default;
		D3D12Buffer(
			D3D12LinkedDevice*	 Parent,
			UINT64				 SizeInBytes,
			UINT				 Stride,
			D3D12_HEAP_TYPE		 HeapType,
			D3D12_RESOURCE_FLAGS ResourceFlags);
		~D3D12Buffer();

		D3D12Buffer(D3D12Buffer&&) noexcept = default;
		D3D12Buffer& operator=(D3D12Buffer&&) noexcept = default;

		D3D12Buffer(const D3D12Buffer&) = delete;
		D3D12Buffer& operator=(const D3D12Buffer&) = delete;

		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index) const;
		[[nodiscard]] UINT						GetStride() const { return Stride; }
		template<typename T>
		[[nodiscard]] T* GetCpuVirtualAddress() const
		{
			assert(MappedData && "Invalid CpuVirtualAddress");
			return reinterpret_cast<T*>(MappedData.get());
		}

		[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const noexcept
		{
			D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
			VertexBufferView.BufferLocation			  = Resource->GetGPUVirtualAddress();
			VertexBufferView.SizeInBytes			  = static_cast<UINT>(Desc.Width);
			VertexBufferView.StrideInBytes			  = Stride;
			return VertexBufferView;
		}

		[[nodiscard]] D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(DXGI_FORMAT Format = DXGI_FORMAT_R32_UINT) const noexcept
		{
			D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
			IndexBufferView.BufferLocation			= Resource->GetGPUVirtualAddress();
			IndexBufferView.SizeInBytes				= static_cast<UINT>(Desc.Width);
			IndexBufferView.Format					= Format;
			return IndexBufferView;
		}

		template<typename T>
		void CopyData(UINT Index, const T& Data)
		{
			assert(MappedData.get() && "Invalid CpuVirtualAddress");
			memcpy(&MappedData.get()[Index * Stride], &Data, sizeof(T));
		}

	private:
		static void EmptyDestructor([[maybe_unused]] BYTE* Ptr)
		{
		}

	private:
		D3D12_HEAP_TYPE									  HeapType	 = {};
		UINT											  Stride	 = 0;
		std::unique_ptr<BYTE, decltype(&EmptyDestructor)> MappedData = { nullptr, &EmptyDestructor }; // Upload heap
	};

	class D3D12Texture : public D3D12Resource
	{
	public:
		D3D12Texture() noexcept = default;
		D3D12Texture(
			D3D12LinkedDevice*	  Parent,
			Arc<ID3D12Resource>&& Resource,
			D3D12_CLEAR_VALUE	  ClearValue,
			D3D12_RESOURCE_STATES InitialResourceState);
		D3D12Texture(
			D3D12LinkedDevice*				 Parent,
			const D3D12_RESOURCE_DESC&		 Desc,
			std::optional<D3D12_CLEAR_VALUE> ClearValue = std::nullopt,
			bool							 Cubemap	= false);

		[[nodiscard]] UINT GetSubresourceIndex(
			std::optional<UINT> OptArraySlice = std::nullopt,
			std::optional<UINT> OptMipSlice	  = std::nullopt,
			std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

		[[nodiscard]] bool IsCubemap() const noexcept { return Cubemap; }

	private:
		bool Cubemap = false;
	};
} // namespace RHI
