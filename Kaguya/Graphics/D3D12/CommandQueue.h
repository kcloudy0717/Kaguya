#pragma once

class CommandAllocatorPool
{
public:
	CommandAllocatorPool() noexcept = default;
	explicit CommandAllocatorPool(_In_ ID3D12Device* pDevice, _In_ D3D12_COMMAND_LIST_TYPE Type) noexcept;

	CommandAllocatorPool(CommandAllocatorPool&&) noexcept = default;
	CommandAllocatorPool& operator=(CommandAllocatorPool&&) noexcept = default;

	CommandAllocatorPool(const CommandAllocatorPool&) = delete;
	CommandAllocatorPool& operator=(const CommandAllocatorPool&) = delete;

	ID3D12CommandAllocator* RequestCommandAllocator(_In_ UINT64 CompletedFenceValue);

	void DiscardCommandAllocator(_In_ UINT64 FenceValue, _In_ ID3D12CommandAllocator* pCommandAllocator);

private:
	ID3D12Device*			m_pDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE m_Type	  = D3D12_COMMAND_LIST_TYPE(-1);

	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_CommandAllocatorPool;
	std::queue<std::pair<UINT64, ID3D12CommandAllocator*>>		m_ReadyCommandAllocators;
	CriticalSection												m_CriticalSection;
};

class CommandQueue
{
public:
	CommandQueue() noexcept = default;
	explicit CommandQueue(_In_ ID3D12Device* pDevice, _In_ D3D12_COMMAND_LIST_TYPE Type);

	CommandQueue(CommandQueue&&) noexcept = default;
	CommandQueue& operator=(CommandQueue&&) noexcept = default;

	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator=(const CommandQueue&) = delete;

						operator ID3D12CommandQueue*() const { return m_CommandQueue.Get(); }
	ID3D12CommandQueue* operator->() const { return m_CommandQueue.Get(); }

	ID3D12CommandAllocator* RequestCommandAllocator(_In_ UINT64 CompletedFenceValue);

	void DiscardCommandAllocator(_In_ UINT64 FenceValue, _In_ ID3D12CommandAllocator* pCommandAllocator);

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
	CommandAllocatorPool					   m_CommandAllocatorPool;
};
