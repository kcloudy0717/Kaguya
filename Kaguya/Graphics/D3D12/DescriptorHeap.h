#pragma once
#include "DeviceChild.h"

class DescriptorHeap : public DeviceChild
{
public:
	DescriptorHeap() = default;

	DescriptorHeap(Device* Parent)
		: DeviceChild(Parent)
	{
	}

	void SetName(LPCWSTR Name)
	{
		pDescriptorHeap->SetName(Name);
	}

	void Initialize(UINT NumDescriptors, bool ShaderVisible, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	operator ID3D12DescriptorHeap*() const { return pDescriptorHeap.Get(); }

	void Allocate(D3D12_CPU_DESCRIPTOR_HANDLE& hCPU, D3D12_GPU_DESCRIPTOR_HANDLE& hGPU, UINT& Index);

	void Release(UINT Index);

	D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT Index) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT Index) const noexcept;

private:
	struct IndexPool
	{
		IndexPool() = default;
		IndexPool(size_t NumIndices)
		{
			Elements.resize(NumIndices);
			Reset();
		}

		auto& operator[](size_t Index) { return Elements[Index]; }

		const auto& operator[](size_t Index) const { return Elements[Index]; }

		void Reset()
		{
			FreeStart		  = 0;
			NumActiveElements = 0;
			for (size_t i = 0; i < Elements.size(); ++i)
			{
				Elements[i] = i + 1;
			}
		}

		// Removes the first element from the free list and returns its index
		size_t Allocate()
		{
			assert(NumActiveElements < Elements.size() && "Consider increasing the size of the pool");
			NumActiveElements++;
			size_t index = FreeStart;
			FreeStart	 = Elements[index];
			return index;
		}

		void Release(size_t Index)
		{
			NumActiveElements--;
			Elements[Index] = FreeStart;
			FreeStart		= Index;
		}

		std::vector<size_t> Elements;
		size_t				FreeStart;
		size_t				NumActiveElements;
	};

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE					 hCPUStart;
	D3D12_GPU_DESCRIPTOR_HANDLE					 hGPUStart;
	UINT										 DescriptorStride;
	IndexPool									 IndexPool;
	std::mutex									 Mutex;
};
