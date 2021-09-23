#include "D3D12CommandList.h"
#include "D3D12LinkedDevice.h"

using Microsoft::WRL::ComPtr;

D3D12CommandAllocator::D3D12CommandAllocator(ID3D12Device* Device, D3D12_COMMAND_LIST_TYPE Type)
{
	VERIFY_D3D12_API(Device->CreateCommandAllocator(Type, IID_PPV_ARGS(CommandAllocator.ReleaseAndGetAddressOf())));
}

D3D12CommandListHandle::D3D12CommandListHandle(D3D12LinkedDevice* Device, D3D12_COMMAND_LIST_TYPE Type)
{
	CommandList = std::make_shared<D3D12CommandList>(Device, Type);
}

std::vector<D3D12_RESOURCE_BARRIER> D3D12CommandListHandle::ResolveResourceBarriers()
{
	const auto& PendingResourceBarriers = CommandList->ResourceStateTracker.GetPendingResourceBarriers();

	std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers;
	ResourceBarriers.reserve(PendingResourceBarriers.size());

	D3D12_RESOURCE_BARRIER Desc = {};
	Desc.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	for (const auto& [Resource, State, Subresource] : PendingResourceBarriers)
	{
		CResourceState& ResourceState = Resource->GetResourceState();

		D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(Subresource);
		D3D12_RESOURCE_STATES StateAfter  = State != D3D12_RESOURCE_STATE_UNKNOWN ? State : StateBefore;

		if (StateBefore != StateAfter)
		{
			Desc.Transition.pResource	= Resource->GetResource();
			Desc.Transition.Subresource = Subresource;
			Desc.Transition.StateBefore = StateBefore;
			Desc.Transition.StateAfter	= StateAfter;

			ResourceBarriers.push_back(Desc);
		}

		D3D12_RESOURCE_STATES StateCommandList = this->GetResourceState(Resource).GetSubresourceState(Subresource);
		D3D12_RESOURCE_STATES StatePrevious =
			StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN ? StateCommandList : StateAfter;

		if (StateBefore != StatePrevious)
		{
			ResourceState.SetSubresourceState(Subresource, StatePrevious);
		}
	}

	return ResourceBarriers;
}

void D3D12CommandListHandle::TransitionBarrier(
	D3D12Resource*		  Resource,
	D3D12_RESOURCE_STATES State,
	UINT				  Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
	// TODO: There might be some logic and cases here im missing, come back and edit if anything goes boom

	D3D12ResourceStateTracker& ResourceStateTracker = CommandList->ResourceStateTracker;
	CResourceState&			   ResourceState		= ResourceStateTracker.GetResourceState(Resource);
	// First use on the command list
	if (ResourceState.IsUnknown())
	{
		PendingResourceBarrier PendingResourceBarrier;
		PendingResourceBarrier.Resource	   = Resource;
		PendingResourceBarrier.State	   = State;
		PendingResourceBarrier.Subresource = Subresource;
		ResourceStateTracker.Add(PendingResourceBarrier);
	}
	// Known state within the command list
	else
	{
		// If we are applying transition to all subresources and we are in different tracking mode
		// transition each subresource individually
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !ResourceState.IsUniform())
		{
			// First transition all of the subresources if they are different than the State
			UINT i = 0;
			for (D3D12_RESOURCE_STATES SubresourceState : ResourceState)
			{
				if (SubresourceState != State)
				{
					CommandList->ResourceBarrierBatch.AddTransition(Resource, SubresourceState, State, i);
				}

				i++;
			}
		}
		else
		{
			D3D12_RESOURCE_STATES StateKnown = ResourceState.GetSubresourceState(Subresource);
			if (StateKnown != State)
			{
				CommandList->ResourceBarrierBatch.AddTransition(Resource, StateKnown, State, Subresource);
			}
		}
	}

	// Update resource state, potentially change state tracking mode
	ResourceState.SetSubresourceState(Subresource, State);
}

void D3D12CommandListHandle::AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource)
{
	CommandList->ResourceBarrierBatch.AddAliasing(BeforeResource, AfterResource);
}

void D3D12CommandListHandle::UAVBarrier(D3D12Resource* Resource)
{
	CommandList->ResourceBarrierBatch.AddUAV(Resource);
}

void D3D12CommandListHandle::FlushResourceBarriers()
{
	CommandList->FlushResourceBarriers();
}

bool D3D12CommandListHandle::AssertResourceState(D3D12Resource* Resource, D3D12_RESOURCE_STATES State, UINT Subresource)
{
#ifdef D3D12_DEBUG_RESOURCE_STATES
	if (CommandList->DebugCommandList)
	{
		if (!CommandList->DebugCommandList->AssertResourceState(Resource->GetResource(), Subresource, State))
		{
			return false;
		}
	}
#endif
	return true;
}

D3D12CommandListHandle::D3D12CommandList::D3D12CommandList(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE Type)
	: D3D12LinkedDeviceChild(Parent)
	, Type(Type)
	, CommandAllocator(nullptr)
{
#ifdef NVIDIA_NSIGHT_AFTERMATH
	ComPtr<ID3D12CommandAllocator> temp;
	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommandAllocator(Type, IID_PPV_ARGS(temp.ReleaseAndGetAddressOf())));
	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommandList(
		1,
		Type,
		temp.Get(),
		nullptr,
		IID_PPV_ARGS(GraphicsCommandList.ReleaseAndGetAddressOf())));
#else
	VERIFY_D3D12_API(Parent->GetDevice5()->CreateCommandList1(
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
	VERIFY_D3D12_API(GraphicsCommandList->Close());
#endif
}

D3D12CommandListHandle::D3D12CommandList::~D3D12CommandList()
{
#ifdef NVIDIA_NSIGHT_AFTERMATH
	GFSDK_Aftermath_ReleaseContextHandle(AftermathContextHandle);
#endif
}

void D3D12CommandListHandle::D3D12CommandList::Close()
{
	FlushResourceBarriers();
	VERIFY_D3D12_API(GraphicsCommandList->Close());
}

void D3D12CommandListHandle::D3D12CommandList::Reset(D3D12CommandAllocator* CommandAllocator)
{
	this->CommandAllocator = CommandAllocator;

	VERIFY_D3D12_API(GraphicsCommandList->Reset(*CommandAllocator, nullptr));

	// Reset resource state tracking and resource barriers
	ResourceStateTracker.Reset();
	ResourceBarrierBatch.Reset();
}

void D3D12CommandListHandle::D3D12CommandList::FlushResourceBarriers()
{
	ResourceBarrierBatch.Flush(GraphicsCommandList.Get());
}
