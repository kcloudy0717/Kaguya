#pragma once
#include "D3D12Common.h"
#include "D3D12CommandContext.h"

class D3D12ResourceUploader : public D3D12LinkedDeviceChild
{
public:
	D3D12ResourceUploader(D3D12LinkedDevice* Parent);
	~D3D12ResourceUploader();

	void Begin();

	D3D12CommandSyncPoint End(bool WaitForCompletion);

	void Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource);
	void Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* Resource);

private:
	D3D12CommandContext&								CopyContext2;
	D3D12CommandSyncPoint								SyncPoint;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> TrackedResources;
};
