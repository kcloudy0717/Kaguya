#include "pch.h"
#include "Device.h"

#define GPU_BASED_VALIDATION 0

using Microsoft::WRL::ComPtr;
using namespace D3D12MA;

void Device::ReportLiveObjects()
{
#ifdef _DEBUG
	ComPtr<IDXGIDebug> pDXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(pDXGIDebug.ReleaseAndGetAddressOf()))))
	{
		pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
#endif
}

void Device::Create(IDXGIAdapter4* pAdapter)
{
	/*
	* https://walbourn.github.io/anatomy-of-direct3d-12-create-device/
	* ID3D12Device1					Windows 10 Anniversary Update	(14393)
	* ID3D12Device2					Windows 10 Creators Update		(15063)
	* ID3D12Device3					Windows 10 Fall Creators Update (16299)
	* ID3D12Device4					Windows 10 April 2018 Update	(17134)
	* ID3D12Device5					Windows 10 October 2018			(17763)
	* ID3D12Device6					Windows 10 May 2019				(18362)
	* ID3D12Device7, ID3D12Device8	Windows 10 May 2020 Update		(19041)
	*/
#ifdef _DEBUG
	// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the device.
	ComPtr<ID3D12Debug1> pDebug1;
	ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(pDebug1.ReleaseAndGetAddressOf())));
	pDebug1->EnableDebugLayer();
#if GPU_BASED_VALIDATION
	pDebug1->SetEnableGPUBasedValidation(TRUE);
#endif
#endif

	constexpr D3D_FEATURE_LEVEL MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	// Create our virtual device used for interacting with the GPU so we can create resources
	ThrowIfFailed(::D3D12CreateDevice(pAdapter, MinimumFeatureLevel, IID_PPV_ARGS(m_Device5.ReleaseAndGetAddressOf())));

	// Check for different features
	CheckRootSignature_1_1Support();
	CheckShaderModel6PlusSupport();
	CheckRaytracingSupport();
	//CheckMeshShaderSupport();

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_Device5.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
#endif

	// This class is used to manage video memory allocations for constants, dynamic vertex buffers, dynamic index buffers, etc.
	m_GraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(m_Device5.Get());

	// Create our memory allocator
	ALLOCATOR_DESC desc = {};
	desc.Flags = ALLOCATOR_FLAG_NONE;
	desc.pDevice = m_Device5.Get();
	desc.PreferredBlockSize = 0;
	desc.pAllocationCallbacks = nullptr;
	desc.pAdapter = pAdapter;
	ThrowIfFailed(CreateAllocator(&desc, &m_Allocator));
}

Device::~Device()
{
	SafeRelease(m_Allocator);
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