#pragma once

class BottomLevelAccelerationStructure
{
public:
	auto size() const noexcept { return m_RaytracingGeometryDescs.size(); }

	void clear() noexcept { m_RaytracingGeometryDescs.clear(); }

	void AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc);

	void ComputeMemoryRequirements(ID3D12Device5* pDevice, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);

	void Generate(ID3D12GraphicsCommandList6* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult);

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_RaytracingGeometryDescs;
	UINT64										m_ScratchSizeInBytes = 0;
	UINT64										m_ResultSizeInBytes	 = 0;
};

// https://developer.nvidia.com/blog/rtx-best-practices/
// We should rebuild the TLAS rather than update, It’s just easier to manage in most circumstances, and the cost savings
// to refit likely aren’t worth sacrificing quality of TLAS.
class TopLevelAccelerationStructure
{
public:
	auto size() const noexcept { return m_RaytracingInstanceDescs.size(); }

	bool empty() const noexcept { return m_RaytracingInstanceDescs.empty(); }

	void clear() noexcept { m_RaytracingInstanceDescs.clear(); }

	auto begin() noexcept { return m_RaytracingInstanceDescs.begin(); }

	auto end() noexcept { return m_RaytracingInstanceDescs.end(); }

	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc);

	void ComputeMemoryRequirements(ID3D12Device5* pDevice, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);

	void Generate(
		ID3D12GraphicsCommandList6* pCommandList,
		ID3D12Resource*				pScratch,
		ID3D12Resource*				pResult,
		D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs);

private:
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_RaytracingInstanceDescs;
	UINT64										m_ScratchSizeInBytes = 0;
	UINT64										m_ResultSizeInBytes	 = 0;
};
