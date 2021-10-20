#include "D3D12Descriptor.h"
#include "D3D12LinkedDevice.h"

D3D12ShaderResourceView::D3D12ShaderResourceView(D3D12LinkedDevice* Device)
	: D3D12View(Device)
{
}

D3D12ShaderResourceView::D3D12ShaderResourceView(D3D12LinkedDevice* Device, ID3D12Resource* Resource)
	: D3D12View(Device)
{
	Descriptor.CreateDefaultView(Resource);
}

D3D12ShaderResourceView::D3D12ShaderResourceView(
	D3D12LinkedDevice*					   Device,
	const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
	ID3D12Resource*						   Resource)
	: D3D12View(Device, Desc)
{
	Descriptor.CreateView(Desc, Resource);
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(D3D12LinkedDevice* Device)
	: D3D12View(Device)
{
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(
	D3D12LinkedDevice* Device,
	ID3D12Resource*	   Resource,
	ID3D12Resource*	   CounterResource /*= nullptr*/)
	: D3D12View(Device)
{
	Descriptor.CreateDefaultView(Resource, CounterResource);
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(
	D3D12LinkedDevice*						Device,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
	ID3D12Resource*							Resource,
	ID3D12Resource*							CounterResource /*= nullptr*/)
	: D3D12View(Device, Desc)
{
	Descriptor.CreateView(Desc, Resource, CounterResource);
}
