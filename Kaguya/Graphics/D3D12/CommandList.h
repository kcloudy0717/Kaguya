#pragma once
#include <d3d12.h>
#include "d3dx12.h"

#include <Core/Math.h>
#include "ResourceStateTracker.h"
#include "ResourceViewHeaps.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "RaytracingPipelineState.h"

class CommandList
{
public:
	CommandList() noexcept = default;
	explicit CommandList(
		_In_ ID3D12Device4* pDevice,
		_In_ D3D12_COMMAND_LIST_TYPE Type);

	CommandList(CommandList&&) noexcept = default;
	CommandList& operator=(CommandList&&) noexcept = default;

	CommandList(const CommandList&) = delete;
	CommandList& operator=(const CommandList&) = delete;

	operator ID3D12GraphicsCommandList6* () const { return m_pCommandList.Get(); }
	ID3D12GraphicsCommandList6* operator->() const { return m_pCommandList.Get(); }

	auto GetCommandList() { return m_pCommandList.Get(); }
	auto GetPendingCommandList() { return m_pPendingCommandList.Get(); }

	bool Close(
		_In_ ResourceStateTracker* pGlobalResourceStateTracker);

	void Reset(
		_In_ UINT64 FenceValue,
		_In_ UINT64 CompletedValue,
		_In_ CommandQueue* pCommandQueue);

	void SetDescriptorHeaps(
		_In_ const ResourceViewHeaps& ResourceViewHeaps);

	void TransitionBarrier(
		_In_ ID3D12Resource* pResource,
		_In_ D3D12_RESOURCE_STATES TransitionState,
		_In_ UINT Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void AliasingBarrier(
		_In_ ID3D12Resource* pBeforeResource,
		_In_ ID3D12Resource* pAfterResource);

	void UAVBarrier(
		_In_ ID3D12Resource* pResource);

	void FlushResourceBarriers();

	// These version of the API calls should be used as it needs to flush resource barriers before any work
	void DrawInstanced(
		_In_ UINT VertexCount,
		_In_ UINT InstanceCount,
		_In_ UINT StartVertexLocation,
		_In_ UINT StartInstanceLocation);

	void DrawIndexedInstanced(
		_In_ UINT IndexCount,
		_In_ UINT InstanceCount,
		_In_ UINT StartIndexLocation,
		_In_ UINT BaseVertexLocation,
		_In_ UINT StartInstanceLocation);

	void Dispatch(
		_In_ UINT ThreadGroupCountX,
		_In_ UINT ThreadGroupCountY,
		_In_ UINT ThreadGroupCountZ);

	template<UINT ThreadSizeX>
	void Dispatch1D(
		_In_ UINT ThreadGroupCountX)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);

		Dispatch(ThreadGroupCountX, 1, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY>
	void Dispatch2D(
		_In_ UINT ThreadGroupCountX,
		_In_ UINT ThreadGroupCountY)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);

		Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
	}

	template<UINT ThreadSizeX, UINT ThreadSizeY, UINT ThreadSizeZ>
	void Dispatch3D(
		_In_ UINT ThreadGroupCountX,
		_In_ UINT ThreadGroupCountY,
		_In_ UINT ThreadGroupCountZ)
	{
		ThreadGroupCountX = RoundUpAndDivide(ThreadGroupCountX, ThreadSizeX);
		ThreadGroupCountY = RoundUpAndDivide(ThreadGroupCountY, ThreadSizeY);
		ThreadGroupCountZ = RoundUpAndDivide(ThreadGroupCountZ, ThreadSizeZ);

		Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void DispatchRays(
		_In_ const D3D12_DISPATCH_RAYS_DESC* pDesc);

	void DispatchMesh(
		_In_  UINT ThreadGroupCountX,
		_In_  UINT ThreadGroupCountY,
		_In_  UINT ThreadGroupCountZ);

private:
	ID3D12CommandAllocator* m_pAllocator = nullptr;
	ID3D12CommandAllocator* m_pPendingAllocator = nullptr;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_pCommandList;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_pPendingCommandList;

	ResourceStateTracker m_ResourceStateTracker;
};
