#include "D3D12SwapChain.h"

using Microsoft::WRL::ComPtr;

D3D12SwapChain::D3D12SwapChain(
	HWND				hWnd,
	IDXGIFactory5*		Factory5,
	ID3D12Device*		Device,
	ID3D12CommandQueue* CommandQueue)
	: Device(Device)
{
	// Check tearing support
	BOOL AllowTearing = FALSE;
	if (FAILED(Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing))))
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
	VERIFY_D3D12_API(Factory5->CreateSwapChainForHwnd(CommandQueue, hWnd, &Desc, nullptr, nullptr, &SwapChain1));
	VERIFY_D3D12_API(Factory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)); // No full screen via alt + enter
	SwapChain1.As(&SwapChain4);

	// Create Descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = { .Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
											.NumDescriptors = BackBufferCount,
											.Flags			= D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
											.NodeMask		= 0 };
	VERIFY_D3D12_API(Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(RTVHeaps.ReleaseAndGetAddressOf())));
	UINT RTVSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Initialize RTVs
	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuBaseAddress(RTVHeaps->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < BackBufferCount; i++)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(RenderTargetViews[i], CpuBaseAddress, i, RTVSize);
	}

	CreateRenderTargetViews();
}

ID3D12Resource* D3D12SwapChain::GetBackBuffer(UINT Index) const
{
	ID3D12Resource* pBackBuffer = nullptr;
	VERIFY_D3D12_API(SwapChain4->GetBuffer(Index, IID_PPV_ARGS(&pBackBuffer)));
	pBackBuffer->Release();
	return pBackBuffer;
}

std::pair<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> D3D12SwapChain::GetCurrentBackBufferResource() const
{
	UINT			BackBufferIndex = SwapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* pBackBuffer		= GetBackBuffer(BackBufferIndex);
	return { pBackBuffer, RenderTargetViews[BackBufferIndex] };
}

void D3D12SwapChain::Resize(UINT Width, UINT Height)
{
	// Resize backbuffer
	// Note: Cannot use ResizeBuffers1 when debugging in Nsight Graphics, it will crash
	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	VERIFY_D3D12_API(SwapChain4->GetDesc1(&Desc));
	VERIFY_D3D12_API(SwapChain4->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Desc.Flags));

	CreateRenderTargetViews();
}

void D3D12SwapChain::Present(bool VSync)
{
	try
	{
		UINT	SyncInterval = VSync ? 1u : 0u;
		UINT	PresentFlags = (TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
		HRESULT Result		 = SwapChain4->Present(SyncInterval, PresentFlags);
		if (Result == DXGI_ERROR_DEVICE_REMOVED)
		{
			// TODO: Handle device removal
			throw DXGI_ERROR_DEVICE_REMOVED;
		}
	}
	catch (HRESULT Result)
	{
		(void)Result;
	}
}

void D3D12SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < BackBufferCount; ++i)
	{
		ComPtr<ID3D12Resource> BackBuffer;
		VERIFY_D3D12_API(SwapChain4->GetBuffer(i, IID_PPV_ARGS(&BackBuffer)));

		D3D12_RENDER_TARGET_VIEW_DESC ViewDesc = {};
		ViewDesc.Format						   = Format;
		ViewDesc.ViewDimension				   = D3D12_RTV_DIMENSION_TEXTURE2D;
		ViewDesc.Texture2D.MipSlice			   = 0;
		ViewDesc.Texture2D.PlaneSlice		   = 0;

		Device->CreateRenderTargetView(BackBuffer.Get(), &ViewDesc, RenderTargetViews[i]);
	}
}
