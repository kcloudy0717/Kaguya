#include "pch.h"
#include "ResourceUploader.h"
#include "Device.h"

using Microsoft::WRL::ComPtr;

ResourceUploader::ResourceUploader(Device* Device)
	: DeviceChild(Device)
	, CopyContext(Device->GetCopyContext2())
{
	m_LinearAllocator = std::make_unique<LinearAllocator>(Device->GetDevice());
}

void ResourceUploader::Begin()
{
	CopyContext.OpenCommandList();

	m_LinearAllocator->Begin(GetParentDevice()->GetCopyQueue2()->GetCompletedValue());
}

CommandSyncPoint ResourceUploader::End(bool WaitForCompletion)
{
	CopyContext.CloseCommandList();
	CommandSyncPoint SyncPoint = CopyContext.Execute(WaitForCompletion);

	m_LinearAllocator->End(SyncPoint.GetValue());

	return SyncPoint;
}

void ResourceUploader::Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* pResource)
{
	const UINT	 NumSubresources = static_cast<UINT>(Subresources.size());
	const UINT64 UploadSize		 = GetRequiredIntermediateSize(pResource, 0, NumSubresources);
	Allocation	 Allocation		 = m_LinearAllocator->Allocate(UploadSize, 512);

	UpdateSubresources(
		CopyContext.CommandListHandle.GetGraphicsCommandList6(),
		pResource,
		Allocation.pResource,
		Allocation.Offset,
		0,
		NumSubresources,
		Subresources.data());
}

void ResourceUploader::Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* pResource)
{
	const UINT64 UploadSize = GetRequiredIntermediateSize(pResource, 0, 1);
	Allocation	 Allocation = m_LinearAllocator->Allocate(UploadSize);

	UpdateSubresources<1>(
		CopyContext.CommandListHandle.GetGraphicsCommandList6(),
		pResource,
		Allocation.pResource,
		Allocation.Offset,
		0,
		1,
		&Subresource);
}
