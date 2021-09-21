#pragma once
#include "D3D12Common.h"
#include "D3D12RootSignature.h"

class D3D12DescriptorHeap : public D3D12LinkedDeviceChild
{
public:
	D3D12DescriptorHeap() noexcept = default;
	explicit D3D12DescriptorHeap(D3D12LinkedDevice* Parent) noexcept
		: D3D12LinkedDeviceChild(Parent)
	{
	}

	void SetName(LPCWSTR Name) const { DescriptorHeap->SetName(Name); }

	void Initialize(UINT NumDescriptors, bool ShaderVisible, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	operator ID3D12DescriptorHeap*() const { return DescriptorHeap.Get(); }

	void Allocate(
		D3D12_CPU_DESCRIPTOR_HANDLE& CpuDescriptorHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE& GpuDescriptorHandle,
		UINT&						 Index);

	void Release(UINT Index);

	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT Index) const noexcept;
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT Index) const noexcept;

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc = {};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE					 CpuBaseAddress				   = {};
	D3D12_GPU_DESCRIPTOR_HANDLE					 GpuBaseAddress				   = {};
	UINT										 DescriptorHandleIncrementSize = 0;
	IndexPool									 IndexPool;
	CriticalSection								 Mutex;
};

// Everything below is not used anymore, I exclusively use D3D12DescriptorHeap for bindless
// resource indexing, these classes are used in my older D3D12 project when I first started it learning
// D3D12 overtime I adapted to bindless model, the older project is now archived and these are here for book keeping
// purposes

// Many of these came from the DirectX-Graphics samples (MiniEngine)
class D3D12DescriptorPage;

// Represents a contiguous CPU descriptor handle
class D3D12DescriptorArray
{
public:
	D3D12DescriptorArray() noexcept = default;
	D3D12DescriptorArray(
		D3D12DescriptorPage*		Parent,
		D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle,
		UINT						Offset,
		UINT						NumDescriptors) noexcept
		: Parent(Parent)
		, CpuDescriptorHandle(CpuDescriptorHandle)
		, Offset(Offset)
		, NumDescriptors(NumDescriptors)
	{
	}

	D3D12DescriptorArray(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept
		: Parent(std::exchange(D3D12DescriptorArray.Parent, {}))
		, CpuDescriptorHandle(std::exchange(D3D12DescriptorArray.CpuDescriptorHandle, {}))
		, Offset(std::exchange(D3D12DescriptorArray.Offset, {}))
		, NumDescriptors(std::exchange(D3D12DescriptorArray.NumDescriptors, {}))
	{
	}

	D3D12DescriptorArray& operator=(D3D12DescriptorArray&& D3D12DescriptorArray) noexcept
	{
		if (this == &D3D12DescriptorArray)
		{
			return *this;
		}

		this->~D3D12DescriptorArray();
		Parent				= std::exchange(D3D12DescriptorArray.Parent, {});
		CpuDescriptorHandle = std::exchange(D3D12DescriptorArray.CpuDescriptorHandle, {});
		Offset				= std::exchange(D3D12DescriptorArray.Offset, {});
		NumDescriptors		= std::exchange(D3D12DescriptorArray.NumDescriptors, {});

		return *this;
	}

	~D3D12DescriptorArray();

	[[nodiscard]] bool IsValid() const noexcept { return Parent != nullptr; }

	D3D12_CPU_DESCRIPTOR_HANDLE operator[](UINT Index) const noexcept;

	[[nodiscard]] UINT GetOffset() const noexcept { return Offset; }
	[[nodiscard]] UINT GetNumDescriptors() const noexcept { return NumDescriptors; }

private:
	D3D12DescriptorPage*		Parent				= nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle = {};
	UINT						Offset				= 0;
	UINT						NumDescriptors		= 0;
};

class D3D12DescriptorPage : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12DescriptorPage(D3D12LinkedDevice* Parent) noexcept
		: D3D12LinkedDeviceChild(Parent)
	{
	}

	void Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	[[nodiscard]] UINT GetDescriptorHandleIncrementSize() const noexcept { return DescriptorHandleIncrementSize; }

	std::optional<D3D12DescriptorArray> Allocate(UINT NumDescriptors);
	void								Release(D3D12DescriptorArray&& DescriptorArray);

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
		FreeBlocksByOffsetPoolIter(CFreeBlocksByOffsetPool::iterator Iterator)
			: Iterator(Iterator)
		{
		}
	};

	struct FreeBlocksBySizePoolIter
	{
		CFreeBlocksBySizePool::iterator Iterator;
		FreeBlocksBySizePoolIter(CFreeBlocksBySizePool::iterator Iterator)
			: Iterator(Iterator)
		{
		}
	};

	void AllocateFreeBlock(TOffset Offset, TSize Size);
	void FreeBlock(TOffset Offset, TSize Size);

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc = {};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE					 CpuBaseAddress				   = {};
	UINT										 DescriptorHandleIncrementSize = 0;

	CFreeBlocksByOffsetPool FreeBlocksByOffsetPool;
	CFreeBlocksBySizePool	SizePool;
	CriticalSection			Mutex;
};

class D3D12DescriptorAllocator : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12DescriptorAllocator(D3D12LinkedDevice* Parent, D3D12_DESCRIPTOR_HEAP_TYPE Type) noexcept
		: D3D12LinkedDeviceChild(Parent)
		, Type(Type)
	{
	}

	D3D12DescriptorArray Allocate(UINT NumDescriptors);

private:
	D3D12_DESCRIPTOR_HEAP_TYPE						  Type;
	std::vector<std::unique_ptr<D3D12DescriptorPage>> DescriptorPages;
};

struct D3D12DescriptorTableCache
{
	// The pointer to the descriptor in the descriptor handle cache.
	D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
	UINT						 NumDescriptors;
};

class D3D12DescriptorHandleCache : public D3D12LinkedDeviceChild
{
public:
	static constexpr UINT DescriptorHandleLimit = 256;

	explicit D3D12DescriptorHandleCache(D3D12LinkedDevice* Parent) noexcept
		: D3D12LinkedDeviceChild(Parent)
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

	void ParseRootSignature(const D3D12RootSignature& RootSignature, D3D12_DESCRIPTOR_HEAP_TYPE Type)
	{
		StaleDescriptorTableBitMask.reset();

		DescriptorTableBitMask = RootSignature.GetDescriptorTableBitMask(Type);
		if (!DescriptorTableBitMask.any())
		{
			return;
		}

		UINT DescriptorOffset = 0;
		for (size_t i = 0; i < DescriptorTableBitMask.size(); ++i)
		{
			if (DescriptorTableBitMask.test(i))
			{
				UINT RootParameterIndex = static_cast<UINT>(i);
				if (RootParameterIndex < RootSignature.GetDesc().NumParameters)
				{
					UINT NumDescriptors = RootSignature.GetNumDescriptors(RootParameterIndex);

					D3D12DescriptorTableCache& DescriptorTableCache = DescriptorTableCaches[RootParameterIndex];
					DescriptorTableCache.NumDescriptors				= NumDescriptors;
					DescriptorTableCache.BaseDescriptor				= DescriptorHandles + DescriptorOffset;

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
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);

	UINT CommitDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE& DestCpuHandle,
		CD3DX12_GPU_DESCRIPTOR_HANDLE& DestGpuHandle,
		ID3D12GraphicsCommandList*	   CommandList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

private:
	D3D12_DESCRIPTOR_HEAP_TYPE Type							 = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	UINT					   DescriptorHandleIncrementSize = 0;

	// Each bit in the bit mask represents the index in the root signature
	// that contains a descriptor table.
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> DescriptorTableBitMask;

	// Each bit set in the bit mask represents a descriptor table
	// in the root signature that has changed since the last time the
	// descriptors were copied.
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> StaleDescriptorTableBitMask;

	D3D12DescriptorTableCache	DescriptorTableCaches[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandles[DescriptorHandleLimit]						= {};
};

class D3D12DynamicDescriptorHeap : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12DynamicDescriptorHeap(D3D12LinkedDevice* Parent) noexcept
		: D3D12LinkedDeviceChild(Parent)
		, GraphicsHandleCache(Parent)
		, ComputeHandleCache(Parent)
	{
	}

	void Initialize(UINT NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE Type);

	void Reset();

	void ParseGraphicsRootSignature(const D3D12RootSignature& RootSignature);
	void ParseComputeRootSignature(const D3D12RootSignature& RootSignature);

	void SetGraphicsDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);
	void SetComputeDescriptorHandles(
		UINT						RootParameterIndex,
		UINT						Offset,
		UINT						NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor);

	void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CommandList);
	void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CommandList);

private:
	D3D12_DESCRIPTOR_HEAP_DESC					 Desc = {};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE				 CpuBaseAddress				   = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE				 GpuBaseAddress				   = {};
	UINT										 DescriptorHandleIncrementSize = 0;

	D3D12DescriptorHandleCache GraphicsHandleCache;
	D3D12DescriptorHandleCache ComputeHandleCache;
};
