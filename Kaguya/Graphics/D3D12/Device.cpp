#include "pch.h"
#include "Device.h"

using Microsoft::WRL::ComPtr;

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

void Device::ReportLiveObjects()
{
	ComPtr<IDXGIDebug> pDXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDXGIDebug))))
	{
		pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
}

Device::Device(_In_ IDXGIAdapter4* pAdapter, _In_ const DeviceOptions& Options, _In_ const DeviceFeatures& Features)
{
	// Enable the D3D12 debug layer
	if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
	{
		// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
		// device.
		ComPtr<ID3D12Debug5> pDebug5;
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(pDebug5.ReleaseAndGetAddressOf()))))
		{
			if (Options.EnableDebugLayer)
			{
				pDebug5->EnableDebugLayer();
			}
			pDebug5->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
			pDebug5->SetEnableAutoName(Options.EnableAutoDebugName);
		}
	}

	ThrowIfFailed(::D3D12CreateDevice(pAdapter, Options.FeatureLevel, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())));

	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_Device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, Options.BreakOnCorruption);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, Options.BreakOnError);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, Options.BreakOnWarning);
	}

	if (Features.WaveOperation)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS1 o1 = {};
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &o1, sizeof(o1))))
		{
			if (!o1.WaveOps)
			{
				throw std::runtime_error("Wave operation not supported on device");
			}
		}
	}

	if (Features.Raytracing)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5 = {};
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5))))
		{
			if (o5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
			{
				throw std::runtime_error("Raytracing not supported on device");
			}
		}
	}

	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
	if (Features.DynamicResources)
	{
		D3D12_FEATURE_DATA_SHADER_MODEL sm = { .HighestShaderModel = D3D_SHADER_MODEL_6_6 };
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))))
		{
			if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_6)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS o0 = {};
		if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &o0, sizeof(o0))))
		{
			if (o0.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}
	}
}

Device::~Device()
{
}
