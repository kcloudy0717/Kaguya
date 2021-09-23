#pragma once
#include <Core/CoreDefines.h>
#include "D3D12Common.h"
#include "D3D12CommandQueue.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12PipelineState.h"
#include "D3D12RaytracingPipelineState.h"
#include "D3D12RenderPass.h"
#include "D3D12RenderTarget.h"

class D3D12CommandContext : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandContext(
		D3D12LinkedDevice*		Parent,
		ED3D12CommandQueueType	Type,
		D3D12_COMMAND_LIST_TYPE CommandListType);

	D3D12CommandQueue* GetCommandQueue();

	D3D12CommandListHandle& operator->() { return CommandListHandle; }

	void OpenCommandList();
	void CloseCommandList();

	// Returns CommandSyncPoint, may be ignored if WaitForCompletion is true
	D3D12CommandSyncPoint Execute(bool WaitForCompletion);

	void TransitionBarrier(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void AliasingBarrier(D3D12Resource* BeforeResource, D3D12Resource* AfterResource);
	void UAVBarrier(D3D12Resource* Resource);
	void FlushResourceBarriers();

	void SetViewport(const RHIViewport& Viewport);
	void SetViewports(UINT NumViewports, RHIViewport* Viewports);
	void SetScissorRect(const RHIRect& ScissorRect);
	void SetScissorRects(UINT NumScissorRects, RHIRect* ScissorRects);

	void SetPipelineState(D3D12PipelineState* PipelineState);
	void SetPipelineState(D3D12RaytracingPipelineState* RaytracingPipelineState);

	void SetGraphicsRootSignature(D3D12RootSignature* RootSignature);
	void SetComputeRootSignature(D3D12RootSignature* RootSignature);

	void SetGraphicsConstantBuffer(UINT RootParameterIndex, UINT64 Size, void* Data)
	{
		D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
		std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
		CommandListHandle->SetGraphicsRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
	}

	void SetComputeConstantBuffer(UINT RootParameterIndex, UINT64 Size, void* Data)
	{
		D3D12Allocation Allocation = CpuConstantAllocator.Allocate(Size);
		std::memcpy(Allocation.CpuVirtualAddress, Data, Size);
		CommandListHandle->SetComputeRootConstantBufferView(RootParameterIndex, Allocation.GpuVirtualAddress);
	}

	void BeginRenderPass(D3D12RenderPass* RenderPass, D3D12RenderTarget* RenderTarget);

	void EndRenderPass();

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

	void ResetCounter(D3D12Resource* CounterResource, UINT64 CounterOffset, UINT Value = 0)
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

	const ED3D12CommandQueueType Type;

	D3D12CommandListHandle	  CommandListHandle;
	D3D12CommandAllocator*	  CommandAllocator;
	D3D12CommandAllocatorPool CommandAllocatorPool;

	D3D12LinearAllocator CpuConstantAllocator;

	// State Cache
	struct StateCache
	{
		D3D12PipelineState*			  PipelineState			  = nullptr;
		D3D12RaytracingPipelineState* RaytracingPipelineState = nullptr;

		D3D12RootSignature* RootSignature = nullptr;

		struct
		{
			// Viewport
			UINT		   NumViewports														   = 0;
			D3D12_VIEWPORT Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

			// Scissor Rect
			UINT	   NumScissorRects														  = 0;
			D3D12_RECT ScissorRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

			D3D12RenderPass*   RenderPass	= nullptr;
			D3D12RenderTarget* RenderTarget = nullptr;
		} Graphics;

		struct
		{
		} Compute;

		struct
		{
		} Raytracing;
	} Cache;
};

class D3D12ScopedEventObject
{
public:
	D3D12ScopedEventObject(D3D12CommandContext& CommandContext, std::string_view Name)
		: CommandContext(CommandContext)
		, PixEvent(CommandContext.CommandListHandle.GetGraphicsCommandList(), 0, Name.data())
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
	D3D12CommandContext&							CommandContext;
	PIXScopedEventObject<ID3D12GraphicsCommandList> PixEvent;
	D3D12ProfileBlock								ProfileBlock;
};

#define D3D12Concatenate(a, b)				  a##b
#define D3D12GetScopedEventVariableName(a, b) D3D12Concatenate(a, b)
#define D3D12ScopedEvent(context, name)                                                                                \
	D3D12ScopedEventObject D3D12GetScopedEventVariableName(d3d12Event, __LINE__)(context, name)
