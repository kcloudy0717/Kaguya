#pragma once
#include "D3D12Core.h"
#include "D3D12LinkedDevice.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"

namespace RHI
{
	struct DeviceOptions
	{
		bool EnableDebugLayer;
		bool EnableGpuBasedValidation;
		bool EnableAutoDebugName;

		D3D_FEATURE_LEVEL FeatureLevel;
		bool			  WaveIntrinsics;
		bool			  Raytracing;
		bool			  DynamicResources;
		bool			  MeshShaders;
	};

	struct RootParameters
	{
		struct DescriptorTable
		{
			enum
			{
				ShaderResourceDescriptorTable,
				UnorderedAccessDescriptorTable,
				SamplerDescriptorTable,
				NumRootParameters
			};
		};
	};

	class D3D12Device
	{
	public:
		explicit D3D12Device(const DeviceOptions& Options);

		void CreateDxgiFactory();

		[[nodiscard]] IDXGIFactory6*	 GetDxgiFactory6() const noexcept { return Factory6.Get(); }
		[[nodiscard]] IDXGIAdapter3*	 GetDxgiAdapter3() const noexcept { return Adapter3.Get(); }
		[[nodiscard]] ID3D12Device*		 GetD3D12Device() const noexcept { return Device.Get(); }
		[[nodiscard]] ID3D12Device1*	 GetD3D12Device1() const noexcept { return Device1.Get(); }
		[[nodiscard]] ID3D12Device5*	 GetD3D12Device5() const noexcept { return Device5.Get(); }
		[[nodiscard]] IDStorageFactory*	 GetDStorageFactory() const noexcept { return DStorageFactory.Get(); }
		[[nodiscard]] IDStorageQueue*	 GetDStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE Type) const noexcept { return DStorageQueues[Type].Get(); }
		[[nodiscard]] D3D12Fence*		 GetDStorageFence() noexcept { return &DStorageFence; }
		[[nodiscard]] D3D12NodeMask		 GetAllNodeMask() const noexcept { return AllNodeMask; }
		[[nodiscard]] UINT				 GetSizeOfDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept { return DescriptorSizeCache[Type]; }
		[[nodiscard]] D3D12LinkedDevice* GetLinkedDevice() noexcept { return &LinkedDevice; }
		[[nodiscard]] bool				 AllowAsyncPsoCompilation() const noexcept;

		[[nodiscard]] bool SupportsWaveIntrinsics() const noexcept;
		[[nodiscard]] bool SupportsRaytracing() const noexcept;
		[[nodiscard]] bool SupportsDynamicResources() const noexcept;
		[[nodiscard]] bool SupportsMeshShaders() const noexcept;

		void OnBeginFrame();
		void OnEndFrame();

		void BeginCapture(const std::filesystem::path& Path) const;
		void EndCapture() const;

		void WaitIdle();

		[[nodiscard]] D3D12SyncHandle DStorageSubmit(DSTORAGE_REQUEST_SOURCE_TYPE Type);

		[[nodiscard]] D3D12RootSignature CreateRootSignature(RootSignatureDesc& Desc);

		template<typename PipelineStateStream>
		[[nodiscard]] D3D12PipelineState CreatePipelineState(std::wstring Name, PipelineStateStream& Stream)
		{
			PipelineStateStreamDesc Desc;
			Desc.SizeInBytes				   = sizeof(Stream);
			Desc.pPipelineStateSubobjectStream = &Stream;
			return D3D12PipelineState(this, Name, Desc);
		}

		[[nodiscard]] D3D12RaytracingPipelineState CreateRaytracingPipelineState(RaytracingPipelineStateDesc& Desc);

	private:
		using TDescriptorSizeCache = std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>;

		static void ReportLiveObjects();
		static void OnDeviceRemoved(PVOID Context, BOOLEAN);

		template<typename T>
		Arc<T> DeviceQueryInterface()
		{
			Arc<T> Interface;
			VERIFY_D3D12_API(Device->QueryInterface(IID_PPV_ARGS(&Interface)));
			return Interface;
		}

		void				  InternalCreateDxgiFactory(bool Debug);
		Arc<ID3D12Device>	  InitializeDevice(const DeviceOptions& Options);
		CD3DX12FeatureSupport InitializeFeatureSupport(const DeviceOptions& Options);
		TDescriptorSizeCache  InitializeDescriptorSizeCache();

		// Dred debug names
		static LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op);
		static LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type);

	private:
		struct ReportLiveObjectGuard
		{
			~ReportLiveObjectGuard() { D3D12Device::ReportLiveObjects(); }
		} MemoryGuard;

		/*struct WinPix
		{
			WinPix()
				: Module(PIXLoadLatestWinPixGpuCapturerLibrary())
			{
			}
			~WinPix()
			{
				if (Module)
				{
					FreeLibrary(Module);
				}
			}

			HMODULE Module;
		} WinPix;*/

		Arc<IDXGIFactory6> Factory6;
		Arc<IDXGIAdapter3> Adapter3;
		DXGI_ADAPTER_DESC2 AdapterDesc = {};

		Arc<ID3D12Device>  Device;
		Arc<ID3D12Device1> Device1;
		Arc<ID3D12Device5> Device5;

		Arc<IDStorageFactory> DStorageFactory;
		Arc<IDStorageQueue>	  DStorageQueues[2];
		D3D12Fence			  DStorageFence;

		D3D12NodeMask		  AllNodeMask;
		CD3DX12FeatureSupport FeatureSupport;
		TDescriptorSizeCache  DescriptorSizeCache;

		struct Dred
		{
			Dred(ID3D12Device* Device);
			~Dred();

			Arc<ID3D12Fence> DeviceRemovedFence;
			HANDLE			 DeviceRemovedWaitHandle;
			Event			 DeviceRemovedEvent;
		} Dred;

		// Represents a single node
		// TODO: Add Multi-Adapter support
		D3D12LinkedDevice LinkedDevice;

		mutable HRESULT CaptureStatus = S_FALSE;
	};
} // namespace RHI
