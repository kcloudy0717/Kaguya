#pragma once
#include "D3D12Core.h"
#include "D3D12LinkedDevice.h"
#include "D3D12PipelineLibrary.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"

namespace RHI
{
	struct DeviceOptions
	{
		bool EnableDebugLayer;
		bool EnableGpuBasedValidation;
		bool EnableAutoDebugName;

		D3D_FEATURE_LEVEL	  FeatureLevel;
		bool				  WaveIntrinsics;
		bool				  Raytracing;
		bool				  DynamicResources;
		bool				  MeshShaders;
		std::filesystem::path PsoCachePath;
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

		void CreateDxgiFactory(bool Debug);

		[[nodiscard]] auto					GetDxgiFactory6() const noexcept -> IDXGIFactory6* { return Factory6.Get(); }
		[[nodiscard]] auto					GetDxgiAdapter3() const noexcept -> IDXGIAdapter3* { return Adapter3.Get(); }
		[[nodiscard]] auto					GetD3D12Device() const noexcept -> ID3D12Device* { return Device.Get(); }
		[[nodiscard]] auto					GetD3D12Device1() const noexcept -> ID3D12Device1* { return Device1.Get(); }
		[[nodiscard]] auto					GetD3D12Device5() const noexcept -> ID3D12Device5* { return Device5.Get(); }
		[[nodiscard]] auto					GetAllNodeMask() const noexcept -> D3D12NodeMask { return AllNodeMask; }
		[[nodiscard]] auto					GetSizeOfDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept -> UINT { return DescriptorSizeCache[Type]; }
		[[nodiscard]] auto					GetDevice() noexcept -> D3D12LinkedDevice* { return &LinkedDevice; }
		[[nodiscard]] bool					AllowAsyncPsoCompilation() const noexcept;
		[[nodiscard]] ThreadPool*			GetPsoCompilationThreadPool() const noexcept { return PsoCompilationThreadPool.get(); }
		[[nodiscard]] D3D12PipelineLibrary* GetPipelineLibrary() const noexcept { return Library.get(); }

		[[nodiscard]] bool SupportsWaveIntrinsics() const noexcept;
		[[nodiscard]] bool SupportsRaytracing() const noexcept;
		[[nodiscard]] bool SupportsDynamicResources() const noexcept;
		[[nodiscard]] bool SupportsMeshShaders() const noexcept;

		void OnBeginFrame();
		void OnEndFrame();

		void BeginCapture(const std::filesystem::path& Path) const;
		void EndCapture() const;

		void WaitIdle();

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

		void				  InitializeDxgiObjects(bool Debug);
		Arc<ID3D12Device>	  InitializeDevice(const DeviceOptions& Options);
		CD3DX12FeatureSupport InitializeFeatureSupport(const DeviceOptions& Options);
		TDescriptorSizeCache  InitializeDescriptorSizeCache();

		// Pre SM6.6 bindless root parameter setup
		void AddBindlessParameterToDesc(RootSignatureDesc& Desc);

	private:
		struct ReportLiveObjectGuard
		{
			~ReportLiveObjectGuard() { D3D12Device::ReportLiveObjects(); }
		} MemoryGuard;

		// struct WinPix
		//{
		//	WinPix()
		//	{
		//		Module = PIXLoadLatestWinPixGpuCapturerLibrary();
		//	}
		//	~WinPix()
		//	{
		//		if (Module)
		//		{
		//			FreeLibrary(Module);
		//		}
		//	}

		//	HMODULE Module;
		//} WinPix;

		Arc<IDXGIFactory6> Factory6;
		Arc<IDXGIAdapter3> Adapter3;
		DXGI_ADAPTER_DESC2 AdapterDesc = {};

		Arc<ID3D12Device>	  Device;
		Arc<ID3D12Device1>	  Device1;
		Arc<ID3D12Device5>	  Device5;
		D3D12NodeMask		  AllNodeMask;
		CD3DX12FeatureSupport FeatureSupport;
		TDescriptorSizeCache  DescriptorSizeCache;

		struct Dred
		{
			Dred(ID3D12Device* Device);
			~Dred();

			Arc<ID3D12Fence>  DeviceRemovedFence;
			HANDLE			  DeviceRemovedWaitHandle;
			wil::unique_event DeviceRemovedEvent;
		} Dred;

		// Represents a single node
		// TODO: Add Multi-Adapter support
		D3D12LinkedDevice					  LinkedDevice;
		std::unique_ptr<ThreadPool>			  PsoCompilationThreadPool;
		std::unique_ptr<D3D12PipelineLibrary> Library;

		mutable HRESULT CaptureStatus = S_FALSE;
	};
} // namespace RHI
