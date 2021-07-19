#pragma once
#include "D3D12Common.h"
#include "RootSignature.h"

class DynamicResourceDescriptorHeap : public DeviceChild
{
public:
	DynamicResourceDescriptorHeap() = default;

	DynamicResourceDescriptorHeap(Device* Parent)
		: DeviceChild(Parent)
	{
	}

	void SetName(LPCWSTR Name) { pDescriptorHeap->SetName(Name); }

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
	UINT										 DescriptorHandleIncrementSize;
	IndexPool									 IndexPool;
	CriticalSection								 Mutex;
};

// Everything below is not used anymore, I exclusively use DynamicResourceDescriptorHeap for bindless
// resource indexing, these classes are used in my older D3D12 project when I first started it learning
// D3D12 overtime I adapted to bindless model, the older project is now archived and these are here for book keeping
// purposes
class DescriptorPage;

// Represents a contiguous CPU descriptor handle, mainly used to stage from CPU -> GPU
class DescriptorArray
{
public:
	DescriptorArray() = default;

	DescriptorArray(
		DescriptorPage*				Parent,
		D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle,
		UINT						Offset,
		UINT						NumDescriptors)
		: Parent(Parent)
		, CPUDescriptorHandle(CPUDescriptorHandle)
		, Offset(Offset)
		, NumDescriptors(NumDescriptors)
	{
	}

	DescriptorArray(DescriptorArray&& rvalue) noexcept
		: Parent(std::exchange(rvalue.Parent, {}))
		, CPUDescriptorHandle(std::exchange(rvalue.CPUDescriptorHandle, {}))
		, Offset(std::exchange(rvalue.Offset, {}))
		, NumDescriptors(std::exchange(rvalue.NumDescriptors, {}))
	{
	}

	DescriptorArray& operator=(DescriptorArray&& rvalue) noexcept
	{
		if (this != &rvalue)
		{
			Parent				= std::exchange(rvalue.Parent, {});
			CPUDescriptorHandle = std::exchange(rvalue.CPUDescriptorHandle, {});
			Offset				= std::exchange(rvalue.Offset, {});
			NumDescriptors		= std::exchange(rvalue.NumDescriptors, {});
		}
		return *this;
	}

	~DescriptorArray();

	bool IsValid() const noexcept { return Parent != nullptr; }

	D3D12_CPU_DESCRIPTOR_HANDLE operator[](INT Index) const noexcept;

	UINT GetOffset() const noexcept { return Offset; }
	UINT GetNumDescriptors() const noexcept { return NumDescriptors; }

private:
	DescriptorPage*				Parent				= nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle = { NULL };
	UINT						Offset				= 0;
	UINT						NumDescriptors		= 0;
};

class DescriptorPage : public DeviceChild
{
public:
	DescriptorPage() = default;

	DescriptorPage(Device* Parent)
		: DeviceChild(Parent)
	{
	}

	void Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	UINT GetDescriptorHandleIncrementSize() { return DescriptorHandleIncrementSize; }

	std::optional<DescriptorArray> Allocate(UINT NumDescriptors);
	void						   Release(DescriptorArray&& DescriptorArray);

private:
	using TOffset = UINT;
	using TSize	  = UINT;

	struct FreeBlocksByOffsetPoolIter;
	struct FreeBlocksBySizePoolIter;
	using CFreeBlocksByOffsetPool = std::map<TOffset, FreeBlocksBySizePoolIter>;
	using CFreeBlocksBySizePool	  = std::multimap<TSize, FreeBlocksByOffsetPoolIter>;

	struct FreeBlocksByOffsetPoolIter
	{
		CFreeBlocksByOffsetPool::iterator Iterator;
		FreeBlocksByOffsetPoolIter(CFreeBlocksByOffsetPool::iterator iter)
			: Iterator(iter)
		{
		}
	};

	struct FreeBlocksBySizePoolIter
	{
		CFreeBlocksBySizePool::iterator Iterator;
		FreeBlocksBySizePoolIter(CFreeBlocksBySizePool::iterator iter)
			: Iterator(iter)
		{
		}
	};

	void AllocateFreeBlock(TOffset Offset, TSize Size);
	void FreeBlock(TOffset Offset, TSize Size);

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE					 hCPUStart;
	UINT										 DescriptorHandleIncrementSize;

	CFreeBlocksByOffsetPool FreeBlocksByOffsetPool;
	CFreeBlocksBySizePool	SizePool;
	CriticalSection			Mutex;
};

class DescriptorAllocator : public DeviceChild
{
public:
	DescriptorAllocator(Device* Parent, D3D12_DESCRIPTOR_HEAP_TYPE Type)
		: DeviceChild(Parent)
		, Type(Type)
	{
	}

	DescriptorArray Allocate(UINT NumDescriptors);

private:
	D3D12_DESCRIPTOR_HEAP_TYPE					 Type;
	std::vector<std::unique_ptr<DescriptorPage>> DescriptorPages;
};

struct DescriptorTableCache
{
	// The pointer to the descriptor in the descriptor handle cache.
	D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
	UINT						 NumDescriptors;
};

class DescriptorHandleCache : public DeviceChild
{
public:
	static constexpr UINT DescriptorHandleLimit = 256;

	DescriptorHandleCache(Device* Parent)
		: DeviceChild(Parent)
	{
	}

	void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT DescriptorHandleIncrementSize)
	{
		this->Type							= Type;
		this->DescriptorHandleIncrementSize = DescriptorHandleIncrementSize;
	}

	void Reset()
	{
		DescriptorTableBitMask.reset();
		StaleDescriptorTableBitMask.reset();

		for (auto& DescriptorTableCache : DescriptorTableCaches)
		{
			DescriptorTableCache = {};
		}
	}

	void ParseRootSignature(const RootSignature& RootSignature, D3D12_DESCRIPTOR_HEAP_TYPE Type)
	{
		StaleDescriptorTableBitMask.reset();

		DescriptorTableBitMask = RootSignature.GetDescriptorTableBitMask(Type);
		if (!DescriptorTableBitMask.any())
		{
			return;
		}

		UINT DescriptorOffset	= 0;
		UINT RootParameterIndex = 0;
		for (size_t i = 0; i < DescriptorTableBitMask.size(); ++i)
		{
			if (DescriptorTableBitMask.test(i))
			{
				RootParameterIndex = static_cast<UINT>(i);
				if (RootParameterIndex < RootSignature.GetDesc().NumParameters)
				{
					UINT NumDescriptors = RootSignature.GetNumDescriptors(RootParameterIndex);

					DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];
					DescriptorTableCache.NumDescriptors		   = NumDescriptors;
					DescriptorTableCache.BaseDescriptor		   = DescriptorHandles + DescriptorOffset;

					DescriptorOffset += NumDescriptors;
				}
			}
		}

		// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
		assert(DescriptorOffset <= DescriptorHandleLimit && "Exceeded user-supplied maximum cache size");
	}

	void StageDescriptors(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		assert(
			RootParameterIndex >= 0 && RootParameterIndex < D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT &&
			"Root parameter index exceeds the max descriptor table index");

		DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];

		assert(
			Offset + NumDescriptors <= DescriptorTableCache.NumDescriptors &&
			"Number of descriptors exceeds the number of descripotrs in the descriptor table");

		D3D12_CPU_DESCRIPTOR_HANDLE* DstDescriptor = DescriptorTableCache.BaseDescriptor + Offset;
		for (UINT i = 0; i < NumDescriptors; ++i)
		{
			DstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(SrcDescriptor, i, DescriptorHandleIncrementSize);
		}

		StaleDescriptorTableBitMask.set(RootParameterIndex, true);
	}

	UINT CommitDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hDestCPU,
		CD3DX12_GPU_DESCRIPTOR_HANDLE& hDestGPU,
		ID3D12GraphicsCommandList*	   pCommandList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*pfnSetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

private:
	D3D12_DESCRIPTOR_HEAP_TYPE Type;
	UINT					   DescriptorHandleIncrementSize;

	// Each bit in the bit mask represents the index in the root signature
	// that contains a descriptor table.
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> DescriptorTableBitMask;

	// Each bit set in the bit mask represents a descriptor table
	// in the root signature that has changed since the last time the
	// descriptors were copied.
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> StaleDescriptorTableBitMask;

	DescriptorTableCache		DescriptorTableCaches[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT];
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandles[DescriptorHandleLimit];
};

class DynamicDescriptorHeap : public DeviceChild
{
public:
	DynamicDescriptorHeap(Device* Parent)
		: DeviceChild(Parent)
		, GraphicsHandleCache(Parent)
		, ComputeHandleCache(Parent)
	{
	}

	void Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	void Reset();

	void ParseGraphicsRootSignature(const RootSignature& RootSignature)
	{
		GraphicsHandleCache.ParseRootSignature(RootSignature, Desc.Type);
	}
	void ParseComputeRootSignature(const RootSignature& RootSignature)
	{
		ComputeHandleCache.ParseRootSignature(RootSignature, Desc.Type);
	}

	void SetGraphicsDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		GraphicsHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
	}

	void SetComputeDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor)
	{
		ComputeHandleCache.StageDescriptors(RootParameterIndex, Offset, NumDescriptors, SrcDescriptor);
	}

	void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* pCommandList)
	{
		UINT NumDescriptors = GraphicsHandleCache.CommitDescriptors(
			hCPU,
			hGPU,
			pCommandList,
			&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
		Desc.NumDescriptors -= NumDescriptors;
	}
	void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* pCommandList)
	{
		UINT NumDescriptors = ComputeHandleCache.CommitDescriptors(
			hCPU,
			hGPU,
			pCommandList,
			&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
		Desc.NumDescriptors -= NumDescriptors;
	}

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE				 hCPU;
	CD3DX12_GPU_DESCRIPTOR_HANDLE				 hGPU;
	UINT										 DescriptorHandleIncrementSize;

	DescriptorHandleCache GraphicsHandleCache;
	DescriptorHandleCache ComputeHandleCache;
};
