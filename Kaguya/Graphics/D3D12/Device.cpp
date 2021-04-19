#include "pch.h"
#include "Device.h"

using Microsoft::WRL::ComPtr;

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
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug1))))
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
		IID_PPV_ARGS(&m_Device5)));

	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_Device5.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, Options.BreakOnCorruption);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, Options.BreakOnError);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, Options.BreakOnWarning);
	}
}

void Device::CheckRootSignature_1_1Support()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE RS = {};
	RS.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_Device5->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &RS, sizeof(RS))))
	{
		throw std::runtime_error("CheckFeatureSupport::D3D12_FEATURE_ROOT_SIGNATURE Failed");
	}

	if (RS.HighestVersion < D3D_ROOT_SIGNATURE_VERSION_1_1)
	{
		throw std::runtime_error("RS 1.1 not supported on device");
	}
}

void Device::CheckShaderModel6PlusSupport()
{
	D3D12_FEATURE_DATA_SHADER_MODEL SM = { D3D_SHADER_MODEL_6_5 };
	if (FAILED(m_Device5->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &SM, sizeof(SM))))
	{
		throw std::runtime_error("CheckFeatureSupport::D3D12_FEATURE_SHADER_MODEL Failed");
	}

	if (SM.HighestShaderModel < D3D_SHADER_MODEL_6_0)
	{
		throw std::runtime_error("Shader Model 6+ not supported on device");
	}
}

void Device::CheckRaytracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 O5 = {};
	if (FAILED(m_Device5->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &O5, sizeof(O5))))
	{
		throw std::runtime_error("CheckFeatureSupport::D3D12_FEATURE_D3D12_OPTIONS5 Failed");
	}

	if (O5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		throw std::runtime_error("Raytracing not supported on device");
	}
}

void Device::CheckMeshShaderSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 O7 = {};
	if (FAILED(m_Device5->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &O7, sizeof(O7))))
	{
		throw std::runtime_error("CheckFeatureSupport::D3D12_FEATURE_D3D12_OPTIONS7 Failed");
	}

	if (O7.MeshShaderTier < D3D12_MESH_SHADER_TIER_1)
	{
		throw std::runtime_error("Mesh shader not supported on device");
	}
}
