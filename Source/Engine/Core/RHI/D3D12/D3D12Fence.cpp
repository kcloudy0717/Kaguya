#include "D3D12Fence.h"

D3D12Fence::D3D12Fence(D3D12Device* Parent)
	: D3D12DeviceChild(Parent)
{
}

void D3D12Fence::Initialize(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags /*= D3D12_FENCE_FLAG_NONE*/)
{
	LastCompletedValue = InitialValue;
	CurrentValue	   = InitialValue + 1;

	VERIFY_D3D12_API(GetParentDevice()->GetD3D12Device()->CreateFence(InitialValue, Flags, IID_PPV_ARGS(&Fence)));
}

UINT64 D3D12Fence::Signal(D3D12CommandQueue* CommandQueue)
{
	InternalSignal(CommandQueue->GetCommandQueue(), CurrentValue);
	UpdateLastCompletedValue();
	CurrentValue++;
	return LastSignaledValue;
}

bool D3D12Fence::IsFenceComplete(UINT64 Value)
{
	if (Value <= LastCompletedValue)
	{
		return true;
	}

	return Value <= UpdateLastCompletedValue();
}

void D3D12Fence::HostWaitForValue(UINT64 Value)
{
	if (IsFenceComplete(Value))
	{
		return;
	}

	VERIFY_D3D12_API(Fence->SetEventOnCompletion(Value, nullptr));
	UpdateLastCompletedValue();
}

void D3D12Fence::InternalSignal(ID3D12CommandQueue* CommandQueue, UINT64 Value)
{
	VERIFY_D3D12_API(CommandQueue->Signal(Fence.Get(), Value));

	LastSignaledValue = Value;
}

UINT64 D3D12Fence::UpdateLastCompletedValue()
{
	LastCompletedValue = Fence->GetCompletedValue();
	return LastCompletedValue;
}
