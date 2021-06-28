#include "pch.h"
#include "Descriptor.h"
#include "Device.h"

ShaderResourceView::ShaderResourceView(Device* Device, ID3D12Resource* Resource)
	: View(Device)
{
	Descriptor.CreateDefaultView(Resource);
}

ShaderResourceView::ShaderResourceView(
	Device*								   Device,
	const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
	ID3D12Resource*						   Resource)
	: View(Device, Desc)
{
	Descriptor.CreateView(Desc, Resource);
}

UnorderedAccessView::UnorderedAccessView(
	Device*			Device,
	ID3D12Resource* Resource,
	ID3D12Resource* CounterResource /*= nullptr*/)
	: View(Device)
{
	Descriptor.CreateDefaultView(Resource, CounterResource);
}

UnorderedAccessView::UnorderedAccessView(
	Device*									Device,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
	ID3D12Resource*							Resource,
	ID3D12Resource*							CounterResource /*= nullptr*/)
	: View(Device)
{
	Descriptor.CreateView(Desc, Resource, CounterResource);
}

RenderTargetView::RenderTargetView(Device* Device, ID3D12Resource* Resource)
	: View(Device)
{
	Descriptor.CreateDefaultView(Resource);
}

RenderTargetView::RenderTargetView(Device* Device, const D3D12_RENDER_TARGET_VIEW_DESC& Desc, ID3D12Resource* Resource)
	: View(Device)
{
	Descriptor.CreateView(Desc, Resource);
}

DepthStencilView::DepthStencilView(Device* Device, ID3D12Resource* Resource)
	: View(Device)
{
	Descriptor.CreateDefaultView(Resource);
}

DepthStencilView::DepthStencilView(Device* Device, const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc, ID3D12Resource* Resource)
	: View(Device)
{
	Descriptor.CreateView(Desc, Resource);
}
