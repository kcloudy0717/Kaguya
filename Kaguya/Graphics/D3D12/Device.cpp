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

	// This class is used to manage video memory allocations for constants, dynamic vertex buffers, dynamic index buffers, etc.
	m_GraphicsMemory = std::make_unique<DirectX::GraphicsMemory>(m_Device.Get());
}

Device::~Device()
{
	SafeRelease(m_Allocator);
}

void Device::CreateAllocator(
	_In_ IDXGIAdapter4* pAdapter)
{
	// Create our memory allocator
	D3D12MA::ALLOCATOR_DESC desc = {};
	desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
	desc.pDevice = m_Device.Get();
	desc.PreferredBlockSize = 0;
	desc.pAllocationCallbacks = nullptr;
	desc.pAdapter = pAdapter;
	ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &m_Allocator));
}
