#pragma once
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include <D3D12MemAlloc.h>
#include <GraphicsMemory.h>

class Device
{
public:
	static void ReportLiveObjects();

	void Create(IDXGIAdapter4* pAdapter);
	~Device();

	operator auto() const { return m_Device5.Get(); }
	auto operator->() { return m_Device5.Get(); }

	inline auto GraphicsMemory() const { return m_GraphicsMemory.get(); }
	inline auto Allocator() const { return m_Allocator; }
private:
	void CheckRootSignature_1_1Support();
	void CheckShaderModel6PlusSupport();
	void CheckRaytracingSupport();
	void CheckMeshShaderSupport();

	Microsoft::WRL::ComPtr<ID3D12Device5> m_Device5;
	std::unique_ptr<DirectX::GraphicsMemory> m_GraphicsMemory;
	D3D12MA::Allocator* m_Allocator = nullptr;
};

inline bool IsCPUWritable(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
{
	assert(HeapType == D3D12_HEAP_TYPE_CUSTOM ? pCustomHeapProperties != nullptr : true);
	return HeapType == D3D12_HEAP_TYPE_UPLOAD ||
		(HeapType == D3D12_HEAP_TYPE_CUSTOM &&
			(pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE || pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
}

inline bool IsCPUInaccessible(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
{
	assert(HeapType == D3D12_HEAP_TYPE_CUSTOM ? pCustomHeapProperties != nullptr : true);
	return HeapType == D3D12_HEAP_TYPE_DEFAULT ||
		(HeapType == D3D12_HEAP_TYPE_CUSTOM &&
			(pCustomHeapProperties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE));
}

inline D3D12_RESOURCE_STATES DetermineInitialResourceState(D3D12_HEAP_TYPE HeapType, const D3D12_HEAP_PROPERTIES* pCustomHeapProperties = nullptr)
{
	if (HeapType == D3D12_HEAP_TYPE_DEFAULT || IsCPUWritable(HeapType, pCustomHeapProperties))
	{
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	assert(HeapType == D3D12_HEAP_TYPE_READBACK);
	return D3D12_RESOURCE_STATE_COPY_DEST;
}