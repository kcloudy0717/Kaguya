#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12LinkedDevice.h"
#include "D3D12CommandQueue.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12Descriptor.h"
#include "D3D12Resource.h"

namespace RHI
{
	D3D12CommandContext::D3D12CommandContext(
		D3D12LinkedDevice*		 Parent,
		RHID3D12CommandQueueType Type,
		D3D12_COMMAND_LIST_TYPE	 CommandListType)
		: D3D12LinkedDeviceChild(Parent)
		, Type(Type)
		, CommandListHandle(Parent, CommandListType)
		, CommandAllocator(nullptr)
		, CommandAllocatorPool(Parent, CommandListType)
		, CpuConstantAllocator(Parent)
	{
	}

	D3D12CommandQueue* D3D12CommandContext::GetCommandQueue()
	{
		return GetParentLinkedDevice()->GetCommandQueue(Type);
	}

	ID3D12GraphicsCommandList* D3D12CommandContext::GetGraphicsCommandList() const noexcept
	{
		return CommandListHandle.GetGraphicsCommandList();
	}

	ID3D12GraphicsCommandList4* D3D12CommandContext::GetGraphicsCommandList4() const noexcept
	{
		return CommandListHandle.GetGraphicsCommandList4();
	}

	ID3D12GraphicsCommandList6* D3D12CommandContext::GetGraphicsCommandList6() const noexcept
	{
		return CommandListHandle.GetGraphicsCommandList6();
	}

	void D3D12CommandContext::Open()
	{
		if (!CommandAllocator)
		{
			CommandAllocator = CommandAllocatorPool.RequestCommandAllocator();
		}
		CommandListHandle.Open(CommandAllocator.Get());

		if (Type == RHID3D12CommandQueueType::Direct || Type == RHID3D12CommandQueueType::AsyncCompute)
		{
			ID3D12DescriptorHeap* DescriptorHeaps[2] = {
				GetParentLinkedDevice()->GetResourceDescriptorHeap(),
				GetParentLinkedDevice()->GetSamplerDescriptorHeap(),
			};

			CommandListHandle->SetDescriptorHeaps(2, DescriptorHeaps);
		}

		// Reset cache
		Cache = {};
	}

	void D3D12CommandContext::Close()
	{
		CommandListHandle.Close();
	}

	D3D12SyncHandle D3D12CommandContext::Execute(bool WaitForCompletion)
	{
		D3D12SyncHandle SyncHandle = GetCommandQueue()->ExecuteCommandLists(1, &CommandListHandle, WaitForCompletion);

		// Release the command allocator so it can be reused.
		CommandAllocatorPool.DiscardCommandAllocator(std::exchange(CommandAllocator, nullptr), SyncHandle);

		CpuConstantAllocator.Version(SyncHandle);
		return SyncHandle;
	}

	void D3D12CommandContext::TransitionBarrier(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
	{
		CommandListHandle.TransitionBarrier(Resource, State, Subresource);
	}

	void D3D12CommandContext::AliasingBarrier(
		D3D12Resource* BeforeResource,
		D3D12Resource* AfterResource)
	{
		CommandListHandle.AliasingBarrier(BeforeResource, AfterResource);
	}

	void D3D12CommandContext::UAVBarrier(
		D3D12Resource* Resource)
	{
		CommandListHandle.UAVBarrier(Resource);
	}

	void D3D12CommandContext::FlushResourceBarriers()
	{
		CommandListHandle.FlushResourceBarriers();
	}

	bool D3D12CommandContext::AssertResourceState(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource)
	{
		return CommandListHandle.AssertResourceState(Resource, State, Subresource);
	}

	void D3D12CommandContext::SetPipelineState(
		D3D12PipelineState* PipelineState)
	{
		if (Cache.PipelineState != PipelineState)
		{
			Cache.PipelineState = PipelineState;

			CommandListHandle->SetPipelineState(Cache.PipelineState->GetApiHandle());
		}
	}

	void D3D12CommandContext::SetPipelineState(
		D3D12RaytracingPipelineState* RaytracingPipelineState)
	{
		if (Cache.Raytracing.RaytracingPipelineState != RaytracingPipelineState)
		{
			Cache.Raytracing.RaytracingPipelineState = RaytracingPipelineState;

			CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RaytracingPipelineState->GetApiHandle());
		}
	}

	void D3D12CommandContext::SetGraphicsRootSignature(
		D3D12RootSignature* RootSignature)
	{
		if (Cache.RootSignature != RootSignature)
		{
			Cache.RootSignature = RootSignature;

			CommandListHandle->SetGraphicsRootSignature(RootSignature->GetApiHandle());

			SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Graphics>(RootSignature);
		}
	}

	void D3D12CommandContext::SetComputeRootSignature(
		D3D12RootSignature* RootSignature)
	{
		if (Cache.RootSignature != RootSignature)
		{
			Cache.RootSignature = RootSignature;

			CommandListHandle->SetComputeRootSignature(RootSignature->GetApiHandle());

			SetDynamicResourceDescriptorTables<RHI_PIPELINE_STATE_TYPE::Compute>(RootSignature);
		}
	}

	void D3D12CommandContext::ClearRenderTarget(
		u32					   NumRenderTargetViews,
		D3D12RenderTargetView* RenderTargetViews[],
		D3D12DepthStencilView* DepthStencilView)
	{
		for (u32 i = 0; i < NumRenderTargetViews; ++i)
		{
			D3D12_CLEAR_VALUE ClearValue = RenderTargetViews[i]->GetResource()->GetClearValue();
			CommandListHandle->ClearRenderTargetView(RenderTargetViews[i]->GetCpuHandle(), ClearValue.Color, 0, nullptr);
		}
		if (DepthStencilView)
		{
			D3D12_CLEAR_VALUE ClearValue = DepthStencilView->GetResource()->GetClearValue();
			FLOAT			  Depth		 = ClearValue.DepthStencil.Depth;
			UINT8			  Stencil	 = ClearValue.DepthStencil.Stencil;

			CommandListHandle->ClearDepthStencilView(DepthStencilView->GetCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Depth, Stencil, 0, nullptr);
		}
	}

	void D3D12CommandContext::SetRenderTarget(
		u32					   NumRenderTargetViews,
		D3D12RenderTargetView* RenderTargetViews[],
		D3D12DepthStencilView* DepthStencilView)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE pRenderTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilDescriptor											 = {};

		for (u32 i = 0; i < NumRenderTargetViews; ++i)
		{
			pRenderTargetDescriptors[i] = RenderTargetViews[i]->GetCpuHandle();
		}
		if (DepthStencilView)
		{
			DepthStencilDescriptor = DepthStencilView->GetCpuHandle();
		}

		CommandListHandle->OMSetRenderTargets(NumRenderTargetViews, pRenderTargetDescriptors, FALSE, DepthStencilView ? &DepthStencilDescriptor : nullptr);
	}

	void D3D12CommandContext::SetViewport(
		const RHIViewport& Viewport)
	{
		Cache.Graphics.NumViewports = 1;
		Cache.Graphics.Viewports[0] = CD3DX12_VIEWPORT(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.Width, Viewport.Height, Viewport.MinDepth, Viewport.MaxDepth);
		CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
	}

	void D3D12CommandContext::SetViewports(
		std::span<RHIViewport> Viewports)
	{
		Cache.Graphics.NumViewports = static_cast<UINT>(Viewports.size());
		UINT ViewportIndex			= 0;
		for (const auto& Viewport : Viewports)
		{
			Cache.Graphics.Viewports[ViewportIndex++] = CD3DX12_VIEWPORT(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.Width, Viewport.Height, Viewport.MinDepth, Viewport.MaxDepth);
		}
		CommandListHandle->RSSetViewports(Cache.Graphics.NumViewports, Cache.Graphics.Viewports);
	}

	void D3D12CommandContext::SetScissorRect(
		const RHIRect& ScissorRect)
	{
		Cache.Graphics.NumScissorRects = 1;
		Cache.Graphics.ScissorRects[0] = CD3DX12_RECT(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
		CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
	}

	void D3D12CommandContext::SetScissorRects(
		std::span<RHIRect> ScissorRects)
	{
		Cache.Graphics.NumScissorRects = static_cast<UINT>(ScissorRects.size());
		UINT ScissorRectIndex		   = 0;
		for (const auto& ScissorRect : ScissorRects)
		{
			Cache.Graphics.ScissorRects[ScissorRectIndex++] = CD3DX12_RECT(ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
		}
		CommandListHandle->RSSetScissorRects(Cache.Graphics.NumScissorRects, Cache.Graphics.ScissorRects);
	}

	void D3D12CommandContext::SetGraphicsConstantBuffer(
		UINT		RootParameterIndex,
		UINT64		Size,
		const void* Data)
	{
		D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
		std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
		CommandListHandle->SetGraphicsRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
	}

	void D3D12CommandContext::SetComputeConstantBuffer(
		UINT		RootParameterIndex,
		UINT64		Size,
		const void* Data)
	{
		D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
		std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
		CommandListHandle->SetComputeRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
	}

	void D3D12CommandContext::DrawInstanced(
		UINT VertexCount,
		UINT InstanceCount,
		UINT StartVertexLocation,
		UINT StartInstanceLocation)
	{
		CommandListHandle.FlushResourceBarriers();
		CommandListHandle->DrawInstanced(
			VertexCount,
			InstanceCount,
			StartVertexLocation,
			StartInstanceLocation);
	}

	void D3D12CommandContext::DrawIndexedInstanced(
		UINT IndexCount,
		UINT InstanceCount,
		UINT StartIndexLocation,
		INT	 BaseVertexLocation,
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

	void D3D12CommandContext::Dispatch(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY,
		UINT ThreadGroupCountZ)
	{
		CommandListHandle.FlushResourceBarriers();
		CommandListHandle->Dispatch(
			ThreadGroupCountX,
			ThreadGroupCountY,
			ThreadGroupCountZ);
	}

	void D3D12CommandContext::DispatchRays(
		const D3D12_DISPATCH_RAYS_DESC* pDesc)
	{
		CommandListHandle.FlushResourceBarriers();
		CommandListHandle.GetGraphicsCommandList4()->DispatchRays(
			pDesc);
	}

	void D3D12CommandContext::DispatchMesh(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY,
		UINT ThreadGroupCountZ)
	{
		CommandListHandle.FlushResourceBarriers();
		CommandListHandle.GetGraphicsCommandList6()->DispatchMesh(
			ThreadGroupCountX,
			ThreadGroupCountY,
			ThreadGroupCountZ);
	}

	void D3D12CommandContext::ResetCounter(
		D3D12Resource* CounterResource,
		UINT64		   CounterOffset,
		UINT		   Value /*= 0*/)
	{
		D3D12Allocation Allocation = CpuConstantAllocator.Allocate(sizeof(UINT));
		std::memcpy(Allocation.CpuVirtualAddress, &Value, sizeof(UINT));

		CommandListHandle->CopyBufferRegion(
			CounterResource->GetResource(),
			CounterOffset,
			Allocation.Resource,
			Allocation.Offset,
			sizeof(UINT));
	}

	template<RHI_PIPELINE_STATE_TYPE PsoType>
	void D3D12CommandContext::SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature)
	{
		ID3D12GraphicsCommandList* CommandList = CommandListHandle.GetGraphicsCommandList();

		// Bindless descriptors
		UINT NumParameters		= RootSignature->GetNumParameters();
		UINT Offset				= NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		auto ResourceDescriptor = GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
		auto SamplerDescriptor	= GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

		(CommandList->*D3D12DynamicResourceTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset, ResourceDescriptor);
		(CommandList->*D3D12DynamicResourceTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset, ResourceDescriptor);
		(CommandList->*D3D12DynamicResourceTableTraits<PsoType>::Bind())(RootParameters::DescriptorTable::SamplerDescriptorTable + Offset, SamplerDescriptor);
	}

	D3D12ScopedEventObject::D3D12ScopedEventObject(D3D12CommandContext& CommandContext, std::string_view Name)
		: ProfileBlock(CommandContext.GetParentLinkedDevice()->GetProfiler(), CommandContext.GetCommandQueue(), CommandContext.GetGraphicsCommandList(), Name.data())
#ifdef _DEBUG
		, PixEvent(CommandContext.GetGraphicsCommandList(), 0, Name.data())
#endif
	{
	}
} // namespace RHI
