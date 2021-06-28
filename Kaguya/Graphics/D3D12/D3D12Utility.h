#pragma once
#include "d3dx12.h"

// Represents a Fence and Value pair, similar to that of a coroutine handle
// you can query the status of a command execution point and wait for it
class CommandSyncPoint
{
public:
	CommandSyncPoint()
		: Fence(nullptr)
		, Value(0)
	{
	}
	CommandSyncPoint(ID3D12Fence* Fence, UINT64 Value)
		: Fence(Fence)
		, Value(Value)
	{
	}

	bool   IsValid() const;
	UINT64 GetValue() const;
	bool   IsComplete() const;
	void   WaitForCompletion() const;

private:
	friend class CommandQueue;

	ID3D12Fence* Fence;
	UINT64		 Value;
};

inline DXGI_FORMAT GetValidDepthStencilViewFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		return Format;
	};
}

inline DXGI_FORMAT GetValidSRVFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	default:
		return Format;
	}
};
