#pragma once
#include "D3D12Common.h"

template<typename ViewDesc>
class Descriptor : public DeviceChild
{
	template<typename ViewDesc>
	struct CreateViewTraits;
	template<>
	struct CreateViewTraits<D3D12_SHADER_RESOURCE_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateShaderResourceView) Create()
		{
			return &ID3D12Device::CreateShaderResourceView;
		}
	};
	template<>
	struct CreateViewTraits<D3D12_RENDER_TARGET_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateRenderTargetView) Create()
		{
			return &ID3D12Device::CreateRenderTargetView;
		}
	};
	template<>
	struct CreateViewTraits<D3D12_DEPTH_STENCIL_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateDepthStencilView) Create()
		{
			return &ID3D12Device::CreateDepthStencilView;
		}
	};
	template<>
	struct CreateViewTraits<D3D12_UNORDERED_ACCESS_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateUnorderedAccessView) Create()
		{
			return &ID3D12Device::CreateUnorderedAccessView;
		}
	};

public:
	Descriptor() noexcept = default;

	Descriptor(Device* Device)
		: DeviceChild(Device)
	{
		Allocate();
	}

	~Descriptor() { Release(); }

	Descriptor(Descriptor&& rvalue) noexcept
		: DeviceChild(std::exchange(rvalue.Parent, {}))
		, CPUHandle(std::exchange(rvalue.CPUHandle, {}))
		, GPUHandle(std::exchange(rvalue.GPUHandle, {}))
		, Index(std::exchange(rvalue.Index, UINT_MAX))
	{
	}

	Descriptor& operator=(Descriptor&& rvalue) noexcept
	{
		if (this != &rvalue)
		{
			Parent	  = std::exchange(rvalue.Parent, {});
			CPUHandle = std::exchange(rvalue.CPUHandle, {});
			GPUHandle = std::exchange(rvalue.GPUHandle, {});
			Index	  = std::exchange(rvalue.Index, UINT_MAX);
		}
		return *this;
	}

	NONCOPYABLE(Descriptor);

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
		(GetParentDevice()->GetDevice()->*CreateViewTraits<ViewDesc>::Create())(Resource, nullptr, CPUHandle);
	}

	void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentDevice()->GetDevice()
			 ->*CreateViewTraits<ViewDesc>::Create())(Resource, CounterResource, nullptr, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
	{
		(GetParentDevice()->GetDevice()->*CreateViewTraits<ViewDesc>::Create())(Resource, &Desc, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentDevice()->GetDevice()
			 ->*CreateViewTraits<ViewDesc>::Create())(Resource, CounterResource, &Desc, CPUHandle);
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
class View
{
public:
	View() noexcept = default;

	View(Device* Device)
		: Descriptor(Device)
		, Desc{}
	{
	}

	View(Device* Device, const ViewDesc& Desc)
		: Descriptor(Device)
		, Desc(Desc)
	{
	}

	DEFAULTMOVABLE(View);
	NONCOPYABLE(View);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const noexcept { return Descriptor.GetCPUHandle(); }
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const noexcept { return Descriptor.GetGPUHandle(); }
	UINT							   GetIndex() const noexcept { return Descriptor.GetIndex(); }

public:
	ViewDesc			 Desc = {};
	Descriptor<ViewDesc> Descriptor;
};

class ShaderResourceView : public View<D3D12_SHADER_RESOURCE_VIEW_DESC>
{
public:
	ShaderResourceView() noexcept = default;

	ShaderResourceView(Device* Device)
		: View(Device)
	{
	}

	ShaderResourceView(Device* Device, ID3D12Resource* Resource);

	ShaderResourceView(Device* Device, const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc, ID3D12Resource* Resource);

	DEFAULTMOVABLE(ShaderResourceView);
	NONCOPYABLE(ShaderResourceView);
};

class UnorderedAccessView : public View<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
public:
	UnorderedAccessView() noexcept = default;

	UnorderedAccessView(Device* Device)
		: View(Device)
	{
	}

	UnorderedAccessView(Device* Device, ID3D12Resource* Resource, ID3D12Resource* CounterResource = nullptr);

	UnorderedAccessView(
		Device*									Device,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
		ID3D12Resource*							Resource,
		ID3D12Resource*							CounterResource = nullptr);

	DEFAULTMOVABLE(UnorderedAccessView);
	NONCOPYABLE(UnorderedAccessView);
};

class RenderTargetView : public View<D3D12_RENDER_TARGET_VIEW_DESC>
{
public:
	RenderTargetView() noexcept = default;

	RenderTargetView(Device* Device)
		: View(Device)
	{
	}

	RenderTargetView(Device* Device, ID3D12Resource* Resource);

	RenderTargetView(Device* Device, const D3D12_RENDER_TARGET_VIEW_DESC& Desc, ID3D12Resource* Resource);

	DEFAULTMOVABLE(RenderTargetView);
	NONCOPYABLE(RenderTargetView);
};

class DepthStencilView : public View<D3D12_DEPTH_STENCIL_VIEW_DESC>
{
public:
	DepthStencilView() noexcept = default;

	DepthStencilView(Device* Device)
		: View(Device)
	{
	}

	DepthStencilView(Device* Device, ID3D12Resource* Resource);

	DepthStencilView(Device* Device, const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc, ID3D12Resource* Resource);

	DEFAULTMOVABLE(DepthStencilView);
	NONCOPYABLE(DepthStencilView);
};
