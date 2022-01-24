#pragma once
#include "D3D12Core.h"
#include "D3D12CommandList.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Profiler.h"

class D3D12CommandQueue;
class D3D12PipelineState;
class D3D12RaytracingPipelineState;
class D3D12RootSignature;
class D3D12RenderTarget;

class D3D12CommandContext : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandContext(
		D3D12LinkedDevice*		 Parent,
		RHID3D12CommandQueueType Type,
		D3D12_COMMAND_LIST_TYPE	 CommandListType);

	[[nodiscard]] D3D12CommandQueue*		  GetCommandQueue();
	[[nodiscard]] ID3D12GraphicsCommandList*  GetGraphicsCommandList() const noexcept;
	[[nodiscard]] ID3D12GraphicsCommandList4* GetGraphicsCommandList4() const noexcept;
	[[nodiscard]] ID3D12GraphicsCommandList6* GetGraphicsCommandList6() const noexcept;
	D3D12CommandListHandle&					  operator->() { return CommandListHandle; }

	void Open();
	void Close();

	// Returns D3D12SyncHandle, may be ignored if WaitForCompletion is true
	D3D12SyncHandle Execute(bool WaitForCompletion);

	void TransitionBarrier(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(
		D3D12Resource* BeforeResource,
		D3D12Resource* AfterResource);

	void UAVBarrier(
		D3D12Resource* Resource);

	void FlushResourceBarriers();

	bool AssertResourceState(
		D3D12Resource*		  Resource,
		D3D12_RESOURCE_STATES State,
		UINT				  Subresource);

	void SetPipelineState(
		D3D12PipelineState* PipelineState);

	void SetPipelineState(
		D3D12RaytracingPipelineState* RaytracingPipelineState);

	void SetGraphicsRootSignature(
		D3D12RootSignature* RootSignature);

	void SetComputeRootSignature(
		D3D12RootSignature* RootSignature);

	void ClearRenderTarget(
		D3D12RenderTarget* RenderTarget);

	void SetRenderTarget(
		D3D12RenderTarget* RenderTarget);

	void SetViewport(
		const RHIViewport& Viewport);

	void SetViewports(
		std::span<RHIViewport> Viewports);

	void SetScissorRect(
		const RHIRect& ScissorRect);

	void SetScissorRects(
		std::span<RHIRect> ScissorRects);

	void SetGraphicsConstantBuffer(
		UINT		RootParameterIndex,
		UINT64		Size,
		const void* Data);

	void SetComputeConstantBuffer(
		UINT		RootParameterIndex,
		UINT64		Size,
		const void* Data);

	template<typename T>
	void SetGraphicsConstantBuffer(
		UINT	 RootParameterIndex,
		const T& Data)
	{
		static_assert(std::is_trivial_v<T>);
		SetGraphicsConstantBuffer(RootParameterIndex, sizeof(T), &Data);
	}

	template<typename T>
	void SetComputeConstantBuffer(
		UINT	 RootParameterIndex,
		const T& Data)
	{
		static_assert(std::is_trivial_v<T>);
		SetComputeConstantBuffer(RootParameterIndex, sizeof(T), &Data);
	}

	// These version of the API calls should be used as it needs to flush resource barriers before any work
	void DrawInstanced(
		UINT VertexCount,
		UINT InstanceCount,
		UINT StartVertexLocation,
		UINT StartInstanceLocation);

	void DrawIndexedInstanced(
		UINT IndexCount,
		UINT InstanceCount,
		UINT StartIndexLocation,
		INT	 BaseVertexLocation,
		UINT StartInstanceLocation);

	void Dispatch(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY,
		UINT ThreadGroupCountZ);

	template<UINT ThreadSizeX>
	void Dispatch1D(
		UINT ThreadGroupCountX)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		Dispatch(ThreadGroupCountX, 1, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY>
	void Dispatch2D(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
		Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY, UINT ThreadSizeZ>
	void Dispatch3D(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY,
		UINT ThreadGroupCountZ)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
		ThreadGroupCountZ = RoundUpAndDivide(ThreadGroupCountZ, ThreadSizeZ);
		Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void DispatchRays(
		const D3D12_DISPATCH_RAYS_DESC* pDesc);

	void DispatchMesh(
		UINT ThreadGroupCountX,
		UINT ThreadGroupCountY,
		UINT ThreadGroupCountZ);

	void ResetCounter(
		D3D12Resource* CounterResource,
		UINT64		   CounterOffset,
		UINT		   Value = 0);

private:
	template<typename T>
	constexpr T RoundUpAndDivide(T Value, size_t Alignment)
	{
		return (T)((Value + Alignment - 1) / Alignment);
	}

	template<RHI_PIPELINE_STATE_TYPE PsoType>
	struct D3D12DescriptorTableTraits
	{
	};
	template<>
	struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Graphics>
	{
		static auto Bind() { return &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable; }
	};
	template<>
	struct D3D12DescriptorTableTraits<RHI_PIPELINE_STATE_TYPE::Compute>
	{
		static auto Bind() { return &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable; }
	};

	template<RHI_PIPELINE_STATE_TYPE PsoType>
	void SetDynamicResourceDescriptorTables(D3D12RootSignature* RootSignature);

private:
	RHID3D12CommandQueueType	Type;
	D3D12CommandListHandle		CommandListHandle;
	ARC<ID3D12CommandAllocator> CommandAllocator;
	D3D12CommandAllocatorPool	CommandAllocatorPool;
	D3D12LinearAllocator		CpuConstantAllocator;

	// TODO: Finish cache
	// State Cache
	UINT64 DirtyStates;
	struct StateCache
	{
		D3D12RootSignature* RootSignature;
		D3D12PipelineState* PipelineState;

		struct
		{
			// Viewport
			UINT		   NumViewports														   = 0;
			D3D12_VIEWPORT Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

			// Scissor Rect
			UINT	   NumScissorRects														  = 0;
			D3D12_RECT ScissorRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};

			D3D12RenderTarget* RenderTarget;
		} Graphics;

		struct
		{
		} Compute;

		struct
		{
			D3D12RaytracingPipelineState* RaytracingPipelineState;
		} Raytracing;
	} Cache = {};
};

class D3D12ScopedEventObject
{
public:
	D3D12ScopedEventObject(
		D3D12CommandContext& CommandContext,
		std::string_view	 Name)
		: ProfileBlock(CommandContext.GetGraphicsCommandList(), Name.data())
#ifdef _DEBUG
		, PixEvent(CommandContext.GetGraphicsCommandList(), 0, Name.data())
#endif
	{
	}

private:
	D3D12ProfileBlock ProfileBlock;
#ifdef _DEBUG
	PIXScopedEventObject<ID3D12GraphicsCommandList> PixEvent;
#endif
};

#define D3D12Concatenate(a, b)				  a##b
#define D3D12GetScopedEventVariableName(a, b) D3D12Concatenate(a, b)
#define D3D12ScopedEvent(context, name)		  D3D12ScopedEventObject D3D12GetScopedEventVariableName(D3D12Event, __LINE__)(context, name)
