#include "D3D12CommandContext.h"
#include "D3D12LinkedDevice.h"

D3D12CommandContext::D3D12CommandContext(
	D3D12LinkedDevice*		Parent,
	ED3D12CommandQueueType	Type,
	D3D12_COMMAND_LIST_TYPE CommandListType)
	: D3D12LinkedDeviceChild(Parent)
	, Type(Type)
	, CommandListHandle(Parent, CommandListType, Parent->GetCommandQueue(Type))
	, CommandAllocator(nullptr)
	, CommandAllocatorPool(Parent, CommandListType)
	, CpuConstantAllocator(Parent)
{
}

D3D12CommandQueue* D3D12CommandContext::GetCommandQueue()
{
	return GetParentLinkedDevice()->GetCommandQueue(Type);
}

void D3D12CommandContext::OpenCommandList()
{
	if (!CommandAllocator)
	{
		CommandAllocator = CommandAllocatorPool.RequestCommandAllocator();
	}

	CommandListHandle = GetCommandQueue()->RequestCommandList(CommandAllocator);

	ID3D12DescriptorHeap* DescriptorHeaps[2] = {
		GetParentLinkedDevice()->GetResourceDescriptorHeap(),
		GetParentLinkedDevice()->GetSamplerDescriptorHeap(),
	};

	CommandListHandle->SetDescriptorHeaps(2, DescriptorHeaps);
}

void D3D12CommandContext::CloseCommandList()
{
	CommandListHandle.Close();
}

D3D12CommandSyncPoint D3D12CommandContext::Execute(bool WaitForCompletion)
{
	D3D12CommandQueue* CommandQueue = GetCommandQueue();
	CommandQueue->ExecuteCommandLists(1, &CommandListHandle, WaitForCompletion);

	assert(CommandAllocator && "Invalid CommandAllocator");

	D3D12CommandSyncPoint SyncPoint = CommandAllocator->GetSyncPoint();
	// Release the command allocator so it can be reused.
	CommandAllocatorPool.DiscardCommandAllocator(CommandAllocator);
	CommandAllocator = nullptr;

	CpuConstantAllocator.Version(SyncPoint);
	return SyncPoint;
}

void D3D12CommandContext::TransitionBarrier(
	D3D12Resource*		  Resource,
	D3D12_RESOURCE_STATES State,
	UINT				  Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	CommandListHandle.TransitionBarrier(Resource, State, Subresource);
}

void D3D12CommandContext::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
{
	CommandListHandle.AliasingBarrier(BeforeResource, AfterResource);
}

void D3D12CommandContext::UAVBarrier(D3D12Resource* Resource)
{
	CommandListHandle.UAVBarrier(Resource);
}

void D3D12CommandContext::FlushResourceBarriers()
{
	CommandListHandle.FlushResourceBarriers();
}

void D3D12CommandContext::DrawInstanced(
	UINT VertexCount,
	UINT InstanceCount,
	UINT StartVertexLocation,
	UINT StartInstanceLocation)
{
	CommandListHandle.FlushResourceBarriers();
	CommandListHandle->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void D3D12CommandContext::DrawIndexedInstanced(
	UINT IndexCount,
	UINT InstanceCount,
	UINT StartIndexLocation,
	UINT BaseVertexLocation,
	UINT StartInstanceLocation)
{
	CommandListHandle.FlushResourceBarriers();
	CommandListHandle->DrawIndexedInstanced(
		IndexCount,
		InstanceCount,
		StartIndexLocation,
		BaseVertexLocation,
		StartInstanceLocation);
}

void D3D12CommandContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	CommandListHandle.FlushResourceBarriers();
	CommandListHandle->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void D3D12CommandContext::DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc)
{
	CommandListHandle.FlushResourceBarriers();
	CommandListHandle.GetGraphicsCommandList4()->DispatchRays(pDesc);
}

void D3D12CommandContext::DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
	CommandListHandle.FlushResourceBarriers();
	CommandListHandle.GetGraphicsCommandList6()->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}
