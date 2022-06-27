#pragma once
#include "D3D12Core.h"
#include "D3D12Resource.h"

namespace RHI
{
	namespace Internal
	{
		template<typename ViewDesc>
		struct D3D12DescriptorTraits
		{
		};
		template<>
		struct D3D12DescriptorTraits<D3D12_RENDER_TARGET_VIEW_DESC>
		{
			static auto Create() { return &ID3D12Device::CreateRenderTargetView; }
		};
		template<>
		struct D3D12DescriptorTraits<D3D12_DEPTH_STENCIL_VIEW_DESC>
		{
			static auto Create() { return &ID3D12Device::CreateDepthStencilView; }
		};
		template<>
		struct D3D12DescriptorTraits<D3D12_SHADER_RESOURCE_VIEW_DESC>
		{
			static auto Create() { return &ID3D12Device::CreateShaderResourceView; }
		};
		template<>
		struct D3D12DescriptorTraits<D3D12_UNORDERED_ACCESS_VIEW_DESC>
		{
			static auto Create() { return &ID3D12Device::CreateUnorderedAccessView; }
		};
		template<>
		struct D3D12DescriptorTraits<D3D12_SAMPLER_DESC>
		{
			static auto Create() { return &ID3D12Device::CreateSampler; }
		};
	} // namespace Internal

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

	protected:
		UINT16 BeginArray = 0; // Also used to store Tex3D slices.
		UINT16 EndArray	  = 0; // End - Begin == Array Slices
		UINT8  BeginMip	  = 0;
		UINT8  EndMip	  = 0; // End - Begin == Mip Levels
		UINT8  BeginPlane = 0;
		UINT8  EndPlane	  = 0;
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
		[[nodiscard]] CViewSubresourceIterator begin() const;
		[[nodiscard]] CViewSubresourceIterator end() const;
		[[nodiscard]] bool					   IsWholeResource() const
		{
			return BeginMip == 0 && BeginArray == 0 && BeginPlane == 0 && (EndMip * EndArray * EndPlane == MipLevels * ArraySlices * PlaneCount);
		}
		[[nodiscard]] bool IsEmpty() const { return BeginMip == EndMip || BeginArray == EndArray || BeginPlane == EndPlane; }
		[[nodiscard]] UINT ArraySize() const { return ArraySlices; }

		[[nodiscard]] UINT MinSubresource() const;
		[[nodiscard]] UINT MaxSubresource() const;

	private:
		void Reduce();
		// Make compiler happy
		CViewSubresourceSubset(
			const D3D12_SAMPLER_DESC& Desc,
			UINT8					  MipLevels,
			UINT16					  ArraySize,
			UINT8					  PlaneCount)
		{
		}

	protected:
		UINT8  MipLevels   = 0;
		UINT16 ArraySlices = 0;
		UINT8  PlaneCount  = 0;
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

		bool operator==(const CViewSubresourceIterator& CViewSubresourceIterator) const
		{
			return &CViewSubresourceIterator.ViewSubresourceSubset == &ViewSubresourceSubset &&
				   CViewSubresourceIterator.CurrentArraySlice == CurrentArraySlice &&
				   CViewSubresourceIterator.CurrentPlaneSlice == CurrentPlaneSlice;
		}
		bool operator!=(const CViewSubresourceIterator& CViewSubresourceIterator) const
		{
			return !(CViewSubresourceIterator == *this);
		}

		[[nodiscard]] UINT StartSubresource() const
		{
			return D3D12CalcSubresource(
				ViewSubresourceSubset.BeginMip,
				CurrentArraySlice,
				CurrentPlaneSlice,
				ViewSubresourceSubset.MipLevels,
				ViewSubresourceSubset.ArraySlices);
		}
		[[nodiscard]] UINT EndSubresource() const
		{
			return D3D12CalcSubresource(
				ViewSubresourceSubset.EndMip,
				CurrentArraySlice,
				CurrentPlaneSlice,
				ViewSubresourceSubset.MipLevels,
				ViewSubresourceSubset.ArraySlices);
		}
		[[nodiscard]] std::pair<UINT, UINT> operator*() const { return std::make_pair(StartSubresource(), EndSubresource()); }

	private:
		const CViewSubresourceSubset& ViewSubresourceSubset;
		UINT16						  CurrentArraySlice;
		UINT8						  CurrentPlaneSlice;
	};

	template<typename ViewDesc, bool Dynamic>
	class D3D12Descriptor : public D3D12LinkedDeviceChild
	{
	public:
		D3D12Descriptor() noexcept = default;
		explicit D3D12Descriptor(D3D12LinkedDevice* Parent);
		D3D12Descriptor(D3D12Descriptor&& D3D12Descriptor) noexcept
			: D3D12LinkedDeviceChild(std::exchange(D3D12Descriptor.Parent, {}))
			, CpuHandle(std::exchange(D3D12Descriptor.CpuHandle, {}))
			, GpuHandle(std::exchange(D3D12Descriptor.GpuHandle, {}))
			, Index(std::exchange(D3D12Descriptor.Index, UINT_MAX))
		{
		}
		D3D12Descriptor& operator=(D3D12Descriptor&& D3D12Descriptor) noexcept
		{
			if (this != &D3D12Descriptor)
			{
				InternalDestroy();
				Parent	  = std::exchange(D3D12Descriptor.Parent, {});
				CpuHandle = std::exchange(D3D12Descriptor.CpuHandle, {});
				GpuHandle = std::exchange(D3D12Descriptor.GpuHandle, {});
				Index	  = std::exchange(D3D12Descriptor.Index, UINT_MAX);
			}
			return *this;
		}
		~D3D12Descriptor()
		{
			InternalDestroy();
		}

		D3D12Descriptor(const D3D12Descriptor&) = delete;
		D3D12Descriptor& operator=(const D3D12Descriptor&) = delete;

		[[nodiscard]] bool IsValid() const noexcept
		{
			return Index != UINT_MAX;
		}
		[[nodiscard]] bool IsShaderVisible() const noexcept
		{
			return Dynamic;
		}
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept
		{
			assert(IsValid());
			return CpuHandle;
		}
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept
		{
			static_assert(Dynamic, "Descriptor is not dynamic");
			assert(IsValid());
			return GpuHandle;
		}
		[[nodiscard]] UINT GetIndex() const noexcept
		{
			assert(IsValid());
			return Index;
		}

		void CreateDefaultView(ID3D12Resource* Resource);
		void CreateDefaultView(ID3D12Resource* Resource, ID3D12Resource* CounterResource);
		void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource);
		void CreateView(const ViewDesc& Desc, ID3D12Resource* Resource, ID3D12Resource* CounterResource);
		void CreateSampler(const ViewDesc& Desc);

	private:
		void InternalDestroy();

	protected:
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = {};
		// If Dynamic is true, this represents bindless descriptor handle index in D3D12DescriptorHeap
		// Else it represents descriptor heap entry index in CDescriptorHeapManager
		UINT Index = UINT_MAX;
	};

	template<typename ViewDesc, bool Dynamic>
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
		D3D12View& operator=(D3D12View&&) noexcept = default;

		D3D12View(const D3D12View&) = delete;
		D3D12View& operator=(const D3D12View&) = delete;

		[[nodiscard]] bool							IsValid() const noexcept { return Descriptor.IsValid(); }
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE	GetCpuHandle() const noexcept { return Descriptor.GetCpuHandle(); }
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE	GetGpuHandle() const noexcept { return Descriptor.GetGpuHandle(); }
		[[nodiscard]] UINT							GetIndex() const noexcept { return Descriptor.GetIndex(); }
		[[nodiscard]] ViewDesc						GetDesc() const noexcept { return Desc; }
		[[nodiscard]] D3D12Resource*				GetResource() const noexcept { return Resource; }
		[[nodiscard]] const CViewSubresourceSubset& GetViewSubresourceSubset() const noexcept { return ViewSubresourceSubset; }

	protected:
		D3D12Descriptor<ViewDesc, Dynamic> Descriptor;
		ViewDesc						   Desc		= {};
		D3D12Resource*					   Resource = nullptr;
		CViewSubresourceSubset			   ViewSubresourceSubset;
	};

	struct TextureSubresource
	{
		static constexpr UINT AllMipLevels	 = UINT(~0);
		static constexpr UINT AllArraySlices = UINT(~0);

		UINT MipSlice		= 0;
		UINT NumMipLevels	= AllMipLevels;
		UINT ArraySlice		= 0;
		UINT NumArraySlices = AllArraySlices;

		static TextureSubresource SRV(UINT MipSlice, UINT NumMipLevels, UINT ArraySlice, UINT NumArraySlices) noexcept
		{
			return { MipSlice, NumMipLevels, ArraySlice, NumArraySlices };
		}
		static TextureSubresource UAV(UINT MipSlice, UINT ArraySlice, UINT NumArraySlices) noexcept
		{
			return { MipSlice, 1, ArraySlice, NumArraySlices };
		}
		static TextureSubresource RTV(UINT MipSlice, UINT ArraySlice, UINT NumArraySlices) noexcept
		{
			return { MipSlice, 1, ArraySlice, NumArraySlices };
		}
		static TextureSubresource DSV(UINT MipSlice, UINT ArraySlice, UINT NumArraySlices) noexcept
		{
			return { MipSlice, 1, ArraySlice, NumArraySlices };
		}
	};

	class D3D12RenderTargetView : public D3D12View<D3D12_RENDER_TARGET_VIEW_DESC, false>
	{
	public:
		D3D12RenderTargetView() noexcept = default;
		explicit D3D12RenderTargetView(
			D3D12LinkedDevice*					 Device,
			const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
			D3D12Resource*						 Resource);
		explicit D3D12RenderTargetView(
			D3D12LinkedDevice*		  Device,
			D3D12Texture*			  Texture,
			bool					  sRGB		  = false,
			const TextureSubresource& Subresource = TextureSubresource{});

		void RecreateView();
	};

	class D3D12DepthStencilView : public D3D12View<D3D12_DEPTH_STENCIL_VIEW_DESC, false>
	{
	public:
		D3D12DepthStencilView() noexcept = default;
		explicit D3D12DepthStencilView(
			D3D12LinkedDevice*					 Device,
			const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
			D3D12Resource*						 Resource);
		explicit D3D12DepthStencilView(
			D3D12LinkedDevice*		  Device,
			D3D12Texture*			  Texture,
			const TextureSubresource& Subresource = TextureSubresource{});

		void RecreateView();
	};

	class D3D12ShaderResourceView : public D3D12View<D3D12_SHADER_RESOURCE_VIEW_DESC, true>
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
			D3D12LinkedDevice*		  Device,
			D3D12Texture*			  Texture,
			bool					  sRGB,
			const TextureSubresource& Subresource = TextureSubresource{});

		void RecreateView();
	};

	class D3D12UnorderedAccessView : public D3D12View<D3D12_UNORDERED_ACCESS_VIEW_DESC, true>
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
			D3D12LinkedDevice*		  Device,
			D3D12Texture*			  Texture,
			const TextureSubresource& Subresource = TextureSubresource{});

		void RecreateView();

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetClearCpuHandle() const noexcept;

	private:
		D3D12Resource* CounterResource = nullptr;
		// CPU descriptor used to clear UAVs
		D3D12Descriptor<D3D12_UNORDERED_ACCESS_VIEW_DESC, false> ClearDescriptor;
	};

	class D3D12Sampler : public D3D12View<D3D12_SAMPLER_DESC, true>
	{
	public:
		D3D12Sampler() noexcept = default;
		explicit D3D12Sampler(
			D3D12LinkedDevice*		  Device,
			const D3D12_SAMPLER_DESC& Desc);
	};
} // namespace RHI
