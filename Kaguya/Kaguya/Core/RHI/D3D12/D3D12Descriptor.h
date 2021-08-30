#pragma once
#include "D3D12Common.h"

template<typename ViewDesc>
class D3D12Descriptor : public D3D12LinkedDeviceChild
{
	// clang-format off
	template<typename ViewDesc> struct Traits;
	template<> struct Traits<D3D12_SHADER_RESOURCE_VIEW_DESC>	{ static decltype(&ID3D12Device::CreateShaderResourceView) Create() { return &ID3D12Device::CreateShaderResourceView; } };
	template<> struct Traits<D3D12_RENDER_TARGET_VIEW_DESC>		{ static decltype(&ID3D12Device::CreateRenderTargetView) Create() { return &ID3D12Device::CreateRenderTargetView; } };
	template<> struct Traits<D3D12_DEPTH_STENCIL_VIEW_DESC>		{ static decltype(&ID3D12Device::CreateDepthStencilView) Create() { return &ID3D12Device::CreateDepthStencilView; } };
	template<> struct Traits<D3D12_UNORDERED_ACCESS_VIEW_DESC>	{ static decltype(&ID3D12Device::CreateUnorderedAccessView) Create() { return &ID3D12Device::CreateUnorderedAccessView; } };
	// clang-format on

public:
	D3D12Descriptor() noexcept = default;

	D3D12Descriptor(D3D12LinkedDevice* Parent)
		: D3D12LinkedDeviceChild(Parent)
	{
		Allocate();
	}

	~D3D12Descriptor() { Release(); }

	D3D12Descriptor(D3D12Descriptor&& D3D12Descriptor) noexcept
		: D3D12LinkedDeviceChild(std::exchange(D3D12Descriptor.Parent, {}))
		, CPUHandle(std::exchange(D3D12Descriptor.CPUHandle, {}))
		, GPUHandle(std::exchange(D3D12Descriptor.GPUHandle, {}))
		, Index(std::exchange(D3D12Descriptor.Index, UINT_MAX))
	{
	}

	D3D12Descriptor& operator=(D3D12Descriptor&& D3D12Descriptor) noexcept
	{
		if (this != &D3D12Descriptor)
		{
			Parent	  = std::exchange(D3D12Descriptor.Parent, {});
			CPUHandle = std::exchange(D3D12Descriptor.CPUHandle, {});
			GPUHandle = std::exchange(D3D12Descriptor.GPUHandle, {});
			Index	  = std::exchange(D3D12Descriptor.Index, UINT_MAX);
		}
		return *this;
	}

	NONCOPYABLE(D3D12Descriptor);

	bool IsValid() const noexcept { return Index != UINT_MAX; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const noexcept
	{
		assert(IsValid());
		return CPUHandle;
	}
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const noexcept
	{
		assert(IsValid());
		return GPUHandle;
	}
	UINT GetIndex() const noexcept
	{
		assert(IsValid());
		return Index;
	}

	void CreateDefaultView(ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*Traits<ViewDesc>::Create())(Resource, nullptr, CPUHandle);
	}

	void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()
			 ->*Traits<ViewDesc>::Create())(Resource, CounterResource, nullptr, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*Traits<ViewDesc>::Create())(Resource, &Desc, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()
			 ->*Traits<ViewDesc>::Create())(Resource, CounterResource, &Desc, CPUHandle);
	}

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = { NULL };
	UINT						Index	  = UINT_MAX;

private:
	inline void Allocate();
	inline void Release();
};

template<typename ViewDesc>
class D3D12View
{
public:
	D3D12View() noexcept = default;

	D3D12View(D3D12LinkedDevice* Device)
		: Descriptor(Device)
		, Desc{}
	{
	}

	D3D12View(D3D12LinkedDevice* Device, const ViewDesc& Desc)
		: Descriptor(Device)
		, Desc(Desc)
	{
	}

	DEFAULTMOVABLE(D3D12View);
	NONCOPYABLE(D3D12View);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const noexcept { return Descriptor.GetCPUHandle(); }
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const noexcept { return Descriptor.GetGPUHandle(); }
	UINT							   GetIndex() const noexcept { return Descriptor.GetIndex(); }

public:
	ViewDesc				  Desc = {};
	D3D12Descriptor<ViewDesc> Descriptor;
};

class D3D12ShaderResourceView : public D3D12View<D3D12_SHADER_RESOURCE_VIEW_DESC>
{
public:
	D3D12ShaderResourceView() noexcept = default;

	D3D12ShaderResourceView(D3D12LinkedDevice* Device)
		: D3D12View(Device)
	{
	}

	D3D12ShaderResourceView(D3D12LinkedDevice* Device, ID3D12Resource* Resource);

	D3D12ShaderResourceView(
		D3D12LinkedDevice*					   Device,
		const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
		ID3D12Resource*						   Resource);

	DEFAULTMOVABLE(D3D12ShaderResourceView);
	NONCOPYABLE(D3D12ShaderResourceView);
};

class D3D12UnorderedAccessView : public D3D12View<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
public:
	D3D12UnorderedAccessView() noexcept = default;

	D3D12UnorderedAccessView(D3D12LinkedDevice* Device)
		: D3D12View(Device)
	{
	}

	D3D12UnorderedAccessView(
		D3D12LinkedDevice* Device,
		ID3D12Resource*	   Resource,
		ID3D12Resource*	   CounterResource = nullptr);

	D3D12UnorderedAccessView(
		D3D12LinkedDevice*						Device,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
		ID3D12Resource*							Resource,
		ID3D12Resource*							CounterResource = nullptr);

	DEFAULTMOVABLE(D3D12UnorderedAccessView);
	NONCOPYABLE(D3D12UnorderedAccessView);
};

class D3D12RenderTargetView : public D3D12View<D3D12_RENDER_TARGET_VIEW_DESC>
{
public:
	D3D12RenderTargetView() noexcept = default;

	D3D12RenderTargetView(D3D12LinkedDevice* Device)
		: D3D12View(Device)
	{
	}

	D3D12RenderTargetView(D3D12LinkedDevice* Device, ID3D12Resource* Resource);

	D3D12RenderTargetView(
		D3D12LinkedDevice*					 Device,
		const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
		ID3D12Resource*						 Resource);

	DEFAULTMOVABLE(D3D12RenderTargetView);
	NONCOPYABLE(D3D12RenderTargetView);
};

class D3D12DepthStencilView : public D3D12View<D3D12_DEPTH_STENCIL_VIEW_DESC>
{
public:
	D3D12DepthStencilView() noexcept = default;

	D3D12DepthStencilView(D3D12LinkedDevice* Device)
		: D3D12View(Device)
	{
	}

	D3D12DepthStencilView(D3D12LinkedDevice* Device, ID3D12Resource* Resource);

	D3D12DepthStencilView(
		D3D12LinkedDevice*					 Device,
		const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
		ID3D12Resource*						 Resource);

	DEFAULTMOVABLE(D3D12DepthStencilView);
	NONCOPYABLE(D3D12DepthStencilView);
};
