#include "pch.h"
#include "SwapChain.h"
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

SwapChain::SwapChain(HWND hWnd, IDXGIFactory5* pFactory5, ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue)
	: m_pDevice(pDevice)
{
	// Check tearing support
	BOOL allowTearing = FALSE;
	if (FAILED(pFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
	{
		allowTearing = FALSE;
	}
	m_TearingSupport = allowTearing == TRUE;

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width				   = 0;
	desc.Height				   = 0;
	desc.Format				   = Format;
	desc.Stereo				   = FALSE;
	desc.SampleDesc			   = DefaultSampleDesc();
	desc.BufferUsage		   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount		   = BackBufferCount;
	desc.Scaling			   = DXGI_SCALING_NONE;
	desc.SwapEffect			   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode			   = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = m_TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	;

	ComPtr<IDXGISwapChain1> pSwapChain1;
	ThrowIfFailed(pFactory5->CreateSwapChainForHwnd(
		pCommandQueue,
		hWnd,
		&desc,
		nullptr,
		nullptr,
		pSwapChain1.ReleaseAndGetAddressOf()));
	ThrowIfFailed(pFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)); // No full screen via alt + enter
	pSwapChain1.As(&m_SwapChain);

	// Create descriptor heap
	m_RTVHeaps = DescriptorHeap(pDevice, BackBufferCount, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Initialize RTVs
	for (UINT i = 0; i < BackBufferCount; i++)
	{
		m_RenderTargetViews[i] = m_RTVHeaps.At(i);
	}

	CreateRenderTargetViews();
}

ID3D12Resource* SwapChain::GetBackBuffer(UINT Index) const
{
	ID3D12Resource* pBackBuffer = nullptr;
	ThrowIfFailed(m_SwapChain->GetBuffer(Index, IID_PPV_ARGS(&pBackBuffer)));
	pBackBuffer->Release();
	return pBackBuffer;
}

std::pair<ID3D12Resource*, Descriptor> SwapChain::GetCurrentBackBufferResource() const
{
	UINT			backBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	ID3D12Resource* pBackBuffer		= GetBackBuffer(backBufferIndex);
	return { pBackBuffer, m_RenderTargetViews[backBufferIndex] };
}

void SwapChain::Resize(UINT Width, UINT Height)
{
	// Resize backbuffer
	// Note: Cannot use ResizeBuffers1 when debugging in Nsight Graphics, it will crash
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	ThrowIfFailed(m_SwapChain->GetDesc1(&desc));
	ThrowIfFailed(m_SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, desc.Flags));

	CreateRenderTargetViews();
}

void SwapChain::Present(bool VSync)
{
	const UINT syncInterval = VSync ? 1u : 0u;
	const UINT presentFlags = (m_TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
	HRESULT	   hr			= m_SwapChain->Present(syncInterval, presentFlags);
	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		// TODO: Handle device removal
	}
}

void SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < BackBufferCount; ++i)
	{
		ComPtr<ID3D12Resource> pBackBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)));

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format						  = Format;
		rtvDesc.ViewDimension				  = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice			  = 0;
		rtvDesc.Texture2D.PlaneSlice		  = 0;

		m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_RenderTargetViews[i].CpuHandle);
	}
}
