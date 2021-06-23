#include "pch.h"
#include "Fence.h"

Fence::Fence(_In_ ID3D12Device* pDevice)
{
	ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
}
