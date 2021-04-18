#pragma once
#include <d3d12.h>
#include "d3dx12.h"

#include <Core/Math.h>
#include "ResourceStateTracker.h"
#include "DescriptorHeap.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "RaytracingPipelineState.h"

struct CommandList
{
	CommandList(ID3D12Device4* pDevice, D3D12_COMMAND_LIST_TYPE Type);

	operator auto()
	{
		return pCommandList.Get();
	}

	auto operator->()
	{
		return pCommandList.Get();
	}

	auto GetApiHandle()
	{
		return pCommandList.Get();
	}

	bool Close(ResourceStateTracker* pGlobalResourceStateTracker);
	void Reset(UINT64 FenceValue, UINT64 CompletedValue, CommandQueue* pCommandQueue);

	void SetDescriptorHeaps(DescriptorHeap* pCBSRUADescriptorHeap, DescriptorHeap* pSamplerDescriptorHeap);

	void TransitionBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES TransitionState, UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void AliasingBarrier(ID3D12Resource* pBeforeResource, ID3D12Resource* pAfterResource);
	void UAVBarrier(ID3D12Resource* pResource);
	void FlushResourceBarriers();

	// These version of the API calls should be used as it needs to flush resource barriers before any work
	void DrawInstanced(UINT VertexCount, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
	void DrawIndexedInstanced(UINT IndexCount, UINT InstanceCount, UINT StartIndexLocation, UINT BaseVertexLocation, UINT StartInstanceLocation);

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

	ID3D12CommandAllocator* pAllocator = nullptr;
	ID3D12CommandAllocator* pPendingAllocator = nullptr;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> pCommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> pPendingCommandList;

	ResourceStateTracker ResourceStateTracker;
};