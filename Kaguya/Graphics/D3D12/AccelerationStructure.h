#pragma once

class BottomLevelAccelerationStructure
{
public:
	BottomLevelAccelerationStructure();

	auto size() const
	{
		return m_RaytracingGeometryDescs.size();
	}

	void clear()
	{
		m_RaytracingGeometryDescs.clear();
	}

	void AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc);
	void ComputeMemoryRequirements(ID3D12Device5* pDevice, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);
	void Generate(ID3D12GraphicsCommandList6* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult);
private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>	m_RaytracingGeometryDescs;
	UINT64 m_ScratchSizeInBytes;
	UINT64 m_ResultSizeInBytes;
};

// https://developer.nvidia.com/blog/rtx-best-practices/
// We should rebuild the TLAS rather than update, It’s just easier to manage in most circumstances, and the cost savings to refit likely aren’t worth sacrificing quality of TLAS.
class TopLevelAccelerationStructure
{
public:
	TopLevelAccelerationStructure();

	auto size() const
	{
		return m_RaytracingInstanceDescs.size();
	}

	bool empty() const
	{
		return m_RaytracingInstanceDescs.empty();
	}

	void clear()
	{
		m_RaytracingInstanceDescs.clear();
	}

	auto& operator[](size_t i)
	{
		return m_RaytracingInstanceDescs[i];
	}

	auto& operator[](size_t i) const
	{
		return m_RaytracingInstanceDescs[i];
	}

	auto begin()
	{
		return m_RaytracingInstanceDescs.begin();
	}

	auto end()
	{
		return m_RaytracingInstanceDescs.end();
	}

	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc);
	void ComputeMemoryRequirements(ID3D12Device5* pDevice, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);
	void Generate(ID3D12GraphicsCommandList6* pCommandList, ID3D12Resource* pScratch, ID3D12Resource* pResult, D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs);
private:
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_RaytracingInstanceDescs;
	UINT64 m_ScratchSizeInBytes;
	UINT64 m_ResultSizeInBytes;
};