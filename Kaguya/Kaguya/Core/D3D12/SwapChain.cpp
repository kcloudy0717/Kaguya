#include "SwapChain.h"
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

SwapChain::SwapChain(HWND hWnd, IDXGIFactory5* pFactory5, ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue)
	: pDevice(pDevice)
{
	// Check tearing support
	BOOL AllowTearing = FALSE;
	if (FAILED(pFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing))))
	{
		AllowTearing = FALSE;
	}
	TearingSupport = AllowTearing == TRUE;

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	Desc.Width				   = 0;
	Desc.Height				   = 0;
	Desc.Format				   = Format;
	Desc.Stereo				   = FALSE;
	Desc.SampleDesc			   = DefaultSampleDesc();
	Desc.BufferUsage		   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	Desc.BufferCount		   = BackBufferCount;
	Desc.Scaling			   = DXGI_SCALING_NONE;
	Desc.SwapEffect			   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	Desc.AlphaMode			   = DXGI_ALPHA_MODE_UNSPECIFIED;
	Desc.Flags = TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain1> SwapChain1;
	VERIFY_D3D12_API(pFactory5->CreateSwapChainForHwnd(
		pCommandQueue,
		hWnd,
		&Desc,
		nullptr,
		nullptr,
		SwapChain1.ReleaseAndGetAddressOf()));
	VERIFY_D3D12_API(pFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)); // No full screen via alt + enter
	SwapChain1.As(&SwapChain4);

	// Create Descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = { .Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
											.NumDescriptors = BackBufferCount,
											.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
											.NodeMask		= 0 };
	VERIFY_D3D12_API(pDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(RTVHeaps.ReleaseAndGetAddressOf())));
	UINT RTVViewStride = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Initialize RTVs
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCPU(RTVHeaps->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < BackBufferCount; i++)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(RenderTargetViews[i], hCPU, i, RTVViewStride);
	}

	CreateRenderTargetViews();
}

ID3D12Resource* SwapChain::GetBackBuffer(UINT Index) const
{
	ID3D12Resource* pBackBuffer = nullptr;
	VERIFY_D3D12_API(SwapChain4->GetBuffer(Index, IID_PPV_ARGS(&pBackBuffer)));
	pBackBuffer->Release();
	return pBackBuffer;
}

std::pair<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> SwapChain::GetCurrentBackBufferResource() const
{
	UINT			BackBufferIndex = SwapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pBackBuffer		= GetBackBuffer(BackBufferIndex);
	return { pBackBuffer, RenderTargetViews[BackBufferIndex] };
}

void SwapChain::Resize(UINT Width, UINT Height)
{
	// Resize backbuffer
	// Note: Cannot use ResizeBuffers1 when debugging in Nsight Graphics, it will crash
	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	VERIFY_D3D12_API(SwapChain4->GetDesc1(&Desc));
	VERIFY_D3D12_API(SwapChain4->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Desc.Flags));

	CreateRenderTargetViews();
}

void SwapChain::Present(bool VSync)
{
	try
	{
		const UINT SyncInterval = VSync ? 1u : 0u;
		const UINT PresentFlags = (TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
		HRESULT	   hr			= SwapChain4->Present(SyncInterval, PresentFlags);
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			// TODO: Handle device removal
			throw hr;
		}
	}
	catch (HRESULT hr)
	{
		HRESULT errorCode = hr;
	}
}

void SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < BackBufferCount; ++i)
	{
		ComPtr<ID3D12Resource> pBackBuffer;
		VERIFY_D3D12_API(SwapChain4->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)));

		D3D12_RENDER_TARGET_VIEW_DESC ViewDesc = {};
		ViewDesc.Format						   = Format;
		ViewDesc.ViewDimension				   = D3D12_RTV_DIMENSION_TEXTURE2D;
		ViewDesc.Texture2D.MipSlice			   = 0;
		ViewDesc.Texture2D.PlaneSlice		   = 0;

		pDevice->CreateRenderTargetView(pBackBuffer.Get(), &ViewDesc, RenderTargetViews[i]);
	}
}
