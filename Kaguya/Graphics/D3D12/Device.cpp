#include "pch.h"
#include "Device.h"

using Microsoft::WRL::ComPtr;

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

void Device::ReportLiveObjects()
{
	ComPtr<IDXGIDebug> pDXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDXGIDebug))))
	{
		pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
}

Device::Device(
	_In_ IDXGIAdapter4* pAdapter,
	_In_ const DeviceOptions& Options)
{
	// Enable the D3D12 debug layer
	if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
	{
		// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the device.
		ComPtr<ID3D12Debug1> pDebug1;
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(pDebug1.ReleaseAndGetAddressOf()))))
		{
			if (Options.EnableDebugLayer)
			{
				pDebug1->EnableDebugLayer();
			}
			pDebug1->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
		}
	}

	ThrowIfFailed(::D3D12CreateDevice(
		pAdapter,
		Options.FeatureLevel,
		IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())));

	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_Device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, Options.BreakOnCorruption);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, Options.BreakOnError);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, Options.BreakOnWarning);
	}

	D3D12_FEATURE_DATA_SHADER_MODEL sm = { .HighestShaderModel = D3D_SHADER_MODEL_6_6 };
	if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))))
	{
		if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
		{
			throw std::runtime_error("Shader Model 6_6 not supported");
		}
	}
}

Device::~Device()
{

}
