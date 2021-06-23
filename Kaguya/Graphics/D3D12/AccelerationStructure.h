#pragma once

class BottomLevelAccelerationStructure
{
public:
	BottomLevelAccelerationStructure() noexcept;

	auto size() const noexcept { return m_RaytracingGeometryDescs.size(); }

	void clear() noexcept { m_RaytracingGeometryDescs.clear(); }

	void AddGeometry(_In_ const D3D12_RAYTRACING_GEOMETRY_DESC& Desc);

	void ComputeMemoryRequirements(
		_In_ ID3D12Device5* pDevice,
		_Out_ UINT64* pScratchSizeInBytes,
		_Out_ UINT64* pResultSizeInBytes);

	void Generate(
		_In_ ID3D12GraphicsCommandList6* pCommandList,
		_In_ ID3D12Resource* pScratch,
		_In_ ID3D12Resource* pResult);

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_RaytracingGeometryDescs;
	UINT64										m_ScratchSizeInBytes;
	UINT64										m_ResultSizeInBytes;
};

// https://developer.nvidia.com/blog/rtx-best-practices/
// We should rebuild the TLAS rather than update, It’s just easier to manage in most circumstances, and the cost savings
// to refit likely aren’t worth sacrificing quality of TLAS.
class TopLevelAccelerationStructure
{
public:
	TopLevelAccelerationStructure();

	auto size() const noexcept { return m_RaytracingInstanceDescs.size(); }

	bool empty() const noexcept { return m_RaytracingInstanceDescs.empty(); }

	void clear() noexcept { m_RaytracingInstanceDescs.clear(); }

	auto begin() noexcept { return m_RaytracingInstanceDescs.begin(); }

	auto end() noexcept { return m_RaytracingInstanceDescs.end(); }

	void AddInstance(_In_ const D3D12_RAYTRACING_INSTANCE_DESC& Desc);

	void ComputeMemoryRequirements(
		_In_ ID3D12Device5* pDevice,
		_Out_ UINT64* pScratchSizeInBytes,
		_Out_ UINT64* pResultSizeInBytes);

	void Generate(
		_In_ ID3D12GraphicsCommandList6* pCommandList,
		_In_ ID3D12Resource* pScratch,
		_In_ ID3D12Resource*		   pResult,
		_In_ D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs);

private:
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_RaytracingInstanceDescs;
	UINT64										m_ScratchSizeInBytes;
	UINT64										m_ResultSizeInBytes;
};
