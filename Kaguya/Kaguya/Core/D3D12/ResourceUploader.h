#pragma once
#include "D3D12Common.h"
#include "CommandContext.h"
#include "CommandQueue.h"
#include "MemoryAllocator.h"

class ResourceUploader : public DeviceChild
{
public:
	ResourceUploader(Device* Parent);
	~ResourceUploader();

	void Begin();

	// GpuSyncPoint can be ignored if WaitForCompletion is true
	CommandSyncPoint End(bool WaitForCompletion);

	void Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* pResource);

	void Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* pResource);

private:
	CommandContext&										CopyContext2;
	CommandSyncPoint									SyncPoint;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> TrackedResources;
};
