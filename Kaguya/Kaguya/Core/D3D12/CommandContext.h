#pragma once
#include <Core/CoreDefines.h>
#include "D3D12Common.h"
#include "CommandQueue.h"
#include "MemoryAllocator.h"

class CommandContext : public DeviceChild
{
public:
	CommandContext(Device* Device, ECommandQueueType Type, D3D12_COMMAND_LIST_TYPE CommandListType);

	CommandQueue* GetCommandQueue();

	CommandListHandle& operator->() { return CommandListHandle; }

	void OpenCommandList()
	{
		if (!CommandAllocator)
		{
			CommandAllocator = CommandAllocatorPool.RequestCommandAllocator();
		}

		CommandListHandle = GetCommandQueue()->RequestCommandList(CommandAllocator);
	}

	void CloseCommandList() { CommandListHandle.Close(); }

	// Returns CommandSyncPoint, may be ignored if WaitForCompletion is true
	CommandSyncPoint Execute(bool WaitForCompletion);

	void TransitionBarrier(
		Resource*			  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(Resource* BeforeResource, Resource* AfterResource);

	void UAVBarrier(Resource* Resource);

	void FlushResourceBarriers();

	void BindResourceViewHeaps();

	// These version of the API calls should be used as it needs to flush resource barriers before any work
	void DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);

	void DrawIndexedInstanced(
		UINT IndexCount,
		UINT InstanceCount,
		UINT StartIndexLocation,
		UINT BaseVertexLocation,
		UINT StartInstanceLocation);

	void Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

	template<UINT ThreadSizeX>
	void Dispatch1D(UINT ThreadGroupCountX)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);

		Dispatch(ThreadGroupCountX, 1, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY>
	void Dispatch2D(UINT ThreadGroupCountX, UINT ThreadGroupCountY)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);

		Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY, UINT ThreadSizeZ>
	void Dispatch3D(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
		ThreadGroupCountZ = RoundUpAndDivide(ThreadGroupCountZ, ThreadSizeZ);

		Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* pDesc);

	void DispatchMesh(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);

	void ResetCounter(Resource* CounterResource, UINT64 CounterOffset, UINT Value = 0)
	{
		Allocation Allocation = CpuConstantAllocator.Allocate(sizeof(UINT));
		std::memcpy(Allocation.CPUVirtualAddress, &Value, sizeof(UINT));

		CommandListHandle->CopyBufferRegion(
			CounterResource->GetResource(),
			CounterOffset,
			Allocation.pResource,
			Allocation.Offset,
			sizeof(UINT));
	}

	const ECommandQueueType Type;

	CommandListHandle	 CommandListHandle;
	CommandAllocator*	 CommandAllocator;
	CommandAllocatorPool CommandAllocatorPool;

	LinearAllocator CpuConstantAllocator;
};

class D3D12ScopedEventObject
{
public:
	D3D12ScopedEventObject(CommandContext& CommandContext, std::string_view Name)
		: CommandContext(CommandContext)
		, PIXEvent(CommandContext.CommandListHandle.GetGraphicsCommandList(), 0, Name.data())
		, ProfileBlock(CommandContext.CommandListHandle.GetGraphicsCommandList(), Name.data())
	{
#ifdef NVIDIA_NSIGHT_AFTERMATH
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(
			CommandContext.CommandListHandle.GetAftermathContextHandle(),
			Name.data(),
			static_cast<uint32_t>(Name.size())));
#endif
	}

private:
	CommandContext&									CommandContext;
	PIXScopedEventObject<ID3D12GraphicsCommandList> PIXEvent;
	ProfileBlock									ProfileBlock;
};

#define D3D12Concatenate(a, b)				  a##b
#define D3D12GetScopedEventVariableName(a, b) D3D12Concatenate(a, b)
#define D3D12ScopedEvent(context, name)                                                                                \
	D3D12ScopedEventObject D3D12GetScopedEventVariableName(d3d12Event, __LINE__)(context, name)
