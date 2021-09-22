#pragma once
#include "D3D12Common.h"

template<typename ViewDesc>
class D3D12Descriptor : public D3D12LinkedDeviceChild
{
	// clang-format off
	template<typename ViewDesc> struct Traits;
	template<> struct Traits<D3D12_SHADER_RESOURCE_VIEW_DESC>	{ static decltype(&ID3D12Device::CreateShaderResourceView) Create() { return &ID3D12Device::CreateShaderResourceView; } };
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
		, CpuHandle(std::exchange(D3D12Descriptor.CpuHandle, {}))
		, GpuHandle(std::exchange(D3D12Descriptor.GpuHandle, {}))
		, Index(std::exchange(D3D12Descriptor.Index, UINT_MAX))
	{
	}

	D3D12Descriptor& operator=(D3D12Descriptor&& D3D12Descriptor) noexcept
	{
		if (this != &D3D12Descriptor)
		{
			Parent	  = std::exchange(D3D12Descriptor.Parent, {});
			CpuHandle = std::exchange(D3D12Descriptor.CpuHandle, {});
			GpuHandle = std::exchange(D3D12Descriptor.GpuHandle, {});
			Index	  = std::exchange(D3D12Descriptor.Index, UINT_MAX);
		}
		return *this;
	}

	NONCOPYABLE(D3D12Descriptor);

	[[nodiscard]] bool						  IsValid() const noexcept { return Index != UINT_MAX; }
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept
	{
		assert(IsValid());
		return CpuHandle;
	}
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept
	{
		assert(IsValid());
		return GpuHandle;
	}
	[[nodiscard]] UINT GetIndex() const noexcept
	{
		assert(IsValid());
		return Index;
	}

	void CreateDefaultView(ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*Traits<ViewDesc>::Create())(Resource, nullptr, CpuHandle);
	}

	void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()
			 ->*Traits<ViewDesc>::Create())(Resource, CounterResource, nullptr, CpuHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*Traits<ViewDesc>::Create())(Resource, &Desc, CpuHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()
			 ->*Traits<ViewDesc>::Create())(Resource, CounterResource, &Desc, CpuHandle);
	}

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { NULL };
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
	{
	}

	D3D12View(D3D12LinkedDevice* Device, const ViewDesc& Desc)
		: Desc(Desc)
		, Descriptor(Device)
	{
	}

	DEFAULTMOVABLE(D3D12View);
	NONCOPYABLE(D3D12View);

	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept { return Descriptor.GetCpuHandle(); }
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return Descriptor.GetGpuHandle(); }
	[[nodiscard]] UINT						  GetIndex() const noexcept { return Descriptor.GetIndex(); }

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
