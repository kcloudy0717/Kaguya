#pragma once
#include "D3D12Core.h"
#include "D3D12LinkedDevice.h"
#include "D3D12Profiler.h"
#include "D3D12PipelineLibrary.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;

	D3D_FEATURE_LEVEL	  FeatureLevel;
	bool				  WaveOperation;
	bool				  Raytracing;
	bool				  DynamicResources;
	bool				  MeshShaders;
	std::filesystem::path CachePath;
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
	[[nodiscard]] auto					GetSizeOfDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept -> UINT { return DescriptorSizeCache[Type]; }
	[[nodiscard]] auto					GetDevice() noexcept -> D3D12LinkedDevice* { return &LinkedDevice; }
	[[nodiscard]] bool					AllowAsyncPsoCompilation() const noexcept;
	[[nodiscard]] ThreadPool*			GetPsoCompilationThreadPool() const noexcept { return PsoCompilationThreadPool.get(); }
	[[nodiscard]] D3D12PipelineLibrary* GetPipelineLibrary() const noexcept { return Library.get(); }

	void OnBeginFrame();
	void OnEndFrame();

	void BeginCapture(const std::filesystem::path& Path)
	{
		PIXCaptureParameters CaptureParameters			= {};
		CaptureParameters.GpuCaptureParameters.FileName = Path.c_str();
		CaptureStatus									= PIXBeginCapture(PIX_CAPTURE_GPU, &CaptureParameters);
	}
	void EndCapture()
	{
		if (SUCCEEDED(CaptureStatus))
		{
			PIXEndCapture(FALSE);
		}
	}

	void WaitIdle();

	[[nodiscard]] std::unique_ptr<D3D12RootSignature> CreateRootSignature(
		RootSignatureDesc& Desc);

	template<typename PipelineStateStream>
	[[nodiscard]] std::unique_ptr<D3D12PipelineState> CreatePipelineState(
		std::wstring		 Name,
		PipelineStateStream& Stream)
	{
		PipelineStateStreamDesc Desc;
		Desc.SizeInBytes				   = sizeof(Stream);
		Desc.pPipelineStateSubobjectStream = &Stream;
		return std::make_unique<D3D12PipelineState>(this, Name, Desc);
	}

	[[nodiscard]] std::unique_ptr<D3D12RaytracingPipelineState> CreateRaytracingPipelineState(
		RaytracingPipelineStateDesc& Desc);

private:
	using TDescriptorSizeCache = std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>;

	static void ReportLiveObjects();
	static void OnDeviceRemoved(PVOID Context, BOOLEAN);

	template<typename T>
	ARC<T> DeviceQueryInterface()
	{
		ARC<T> Interface;
		VERIFY_D3D12_API(Device->QueryInterface(IID_PPV_ARGS(&Interface)));
		return Interface;
	}

	void				  InitializeDxgiObjects(bool Debug);
	ARC<ID3D12Device>	  InitializeDevice(const DeviceOptions& Options);
	CD3DX12FeatureSupport InitializeFeatureSupport(const DeviceOptions& Options);
	TDescriptorSizeCache  InitializeDescriptorSizeCache();

	// Pre SM6.6 bindless root parameter setup
	void AddBindlessParameterToDesc(RootSignatureDesc& Desc);

private:
	struct ReportLiveObjectGuard
	{
		~ReportLiveObjectGuard() { D3D12Device::ReportLiveObjects(); }
	} Guard;

	/*struct WinPix
	{
		WinPix()
		{
			Module = PIXLoadLatestWinPixGpuCapturerLibrary();
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

	ARC<IDXGIFactory6> Factory6;
	ARC<IDXGIAdapter3> Adapter3;
	DXGI_ADAPTER_DESC2 AdapterDesc = {};

	ARC<ID3D12Device>	  Device;
	ARC<ID3D12Device1>	  Device1;
	ARC<ID3D12Device5>	  Device5;
	CD3DX12FeatureSupport FeatureSupport;
	TDescriptorSizeCache  DescriptorSizeCache;

	struct Dred
	{
		Dred(ID3D12Device* Device);
		~Dred();

		ARC<ID3D12Fence>  DeviceRemovedFence;
		HANDLE			  DeviceRemovedWaitHandle = nullptr;
		wil::unique_event DeviceRemovedEvent;
	} Dred;

	// Represents a single node
	// TODO: Add Multi-Adapter support
	D3D12LinkedDevice					  LinkedDevice;
	D3D12Profiler						  Profiler;
	std::unique_ptr<ThreadPool>			  PsoCompilationThreadPool;
	std::unique_ptr<D3D12PipelineLibrary> Library;

	HRESULT CaptureStatus = S_FALSE;
};

inline LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op)
{
#define STRINGIFY(x) \
	case x:          \
		return L#x

	switch (Op)
	{
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETMARKER);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDEVENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCH);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTILES);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PRESENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA);
	}
	return L"<unknown>";
#undef STRINGIFY
}

inline LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type)
{
#define STRINGIFY(x) \
	case x:          \
		return L#x

	switch (Type)
	{
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_FENCE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_RESOURCE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PASS);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_METACOMMAND);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_INVALID);
	}
	return L"<unknown>";
#undef STRINGIFY
}
