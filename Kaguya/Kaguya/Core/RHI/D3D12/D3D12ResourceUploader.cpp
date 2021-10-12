#include "D3D12ResourceUploader.h"
#include "D3D12LinkedDevice.h"

using Microsoft::WRL::ComPtr;

D3D12ResourceUploader::D3D12ResourceUploader(D3D12LinkedDevice* Parent)
	: D3D12LinkedDeviceChild(Parent)
	, CopyContext2(Parent->GetCopyContext2())
{
}

D3D12ResourceUploader::~D3D12ResourceUploader()
{
	if (SyncHandle.IsValid())
	{
		SyncHandle.WaitForCompletion();
	}
}

void D3D12ResourceUploader::Begin()
{
	if (SyncHandle.IsValid() && SyncHandle.IsComplete())
	{
		TrackedResources.clear();
	}

	CopyContext2.OpenCommandList();
}

D3D12SyncHandle D3D12ResourceUploader::End(bool WaitForCompletion)
{
	CopyContext2.CloseCommandList();
	SyncHandle = CopyContext2.Execute(WaitForCompletion);
	return SyncHandle;
}

void D3D12ResourceUploader::Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource)
{
	auto				   NumSubresources = static_cast<UINT>(Subresources.size());
	UINT64				   UploadSize	   = GetRequiredIntermediateSize(Resource, 0, NumSubresources);
	D3D12_HEAP_PROPERTIES  HeapProperties  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC	   ResourceDesc	   = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	ComPtr<ID3D12Resource> UploadResource;
	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources(
		CopyContext2.CommandListHandle.GetGraphicsCommandList6(),
		Resource,
		UploadResource.Get(),
		0,
		0,
		NumSubresources,
		Subresources.data());

	TrackedResources.push_back(std::move(UploadResource));
}

void D3D12ResourceUploader::Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* Resource)
{
	UINT64				   UploadSize	  = GetRequiredIntermediateSize(Resource, 0, 1);
	D3D12_HEAP_PROPERTIES  HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC	   ResourceDesc	  = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	ComPtr<ID3D12Resource> UploadResource;
	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources<1>(
		CopyContext2.CommandListHandle.GetGraphicsCommandList6(),
		Resource,
		UploadResource.Get(),
		0,
		0,
		1,
		&Subresource);

	TrackedResources.push_back(std::move(UploadResource));
}
