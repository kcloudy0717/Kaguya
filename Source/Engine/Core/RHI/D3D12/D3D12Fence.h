#pragma once
#include "D3D12Common.h"

class D3D12Fence : public D3D12DeviceChild
{
public:
	explicit D3D12Fence(D3D12Device* Parent);

	void Initialize(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags = D3D12_FENCE_FLAG_NONE);

	[[nodiscard]] ID3D12Fence1* GetApiHandle() const noexcept { return Fence.Get(); }

	UINT64 Signal(D3D12CommandQueue* CommandQueue);

	[[nodiscard]] bool IsFenceComplete(UINT64 Value);

	void HostWaitForValue(UINT64 Value);

private:
	void InternalSignal(ID3D12CommandQueue* CommandQueue, UINT64 Value);

	UINT64 UpdateLastCompletedValue();

private:
	Microsoft::WRL::ComPtr<ID3D12Fence1> Fence;
	UINT64								 CurrentValue	   = 0;
	UINT64								 LastSignaledValue = 0;
	// Cached to avoid call to ID3D12Fence::GetCompletedValue
	UINT64 LastCompletedValue = 0;
};
