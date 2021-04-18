#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <queue>
#include <Core/Synchronization/CriticalSection.h>

class CommandAllocatorPool
{
public:
	CommandAllocatorPool() = default;
	CommandAllocatorPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE Type);

	ID3D12CommandAllocator* RequestCommandAllocator(UINT64 CompletedFenceValue);
	void DiscardCommandAllocator(UINT64 FenceValue, ID3D12CommandAllocator* pCommandAllocator);
private:
	ID3D12Device* m_pDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE m_Type = D3D12_COMMAND_LIST_TYPE(-1);

	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>	m_CommandAllocatorPool;
	std::queue<std::pair<UINT64, ID3D12CommandAllocator*>> m_ReadyCommandAllocators;
	CriticalSection m_CriticalSection;
};