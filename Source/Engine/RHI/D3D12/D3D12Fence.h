#pragma once
#include "D3D12Core.h"

namespace RHI
{
	class D3D12Fence : public D3D12DeviceChild
	{
	public:
		D3D12Fence() noexcept = default;
		explicit D3D12Fence(
			D3D12Device*	  Parent,
			UINT64			  InitialValue,
			D3D12_FENCE_FLAGS Flags = D3D12_FENCE_FLAG_NONE);

		D3D12Fence(D3D12Fence&&) noexcept = default;
		D3D12Fence& operator=(D3D12Fence&&) noexcept = default;

		D3D12Fence(const D3D12Fence&) noexcept = delete;
		D3D12Fence& operator=(const D3D12Fence&) noexcept = delete;

		[[nodiscard]] ID3D12Fence1* Get() const noexcept { return Fence.Get(); }

		UINT64 Signal(D3D12CommandQueue* CommandQueue);
		UINT64 Signal(IDStorageQueue* DStorageQueue);

		[[nodiscard]] bool IsFenceComplete(UINT64 Value);

		void HostWaitForValue(UINT64 Value);

	private:
		Arc<ID3D12Fence1> InitializeFence(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags);

		void InternalSignal(ID3D12CommandQueue* CommandQueue, UINT64 Value);
		void InternalSignal(IDStorageQueue* DStorageQueue, UINT64 Value);

		UINT64 UpdateLastCompletedValue();

	private:
		Arc<ID3D12Fence1> Fence;
		UINT64			  CurrentValue		 = 0;
		UINT64			  LastSignaledValue	 = 0;
		UINT64			  LastCompletedValue = 0; // Cached to avoid call to ID3D12Fence::GetCompletedValue
	};
} // namespace RHI
