#include "pch.h"
#include "CommandQueue.h"

CommandQueue::CommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE Type)
	: m_CommandAllocatorPool(pDevice, Type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = Type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_CommandQueue.ReleaseAndGetAddressOf())));
}

ID3D12CommandAllocator* CommandQueue::RequestCommandAllocator(UINT64 CompletedValue)
{
	return m_CommandAllocatorPool.RequestCommandAllocator(CompletedValue);
}

void CommandQueue::DiscardCommandAllocator(UINT64 FenceValue, ID3D12CommandAllocator* pCommandAllocator)
{
	m_CommandAllocatorPool.DiscardCommandAllocator(FenceValue, pCommandAllocator);
}