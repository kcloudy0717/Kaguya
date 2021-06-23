#include "pch.h"
#include "CommandQueue.h"

CommandAllocatorPool::CommandAllocatorPool(_In_ ID3D12Device* pDevice, _In_ D3D12_COMMAND_LIST_TYPE Type) noexcept
	: m_pDevice(pDevice)
	, m_Type(Type)
{
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestCommandAllocator(_In_ UINT64 CompletedFenceValue)
{
	std::scoped_lock _(m_CriticalSection);

	ID3D12CommandAllocator* pCommandAllocator = nullptr;

	if (!m_ReadyCommandAllocators.empty())
	{
		// First indicates the fence value of the command allocator that was being used
		// if that value is less than the CompletedFenceValue then that allocator is ready to
		// be used again
		if (auto iter = m_ReadyCommandAllocators.front(); iter.first <= CompletedFenceValue)
		{
			pCommandAllocator = iter.second;
			ThrowIfFailed(pCommandAllocator->Reset());
			m_ReadyCommandAllocators.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (!pCommandAllocator)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
		ThrowIfFailed(
			m_pDevice->CreateCommandAllocator(m_Type, IID_PPV_ARGS(CommandAllocator.ReleaseAndGetAddressOf())));
		wchar_t AllocatorName[32];
		swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_CommandAllocatorPool.size());
		CommandAllocator->SetName(AllocatorName);
		pCommandAllocator = CommandAllocator.Get();
		m_CommandAllocatorPool.push_back(CommandAllocator);
	}

	return pCommandAllocator;
}

void CommandAllocatorPool::DiscardCommandAllocator(
	_In_ UINT64 FenceValue,
	_In_ ID3D12CommandAllocator* pCommandAllocator)
{
	if (!pCommandAllocator)
	{
		return;
	}

	std::scoped_lock _(m_CriticalSection);
	// That fence value indicates we are free to reset the allocator
	m_ReadyCommandAllocators.push(std::make_pair(FenceValue, pCommandAllocator));
}

CommandQueue::CommandQueue(_In_ ID3D12Device* pDevice, _In_ D3D12_COMMAND_LIST_TYPE Type)
	: m_CommandAllocatorPool(pDevice, Type)
{
	D3D12_COMMAND_QUEUE_DESC desc = { .Type		= Type,
									  .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
									  .Flags	= D3D12_COMMAND_QUEUE_FLAG_NONE,
									  .NodeMask = 0 };
	ThrowIfFailed(pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue)));
}

ID3D12CommandAllocator* CommandQueue::RequestCommandAllocator(_In_ UINT64 CompletedFenceValue)
{
	return m_CommandAllocatorPool.RequestCommandAllocator(CompletedFenceValue);
}

void CommandQueue::DiscardCommandAllocator(_In_ UINT64 FenceValue, _In_ ID3D12CommandAllocator* pCommandAllocator)
{
	m_CommandAllocatorPool.DiscardCommandAllocator(FenceValue, pCommandAllocator);
}
