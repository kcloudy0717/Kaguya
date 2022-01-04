#pragma once
#include "D3D12Common.h"

class D3D12Resource : public D3D12LinkedDeviceChild
{
public:
	D3D12Resource() noexcept = default;
	D3D12Resource(
		D3D12LinkedDevice*				 Parent,
		D3D12_HEAP_PROPERTIES			 HeapProperties,
		D3D12_RESOURCE_DESC				 Desc,
		D3D12_RESOURCE_STATES			 InitialResourceState,
		std::optional<D3D12_CLEAR_VALUE> ClearValue);

	[[nodiscard]] ID3D12Resource*			 GetResource() const { return Resource.Get(); }
	[[nodiscard]] D3D12_CLEAR_VALUE			 GetClearValue() const noexcept { return ClearValue.has_value() ? *ClearValue : D3D12_CLEAR_VALUE{}; }
	[[nodiscard]] const D3D12_RESOURCE_DESC& GetDesc() const noexcept { return Desc; }
	[[nodiscard]] UINT16					 GetMipLevels() const noexcept { return Desc.MipLevels; }
	[[nodiscard]] UINT16					 GetArraySize() const noexcept { return Desc.DepthOrArraySize; }
	[[nodiscard]] UINT8						 GetPlaneCount() const noexcept { return PlaneCount; }
	[[nodiscard]] UINT						 GetNumSubresources() const noexcept { return NumSubresources; }
	[[nodiscard]] CResourceState&			 GetResourceState() { return ResourceState; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> InitializeResource(
		D3D12_HEAP_PROPERTIES			 HeapProperties,
		D3D12_RESOURCE_DESC				 Desc,
		D3D12_RESOURCE_STATES			 InitialResourceState,
		std::optional<D3D12_CLEAR_VALUE> ClearValue);

	UINT CalculateNumSubresources();

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	std::optional<D3D12_CLEAR_VALUE>	   ClearValue;
	D3D12_RESOURCE_DESC					   Desc;
	UINT8								   PlaneCount;
	UINT								   NumSubresources;
	CResourceState						   ResourceState;
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

	// Call this for upload heap to map a cpu pointer
	void Initialize();

	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;
	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index) const;
	[[nodiscard]] UINT						GetStride() const { return Stride; }
	template<typename T>
	[[nodiscard]] T* GetCpuVirtualAddress() const
	{
		assert(CpuVirtualAddress && "Invalid CpuVirtualAddress");
		return reinterpret_cast<T*>(CpuVirtualAddress);
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
		assert(CpuVirtualAddress && "Invalid CpuVirtualAddress");
		memcpy(&CpuVirtualAddress[Index * Stride], &Data, sizeof(T));
	}

private:
	D3D12_HEAP_TYPE HeapType		  = {};
	UINT			Stride			  = 0;
	BYTE*			CpuVirtualAddress = nullptr;
};

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture() noexcept = default;
	D3D12Texture(
		D3D12LinkedDevice*				 Parent,
		const D3D12_RESOURCE_DESC&		 Desc,
		std::optional<D3D12_CLEAR_VALUE> ClearValue = std::nullopt,
		bool							 Cubemap	= false);

	[[nodiscard]] UINT GetSubresourceIndex(
		std::optional<UINT> OptArraySlice = std::nullopt,
		std::optional<UINT> OptMipSlice	  = std::nullopt,
		std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

	void CreateRenderTargetView(
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
		std::optional<UINT>			OptArraySlice = std::nullopt,
		std::optional<UINT>			OptMipSlice	  = std::nullopt,
		std::optional<UINT>			OptArraySize  = std::nullopt,
		bool						sRGB		  = false) const;

	void CreateDepthStencilView(
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
		std::optional<UINT>			OptArraySlice = std::nullopt,
		std::optional<UINT>			OptMipSlice	  = std::nullopt,
		std::optional<UINT>			OptArraySize  = std::nullopt) const;

	[[nodiscard]] bool IsCubemap() const noexcept { return Cubemap; }

private:
	bool Cubemap = false;
};
