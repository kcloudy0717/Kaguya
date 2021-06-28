#include "pch.h"
#include "D3D12Utility.h"

bool CommandSyncPoint::IsValid() const
{
	return Fence != nullptr;
}

UINT64 CommandSyncPoint::GetValue() const
{
	assert(IsValid());
	return Value;
}

bool CommandSyncPoint::IsComplete() const
{
	assert(IsValid());
	return Fence->GetCompletedValue() >= Value;
}

void CommandSyncPoint::WaitForCompletion() const
{
	assert(IsValid());
	Fence->SetEventOnCompletion(Value, nullptr);
}
