#include "pch.h"
#include "CommandList.h"

CommandList::CommandList(ID3D12Device4* pDevice, D3D12_COMMAND_LIST_TYPE Type)
{
	/*
	* Known bug in debug layer
	* If GBV is enabled, CreateCommandList1 throws
	*/

	ThrowIfFailed(pDevice->CreateCommandList1(1, Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(pCommandList.ReleaseAndGetAddressOf())));
	ThrowIfFailed(pDevice->CreateCommandList1(1, Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(pPendingCommandList.ReleaseAndGetAddressOf())));
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Alloc;
	//pDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(&Alloc));
	//ThrowIfFailed(pDevice->CreateCommandList(1, Type, Alloc.Get(), nullptr, IID_PPV_ARGS(pCommandList.ReleaseAndGetAddressOf())));
	//pCommandList->Close();

	//pDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(&Alloc));
	//ThrowIfFailed(pDevice->CreateCommandList(1, Type, Alloc.Get(), nullptr, IID_PPV_ARGS(pPendingCommandList.ReleaseAndGetAddressOf())));
	//pPendingCommandList->Close();
}

bool CommandList::Close(::ResourceStateTracker* pGlobalResourceStateTracker)
{
	FlushResourceBarriers();
	ThrowIfFailed(pCommandList->Close());

	UINT numPendingBarriers = ResourceStateTracker.FlushPendingResourceBarriers(*pGlobalResourceStateTracker, pPendingCommandList.Get());
	pGlobalResourceStateTracker->UpdateResourceStates(ResourceStateTracker);
	ThrowIfFailed(pPendingCommandList->Close());
	return numPendingBarriers > 0;
}

void CommandList::Reset(UINT64 FenceValue, UINT64 CompletedValue, CommandQueue* pCommandQueue)
{
	pCommandQueue->DiscardCommandAllocator(FenceValue, pAllocator);
	pCommandQueue->DiscardCommandAllocator(FenceValue, pPendingAllocator);

	pAllocator = pCommandQueue->RequestCommandAllocator(CompletedValue);
	pPendingAllocator = pCommandQueue->RequestCommandAllocator(CompletedValue);

	ThrowIfFailed(pCommandList->Reset(pAllocator, nullptr));
	ThrowIfFailed(pPendingCommandList->Reset(pPendingAllocator, nullptr));
	// Reset resource state tracking and resource barriers 
	ResourceStateTracker.Reset();
}

void CommandList::SetDescriptorHeaps(DescriptorHeap* pCBSRUADescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap)
{
	UINT numDescriptorHeaps = 0;
	ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
	if (pCBSRUADescriptorHeap)
	{
		descriptorHeaps[numDescriptorHeaps++] = *pCBSRUADescriptorHeap;
	}
	if (pSamplerDescriptorHeap)
	{
		descriptorHeaps[numDescriptorHeaps++] = *pSamplerDescriptorHeap;
	}

	if (numDescriptorHeaps > 0)
	{
		pCommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
	}
}

void CommandList::TransitionBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES TransitionState, UINT Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	// Transition barriers indicate that a set of subresources transition between different usages.  
	// The caller must specify the before and after usages of the subresources.  
	// The D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES flag is used to transition all subresources in a resource at the same time. 
	ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(pResource, D3D12_RESOURCE_STATE_UNKNOWN, TransitionState, Subresource));
}

void CommandList::AliasingBarrier(ID3D12Resource* pBeforeResource, ID3D12Resource* pAfterResource)
{
	// Aliasing barriers indicate a transition between usages of two different resources which have mappings into the same heap.  
	// The application can specify both the before and the after resource.  
	// Note that one or both resources can be NULL (indicating that any tiled resource could cause aliasing). 
	ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::Aliasing(pBeforeResource, pAfterResource));
}

void CommandList::UAVBarrier(ID3D12Resource* pResource)
{
	// A UAV barrier indicates that all UAV accesses, both read or write,  
	// to a particular resource must complete between any future UAV accesses, both read or write.  
	// It's not necessary for an app to put a UAV barrier between two draw or dispatch calls that only read from a UAV.  
	// Also, it's not necessary to put a UAV barrier between two draw or dispatch calls that  
	// write to the same UAV if the application knows that it is safe to execute the UAV access in any order.  
	// A D3D12_RESOURCE_UAV_BARRIER structure is used to specify the UAV resource to which the barrier applies.  
	// The application can specify NULL for the barrier's UAV, which indicates that any UAV access could require the barrier. 
	ResourceStateTracker.ResourceBarrier(CD3DX12_RESOURCE_BARRIER::UAV(pResource));
}

void CommandList::FlushResourceBarriers()
{
	ResourceStateTracker.FlushResourceBarriers(pCommandList.Get());
}

void CommandList::DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	pCommandList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void CommandList::DrawIndexedInstanced(UINT IndexCount, UINT InstanceCount, UINT StartIndexLocation, UINT BaseVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	pCommandList->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void CommandList::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	FlushResourceBarriers();
	pCommandList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void CommandList::DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc)
{
	FlushResourceBarriers();
	pCommandList->DispatchRays(pDesc);
}

void CommandList::DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	FlushResourceBarriers();
	pCommandList->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}