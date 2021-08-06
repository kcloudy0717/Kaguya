#include "CommandList.h"
#include "Device.h"
#include "CommandQueue.h"

using Microsoft::WRL::ComPtr;

CommandAllocator::CommandAllocator(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE Type)
{
	ASSERTD3D12APISUCCEEDED(
		pDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(pCommandAllocator.ReleaseAndGetAddressOf())));
}

CommandListHandle::CommandListHandle(Device* Device, D3D12_COMMAND_LIST_TYPE Type, CommandQueue* CommandQueue)
{
	pCommandList = std::make_shared<CommandList>(Device, Type);
}

void CommandListHandle::TransitionBarrier(
	Resource*			  Resource,
	D3D12_RESOURCE_STATES State,
	UINT				  Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	// TODO: There might be some logic and cases here im missing, come back and edit
	// if anything goes boom

	ResourceStateTracker& Tracker		= pCommandList->ResourceStateTracker;
	CResourceState&		  ResourceState = Tracker.GetResourceState(Resource);
	// First use on the command list
	if (ResourceState.IsResourceStateUnknown())
	{
		PendingResourceBarrier PendingResourceBarrier = { .Resource	   = Resource,
														  .State	   = State,
														  .Subresource = Subresource };
		Tracker.Add(PendingResourceBarrier);
	}
	// Known state within the command list
	else
	{
		// If we are applying transition to all subresources and we are in different tracking mode
		// transition each subresource individually
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !ResourceState.IsUniformResourceState())
		{
			// First transition all of the subresources if they are different than the StateAfter
			int i = 0;
			for (const auto& SubresourceState : ResourceState)
			{
				if (SubresourceState != State)
				{
					pCommandList->ResourceBarrierBatch.AddTransition(Resource, SubresourceState, State, i);
				}

				i++;
			}
		}
		else
		{
			D3D12_RESOURCE_STATES KnownState = ResourceState.GetSubresourceState(Subresource);
			if (KnownState != State)
			{
				pCommandList->ResourceBarrierBatch.AddTransition(Resource, KnownState, State, Subresource);
			}
		}
	}

	// Update resource state, potentially change state tracking mode
	ResourceState.SetSubresourceState(Subresource, State);
}

void CommandListHandle::AliasingBarrier(Resource* BeforeResource, Resource* AfterResource)
{
	pCommandList->ResourceBarrierBatch.AddAliasing(BeforeResource, AfterResource);
}

void CommandListHandle::UAVBarrier(Resource* Resource)
{
	pCommandList->ResourceBarrierBatch.AddUAV(Resource);
}

void CommandListHandle::FlushResourceBarriers()
{
	pCommandList->FlushResourceBarriers();
}

bool CommandListHandle::AssertResourceState(Resource* pResource, D3D12_RESOURCE_STATES State, UINT Subresource)
{
#ifdef D3D12_DEBUG_RESOURCE_STATES
	// The check here is needed as GraphicsCommandList.As(&DebugCommandList); seems to fail under GPU capture
	if (pCommandList->DebugCommandList)
	{
		const BOOL Assertion =
			pCommandList->DebugCommandList->AssertResourceState(pResource->GetResource(), Subresource, State);
		if (!Assertion)
		{
			return false;
		}
	}
#endif
	return true;
}

CommandListHandle::CommandList::CommandList(Device* Parent, D3D12_COMMAND_LIST_TYPE Type)
	: DeviceChild(Parent)
	, Type(Type)
	, pCommandAllocator(nullptr)
	, IsClosed(true)
{
#ifdef NVIDIA_NSIGHT_AFTERMATH
	// GFSDK_Aftermath_DX12_CreateContextHandle fails if I used CreateCommandList1 with type thats not
	// D3D12_COMMAND_LIST_TYPE_DIRECT
	ComPtr<ID3D12CommandAllocator> Temp;
	ASSERTD3D12APISUCCEEDED(
		Device->GetDevice()->CreateCommandAllocator(Type, IID_PPV_ARGS(Temp.ReleaseAndGetAddressOf())));
	ASSERTD3D12APISUCCEEDED(Device->GetDevice()->CreateCommandList(
		1,
		Type,
		Temp.Get(),
		nullptr,
		IID_PPV_ARGS(GraphicsCommandList.ReleaseAndGetAddressOf())));
#else
	ASSERTD3D12APISUCCEEDED(Parent->GetDevice5()->CreateCommandList1(
		1,
		Type,
		D3D12_COMMAND_LIST_FLAG_NONE,
		IID_PPV_ARGS(GraphicsCommandList.ReleaseAndGetAddressOf())));
#endif

	GraphicsCommandList.As(&GraphicsCommandList4);
	GraphicsCommandList.As(&GraphicsCommandList6);

#ifdef D3D12_DEBUG_RESOURCE_STATES
	GraphicsCommandList.As(&DebugCommandList);
#endif
#ifdef NVIDIA_NSIGHT_AFTERMATH
	// Create an Nsight Aftermath context handle for setting Aftermath event markers in this command list.
	GFSDK_Aftermath_DX12_CreateContextHandle(GraphicsCommandList.Get(), &AftermathContextHandle);
	// GFSDK_Aftermath_DX12_CreateContextHandle needs the CommandList to be open prior to calling the function
	ASSERTD3D12APISUCCEEDED(GraphicsCommandList->Close());
#endif
}

CommandListHandle::CommandList::~CommandList()
{
#ifdef NVIDIA_NSIGHT_AFTERMATH
	GFSDK_Aftermath_ReleaseContextHandle(AftermathContextHandle);
#endif
}

void CommandListHandle::CommandList::Close()
{
	if (!IsClosed)
	{
		IsClosed = true;

		FlushResourceBarriers();
		ASSERTD3D12APISUCCEEDED(GraphicsCommandList->Close());
	}
}

void CommandListHandle::CommandList::Reset(CommandAllocator* CommandAllocator)
{
	pCommandAllocator = CommandAllocator;
	IsClosed		  = false;

	ASSERTD3D12APISUCCEEDED(GraphicsCommandList->Reset(*pCommandAllocator, nullptr));

	// Reset resource state tracking and resource barriers
	ResourceStateTracker.Reset();
	ResourceBarrierBatch.Reset();
}

void CommandListHandle::CommandList::FlushResourceBarriers()
{
	ResourceBarrierBatch.Flush(GraphicsCommandList.Get());
}
