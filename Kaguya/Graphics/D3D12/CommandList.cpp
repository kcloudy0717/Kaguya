#include "pch.h"
#include "CommandList.h"

CommandList::CommandList(
	_In_ ID3D12Device4* pDevice,
	_In_ D3D12_COMMAND_LIST_TYPE Type)
{
	/*
	* Known bug in debug layer
	* If GBV is enabled, CreateCommandList1 throws
	*/

	ThrowIfFailed(pDevice->CreateCommandList1(1, Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(m_pCommandList.ReleaseAndGetAddressOf())));
	ThrowIfFailed(pDevice->CreateCommandList1(1, Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(m_pPendingCommandList.ReleaseAndGetAddressOf())));
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Alloc;
	//pDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(&Alloc));
	//ThrowIfFailed(pDevice->CreateCommandList(1, Type, Alloc.Get(), nullptr, IID_PPV_ARGS(pCommandList.ReleaseAndGetAddressOf())));
	//pCommandList->Close();

	//pDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(&Alloc));
	//ThrowIfFailed(pDevice->CreateCommandList(1, Type, Alloc.Get(), nullptr, IID_PPV_ARGS(pPendingCommandList.ReleaseAndGetAddressOf())));
	//pPendingCommandList->Close();
}

bool CommandList::Close(
	_In_ ResourceStateTracker* pGlobalResourceStateTracker)
{
	FlushResourceBarriers();
	ThrowIfFailed(m_pCommandList->Close());

	UINT numPendingBarriers = m_ResourceStateTracker.FlushPendingResourceBarriers(*pGlobalResourceStateTracker, m_pPendingCommandList.Get());
	pGlobalResourceStateTracker->UpdateResourceStates(m_ResourceStateTracker);
	ThrowIfFailed(m_pPendingCommandList->Close());
	return numPendingBarriers > 0;
}

void CommandList::Reset(
	_In_ UINT64 FenceValue,
	_In_ UINT64 CompletedValue,
	_In_ CommandQueue* pCommandQueue)
{
	pCommandQueue->DiscardCommandAllocator(FenceValue, m_pAllocator);
	pCommandQueue->DiscardCommandAllocator(FenceValue, m_pPendingAllocator);

	m_pAllocator = pCommandQueue->RequestCommandAllocator(CompletedValue);
	m_pPendingAllocator = pCommandQueue->RequestCommandAllocator(CompletedValue);

	ThrowIfFailed(m_pCommandList->Reset(m_pAllocator, nullptr));
	ThrowIfFailed(m_pPendingCommandList->Reset(m_pPendingAllocator, nullptr));
	// Reset resource state tracking and resource barriers 
	m_ResourceStateTracker.Reset();
}

void CommandList::SetDescriptorHeaps(
	_In_ const ResourceViewHeaps& ResourceViewHeaps)
{
	ID3D12DescriptorHeap* descriptorHeaps[2] = {
		ResourceViewHeaps.ResourceDescriptorHeap(),
		ResourceViewHeaps.SamplerDescriptorHeap(),
	};

	m_pCommandList->SetDescriptorHeaps(2, descriptorHeaps);
}

void CommandList::TransitionBarrier(
	_In_ ID3D12Resource* pResource,
	_In_ D3D12_RESOURCE_STATES TransitionState,
	_In_ UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	// Transition barriers indicate that a set of subresources transition between different usages.  
	// The caller must specify the before and after usages of the subresources.  
	// The D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES flag is used to transition all subresources in a resource at the same time. 
	m_ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(pResource, D3D12_RESOURCE_STATE_UNKNOWN, TransitionState, Subresource));
}

void CommandList::AliasingBarrier(
	_In_ ID3D12Resource* pBeforeResource,
	_In_ ID3D12Resource* pAfterResource)
{
	// Aliasing barriers indicate a transition between usages of two different resources which have mappings into the same heap.  
	// The application can specify both the before and the after resource.  
	// Note that one or both resources can be NULL (indicating that any tiled resource could cause aliasing). 
	m_ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(pBeforeResource, pAfterResource));
}

void CommandList::UAVBarrier(
	_In_ ID3D12Resource* pResource)
{
	// A UAV barrier indicates that all UAV accesses, both read or write,  
	// to a particular resource must complete between any future UAV accesses, both read or write.  
	// It's not necessary for an app to put a UAV barrier between two draw or dispatch calls that only read from a UAV.  
	// Also, it's not necessary to put a UAV barrier between two draw or dispatch calls that  
	// write to the same UAV if the application knows that it is safe to execute the UAV access in any order.  
	// A D3D12_RESOURCE_UAV_BARRIER structure is used to specify the UAV resource to which the barrier applies.  
	// The application can specify NULL for the barrier's UAV, which indicates that any UAV access could require the barrier. 
	m_ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(pResource));
}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTracker.FlushResourceBarriers(m_pCommandList.Get());
}

void CommandList::DrawInstanced(
	_In_ UINT VertexCount,
	_In_ UINT InstanceCount,
	_In_ UINT StartVertexLocation,
	_In_ UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	m_pCommandList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CommandList::DrawIndexedInstanced(
	_In_ UINT IndexCount,
	_In_ UINT InstanceCount,
	_In_ UINT StartIndexLocation,
	_In_ UINT BaseVertexLocation,
	_In_ UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	m_pCommandList->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CommandList::Dispatch(
	_In_ UINT ThreadGroupCountX,
	_In_ UINT ThreadGroupCountY,
	_In_ UINT ThreadGroupCountZ)
{
	FlushResourceBarriers();
	m_pCommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void CommandList::DispatchRays(
	_In_ const D3D12_DISPATCH_RAYS_DESC* pDesc)
{
	FlushResourceBarriers();
	m_pCommandList->DispatchRays(pDesc);
}

void CommandList::DispatchMesh(
	_In_  UINT ThreadGroupCountX,
	_In_  UINT ThreadGroupCountY,
	_In_  UINT ThreadGroupCountZ)
{
	FlushResourceBarriers();
	m_pCommandList->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}
