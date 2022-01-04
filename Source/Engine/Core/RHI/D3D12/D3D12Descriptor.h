#pragma once
#include "D3D12Common.h"
#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

class CSubresourceSubset
{
public:
	CSubresourceSubset() noexcept = default;
	explicit CSubresourceSubset(
		UINT8  NumMips,
		UINT16 NumArraySlices,
		UINT8  NumPlanes,
		UINT8  FirstMip		   = 0,
		UINT16 FirstArraySlice = 0,
		UINT8  FirstPlane	   = 0) noexcept;
	explicit CSubresourceSubset(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc) noexcept;
	explicit CSubresourceSubset(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc) noexcept;
	explicit CSubresourceSubset(const D3D12_RENDER_TARGET_VIEW_DESC& Desc) noexcept;
	explicit CSubresourceSubset(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc) noexcept;

	bool DoesNotOverlap(const CSubresourceSubset& CSubresourceSubset) const noexcept;

protected:
	UINT16 BeginArray; // Also used to store Tex3D slices.
	UINT16 EndArray;   // End - Begin == Array Slices
	UINT8  BeginMip;
	UINT8  EndMip; // End - Begin == Mip Levels
	UINT8  BeginPlane;
	UINT8  EndPlane;
};

class CViewSubresourceSubset : public CSubresourceSubset
{
public:
	enum DepthStencilMode
	{
		ReadOnly,
		WriteOnly,
		ReadOrWrite
	};

	CViewSubresourceSubset() noexcept = default;
	explicit CViewSubresourceSubset(
		const CSubresourceSubset& Subresources,
		UINT8					  MipLevels,
		UINT16					  ArraySize,
		UINT8					  PlaneCount);
	explicit CViewSubresourceSubset(
		const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
		UINT8								   MipLevels,
		UINT16								   ArraySize,
		UINT8								   PlaneCount);
	explicit CViewSubresourceSubset(
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
		UINT8									MipLevels,
		UINT16									ArraySize,
		UINT8									PlaneCount);
	explicit CViewSubresourceSubset(
		const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
		UINT8								 MipLevels,
		UINT16								 ArraySize,
		UINT8								 PlaneCount);
	explicit CViewSubresourceSubset(
		const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
		UINT8								 MipLevels,
		UINT16								 ArraySize,
		UINT8								 PlaneCount,
		DepthStencilMode					 DSMode = ReadOrWrite);

	template<typename T>
	static CViewSubresourceSubset FromView(const T* pView)
	{
		return CViewSubresourceSubset(
			pView->GetDesc(),
			static_cast<UINT8>(pView->GetResource()->GetMipLevels()),
			static_cast<UINT16>(pView->GetResource()->GetArraySize()),
			static_cast<UINT8>(pView->GetResource()->GetPlaneCount()));
	}

public:
	class CViewSubresourceIterator;

public:
	CViewSubresourceIterator begin() const;
	CViewSubresourceIterator end() const;
	bool					 IsWholeResource() const
	{
		return BeginMip == 0 && BeginArray == 0 && BeginPlane == 0 &&
			   (EndMip * EndArray * EndPlane == MipLevels * ArraySlices * PlaneCount);
	}
	bool IsEmpty() const { return BeginMip == EndMip || BeginArray == EndArray || BeginPlane == EndPlane; }
	UINT ArraySize() const { return ArraySlices; }

	UINT MinSubresource() const;
	UINT MaxSubresource() const;

private:
	void Reduce()
	{
		if (BeginMip == 0 && EndMip == MipLevels && BeginArray == 0 && EndArray == ArraySlices)
		{
			UINT startSubresource = D3D12CalcSubresource(0, 0, BeginPlane, MipLevels, ArraySlices);
			UINT endSubresource	  = D3D12CalcSubresource(0, 0, EndPlane, MipLevels, ArraySlices);

			// Only coalesce if the full-resolution UINTs fit in the UINT8s used
			// for storage here
			if (endSubresource < static_cast<UINT8>(-1))
			{
				BeginArray = 0;
				EndArray   = 1;
				BeginPlane = 0;
				EndPlane   = 1;
				BeginMip   = static_cast<UINT8>(startSubresource);
				EndMip	   = static_cast<UINT8>(endSubresource);
			}
		}
	}

protected:
	UINT8  MipLevels;
	UINT16 ArraySlices;
	UINT8  PlaneCount;
};

// This iterator iterates over contiguous ranges of subresources within a
// subresource subset. eg:
//
// // For each contiguous subresource range.
// for( CViewSubresourceSubset::CViewSubresourceIterator it =
// ViewSubset.begin(); it != ViewSubset.end(); ++it )
// {
//      // StartSubresource and EndSubresource members of the iterator describe
//      the contiguous range. for( UINT SubresourceIndex =
//      it.StartSubresource(); SubresourceIndex < it.EndSubresource();
//      SubresourceIndex++ )
//      {
//          // Action for each subresource within the current range.
//      }
//  }
//
class CViewSubresourceSubset::CViewSubresourceIterator
{
public:
	explicit CViewSubresourceIterator(
		const CViewSubresourceSubset& ViewSubresourceSubset,
		UINT16						  ArraySlice,
		UINT8						  PlaneSlice)
		: ViewSubresourceSubset(ViewSubresourceSubset)
		, CurrentArraySlice(ArraySlice)
		, CurrentPlaneSlice(PlaneSlice)
	{
	}
	CViewSubresourceIterator& operator++()
	{
		assert(CurrentArraySlice < ViewSubresourceSubset.EndArray);

		if (++CurrentArraySlice >= ViewSubresourceSubset.EndArray)
		{
			assert(CurrentPlaneSlice < ViewSubresourceSubset.EndPlane);
			CurrentArraySlice = ViewSubresourceSubset.BeginArray;
			++CurrentPlaneSlice;
		}

		return *this;
	}
	CViewSubresourceIterator& operator--()
	{
		if (CurrentArraySlice <= ViewSubresourceSubset.BeginArray)
		{
			CurrentArraySlice = ViewSubresourceSubset.EndArray;

			assert(CurrentPlaneSlice > ViewSubresourceSubset.BeginPlane);
			--CurrentPlaneSlice;
		}

		--CurrentArraySlice;

		return *this;
	}

	bool operator==(const CViewSubresourceIterator& other) const
	{
		return &other.ViewSubresourceSubset == &ViewSubresourceSubset && other.CurrentArraySlice == CurrentArraySlice &&
			   other.CurrentPlaneSlice == CurrentPlaneSlice;
	}
	bool operator!=(const CViewSubresourceIterator& other) const { return !(other == *this); }

	UINT StartSubresource() const
	{
		return D3D12CalcSubresource(
			ViewSubresourceSubset.BeginMip,
			CurrentArraySlice,
			CurrentPlaneSlice,
			ViewSubresourceSubset.MipLevels,
			ViewSubresourceSubset.ArraySlices);
	}
	UINT EndSubresource() const
	{
		return D3D12CalcSubresource(
			ViewSubresourceSubset.EndMip,
			CurrentArraySlice,
			CurrentPlaneSlice,
			ViewSubresourceSubset.MipLevels,
			ViewSubresourceSubset.ArraySlices);
	}
	std::pair<UINT, UINT> operator*() const { return std::make_pair(StartSubresource(), EndSubresource()); }

private:
	const CViewSubresourceSubset& ViewSubresourceSubset;
	UINT16						  CurrentArraySlice;
	UINT8						  CurrentPlaneSlice;
};

template<typename ViewDesc>
struct D3D12DescriptorTraits
{
};
template<>
struct D3D12DescriptorTraits<D3D12_SHADER_RESOURCE_VIEW_DESC>
{
	static decltype(&ID3D12Device::CreateShaderResourceView) Create() { return &ID3D12Device::CreateShaderResourceView; }
};
template<>
struct D3D12DescriptorTraits<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
	static decltype(&ID3D12Device::CreateUnorderedAccessView) Create() { return &ID3D12Device::CreateUnorderedAccessView; }
};

template<typename ViewDesc>
class D3D12Descriptor : public D3D12LinkedDeviceChild
{
public:
	D3D12Descriptor() noexcept = default;
	explicit D3D12Descriptor(D3D12LinkedDevice* Parent)
		: D3D12LinkedDeviceChild(Parent)
	{
		if (Parent)
		{
			D3D12DescriptorHeap& DescriptorHeap = Parent->GetDescriptorHeap<ViewDesc>();
			DescriptorHeap.Allocate(CpuHandle, GpuHandle, Index);
		}
	}
	~D3D12Descriptor()
	{
		InternalDestroy();
	}

	D3D12Descriptor(D3D12Descriptor&& D3D12Descriptor) noexcept
		: D3D12LinkedDeviceChild(std::exchange(D3D12Descriptor.Parent, {}))
		, CpuHandle(std::exchange(D3D12Descriptor.CpuHandle, {}))
		, GpuHandle(std::exchange(D3D12Descriptor.GpuHandle, {}))
		, Index(std::exchange(D3D12Descriptor.Index, UINT_MAX))
	{
	}
	D3D12Descriptor& operator=(D3D12Descriptor&& D3D12Descriptor) noexcept
	{
		if (this == &D3D12Descriptor)
		{
			return *this;
		}

		InternalDestroy();
		Parent	  = std::exchange(D3D12Descriptor.Parent, {});
		CpuHandle = std::exchange(D3D12Descriptor.CpuHandle, {});
		GpuHandle = std::exchange(D3D12Descriptor.GpuHandle, {});
		Index	  = std::exchange(D3D12Descriptor.Index, UINT_MAX);

		return *this;
	}

	D3D12Descriptor(const D3D12Descriptor&) = delete;
	D3D12Descriptor& operator=(const D3D12Descriptor&) = delete;

	[[nodiscard]] bool						  IsValid() const noexcept { return Index != UINT_MAX; }
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept
	{
		assert(IsValid());
		return CpuHandle;
	}
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept
	{
		assert(IsValid());
		return GpuHandle;
	}
	[[nodiscard]] UINT GetIndex() const noexcept
	{
		assert(IsValid());
		return Index;
	}

	void CreateDefaultView(ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(Resource, nullptr, CpuHandle);
	}

	void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(Resource, CounterResource, nullptr, CpuHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource)
	{
		(GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(Resource, &Desc, CpuHandle);
	}

	void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource)
	{
		(GetParentLinkedDevice()->GetDevice()->*D3D12DescriptorTraits<ViewDesc>::Create())(Resource, CounterResource, &Desc, CpuHandle);
	}

private:
	void InternalDestroy()
	{
		if (Parent && IsValid())
		{
			D3D12DescriptorHeap& DescriptorHeap = Parent->GetDescriptorHeap<ViewDesc>();
			DescriptorHeap.Release(Index);
			Parent	  = nullptr;
			CpuHandle = { NULL };
			GpuHandle = { NULL };
			Index	  = UINT_MAX;
		}
	}

protected:
	D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { NULL };
	D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { NULL };
	UINT						Index	  = UINT_MAX;
};

template<typename ViewDesc>
class D3D12View
{
public:
	D3D12View() noexcept = default;
	explicit D3D12View(D3D12LinkedDevice* Device, const ViewDesc& Desc, D3D12Resource* Resource)
		: Descriptor(Device)
		, Desc(Desc)
		, Resource(Resource)
		, ViewSubresourceSubset(Resource ? CViewSubresourceSubset::FromView(this) : CViewSubresourceSubset())
	{
	}

	D3D12View(D3D12View&&) noexcept = default;
	D3D12View& operator=(D3D12View&&) = default;

	D3D12View(const D3D12View&) = delete;
	D3D12View& operator=(const D3D12View&) = delete;

	[[nodiscard]] bool						  IsValid() const noexcept { return Descriptor.IsValid(); }
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept { return Descriptor.GetCpuHandle(); }
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return Descriptor.GetGpuHandle(); }
	[[nodiscard]] UINT						  GetIndex() const noexcept { return Descriptor.GetIndex(); }
	[[nodiscard]] ViewDesc					  GetDesc() const noexcept { return Desc; }
	[[nodiscard]] D3D12Resource*			  GetResource() const noexcept { return Resource; }

protected:
	D3D12Descriptor<ViewDesc> Descriptor;
	ViewDesc				  Desc	   = {};
	D3D12Resource*			  Resource = nullptr;
	CViewSubresourceSubset	  ViewSubresourceSubset;
};

class D3D12ShaderResourceView : public D3D12View<D3D12_SHADER_RESOURCE_VIEW_DESC>
{
public:
	D3D12ShaderResourceView() noexcept = default;
	explicit D3D12ShaderResourceView(
		D3D12LinkedDevice*					   Device,
		const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
		D3D12Resource*						   Resource);
	explicit D3D12ShaderResourceView(
		D3D12LinkedDevice* Device,
		D3D12ASBuffer*	   ASBuffer);
	explicit D3D12ShaderResourceView(
		D3D12LinkedDevice* Device,
		D3D12Buffer*	   Buffer,
		bool			   Raw,
		UINT			   FirstElement,
		UINT			   NumElements);
	explicit D3D12ShaderResourceView(
		D3D12LinkedDevice*	Device,
		D3D12Texture*		Texture,
		bool				sRGB,
		std::optional<UINT> OptMostDetailedMip,
		std::optional<UINT> OptMipLevels);

	void RecreateView();

private:
	static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(
		D3D12ASBuffer* ASBuffer);
	static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(
		D3D12Buffer* Buffer,
		bool		 Raw,
		UINT		 FirstElement,
		UINT		 NumElements);
	static D3D12_SHADER_RESOURCE_VIEW_DESC GetDesc(
		D3D12Texture*		Texture,
		bool				sRGB,
		std::optional<UINT> OptMostDetailedMip,
		std::optional<UINT> OptMipLevels);
};

class D3D12UnorderedAccessView : public D3D12View<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
public:
	D3D12UnorderedAccessView() noexcept = default;
	explicit D3D12UnorderedAccessView(
		D3D12LinkedDevice*						Device,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
		D3D12Resource*							Resource,
		D3D12Resource*							CounterResource = nullptr);
	explicit D3D12UnorderedAccessView(
		D3D12LinkedDevice* Device,
		D3D12Buffer*	   Buffer,
		UINT			   NumElements,
		UINT64			   CounterOffsetInBytes);
	explicit D3D12UnorderedAccessView(
		D3D12LinkedDevice*	Device,
		D3D12Texture*		Texture,
		std::optional<UINT> OptArraySlice,
		std::optional<UINT> OptMipSlice);

	void RecreateView();

private:
	static D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc(
		D3D12Buffer* Buffer,
		UINT		 NumElements,
		UINT64		 CounterOffsetInBytes);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC GetDesc(
		D3D12Texture*		Texture,
		std::optional<UINT> OptArraySlice,
		std::optional<UINT> OptMipSlice);

private:
	D3D12Resource* CounterResource = nullptr;
};
