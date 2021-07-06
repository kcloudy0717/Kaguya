#include "pch.h"
#include "ResourceUploader.h"
#include "Device.h"

using Microsoft::WRL::ComPtr;

ResourceUploader::ResourceUploader(Device* Device)
	: DeviceChild(Device)
	, CopyContext(Device->GetCopyContext2())
{
}

ResourceUploader::~ResourceUploader()
{
	if (SyncPoint.IsValid())
	{
		SyncPoint.WaitForCompletion();
	}
}

void ResourceUploader::Begin()
{
	if (SyncPoint.IsValid() && SyncPoint.IsComplete())
	{
		TrackedResources.clear();
	}

	CopyContext.OpenCommandList();
}

CommandSyncPoint ResourceUploader::End(bool WaitForCompletion)
{
	CopyContext.CloseCommandList();
	SyncPoint = CopyContext.Execute(WaitForCompletion);
	return SyncPoint;
}

void ResourceUploader::Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* pResource)
{
	const UINT							   NumSubresources = static_cast<UINT>(Subresources.size());
	const UINT64						   UploadSize	   = GetRequiredIntermediateSize(pResource, 0, NumSubresources);
	const D3D12_HEAP_PROPERTIES			   HeapProperties  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC			   ResourceDesc	   = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadResource;
	ThrowIfFailed(GetParentDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources(
		CopyContext.CommandListHandle.GetGraphicsCommandList6(),
		pResource,
		UploadResource.Get(),
		0,
		0,
		NumSubresources,
		Subresources.data());

	TrackedResources.push_back(std::move(UploadResource));
}

void ResourceUploader::Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* pResource)
{
	const UINT64						   UploadSize	  = GetRequiredIntermediateSize(pResource, 0, 1);
	const D3D12_HEAP_PROPERTIES			   HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC			   ResourceDesc	  = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadResource;
	ThrowIfFailed(GetParentDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources<1>(
		CopyContext.CommandListHandle.GetGraphicsCommandList6(),
		pResource,
		UploadResource.Get(),
		0,
		0,
		1,
		&Subresource);

	TrackedResources.push_back(std::move(UploadResource));
}
