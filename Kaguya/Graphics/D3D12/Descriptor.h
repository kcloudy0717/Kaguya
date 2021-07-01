#pragma once
#include "DeviceChild.h"

template<typename ViewDesc>
class Descriptor : public DeviceChild
{
	template<typename ViewDesc>
	struct CreateViewMap;
	template<>
	struct CreateViewMap<D3D12_SHADER_RESOURCE_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateShaderResourceView) Create()
		{
			return &ID3D12Device::CreateShaderResourceView;
		}
	};
	template<>
	struct CreateViewMap<D3D12_RENDER_TARGET_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateRenderTargetView) Create()
		{
			return &ID3D12Device::CreateRenderTargetView;
		}
	};
	template<>
	struct CreateViewMap<D3D12_DEPTH_STENCIL_VIEW_DESC>
	{
		static decltype(&ID3D12Device::CreateDepthStencilView) Create()
		{
			return &ID3D12Device::CreateDepthStencilView;
		}
	};
	template<>
	struct CreateViewMap<D3D12_UNORDERED_ACCESS_VIEW_DESC>
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
		, Index(std::exchange(rvalue.Index, {}))
	{
	}

	Descriptor& operator=(Descriptor&& rvalue) noexcept
	{
		if (this != &rvalue)
		{
			Parent	  = std::exchange(rvalue.Parent, {});
			CPUHandle = std::exchange(rvalue.CPUHandle, {});
			GPUHandle = std::exchange(rvalue.GPUHandle, {});
			Index	  = std::exchange(rvalue.Index, {});
		}
		return *this;
	}

	bool IsValid() const noexcept { return CPUHandle.ptr != NULL; }

	bool IsReferencedByShader() const noexcept
	{
		assert(IsValid());
		return GPUHandle.ptr != NULL;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const noexcept
	{
		assert(IsValid());
		return CPUHandle;
	}
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const noexcept
	{
		assert(IsReferencedByShader());
		return GPUHandle;
	}
	UINT GetIndex() const noexcept
	{
		assert(IsValid());
		return Index;
	}

	void CreateDefaultView(ID3D12Resource* Resource)
	{
		(GetParentDevice()->GetDevice()->*CreateViewMap<ViewDesc>::Create())(Resource, nullptr, CPUHandle);
	}

	void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentDevice()->GetDevice()
			 ->*CreateViewMap<ViewDesc>::Create())(Resource, CounterResource, nullptr, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
	{
		(GetParentDevice()->GetDevice()->*CreateViewMap<ViewDesc>::Create())(Resource, &Desc, CPUHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentDevice()->GetDevice()
			 ->*CreateViewMap<ViewDesc>::Create())(Resource, CounterResource, &Desc, CPUHandle);
	}

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = { NULL };
	UINT						Index	  = 0;

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

	View(View&&) noexcept = default;
	View& operator=(View&&) noexcept = default;

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

	ShaderResourceView(ShaderResourceView&&) noexcept = default;
	ShaderResourceView& operator=(ShaderResourceView&&) noexcept = default;
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

	UnorderedAccessView(UnorderedAccessView&&) noexcept = default;
	UnorderedAccessView& operator=(UnorderedAccessView&&) noexcept = default;
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

	RenderTargetView(RenderTargetView&&) noexcept = default;
	RenderTargetView& operator=(RenderTargetView&&) noexcept = default;
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

	DepthStencilView(DepthStencilView&&) noexcept = default;
	DepthStencilView& operator=(DepthStencilView&&) noexcept = default;
};
