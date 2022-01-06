#include "D3D12SwapChain.h"

using Microsoft::WRL::ComPtr;

D3D12SwapChain::D3D12SwapChain(
	D3D12Device* Parent,
	HWND		 HWnd)
	: D3D12DeviceChild(Parent)
	, WindowHandle(HWnd)
	, SwapChain4(InitializeSwapChain())
	, RenderTargetViews(Parent->GetDevice()->GetRtvAllocator().Allocate(BackBufferCount))
	, Fence(Parent, 0, D3D12_FENCE_FLAG_NONE)
{
	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	SwapChain4->GetDesc1(&Desc);
	Resize(Desc.Width, Desc.Height);
}

D3D12SwapChain::~D3D12SwapChain()
{
	if (SyncHandle)
	{
		SyncHandle.WaitForCompletion();
	}
}

ID3D12Resource* D3D12SwapChain::GetBackBuffer(UINT Index) const
{
	return BackBuffers[Index].Get();
}

D3D12SwapChainResource D3D12SwapChain::GetCurrentBackBufferResource() const
{
	UINT			BackBufferIndex = SwapChain4->GetCurrentBackBufferIndex();
	ID3D12Resource* BackBuffer		= GetBackBuffer(BackBufferIndex);
	return { BackBuffer, RenderTargetViews[BackBufferIndex] };
}

D3D12_VIEWPORT D3D12SwapChain::GetViewport() const noexcept
{
	return CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<FLOAT>(Width), static_cast<FLOAT>(Height));
}

D3D12_RECT D3D12SwapChain::GetScissorRect() const noexcept
{
	return CD3DX12_RECT(0, 0, Width, Height);
}

void D3D12SwapChain::Resize(UINT Width, UINT Height)
{
	if (SyncHandle)
	{
		SyncHandle.WaitForCompletion();
		SyncHandle = nullptr;
	}

	for (auto& BackBuffer : BackBuffers)
	{
		BackBuffer.Reset();
	}

	if (this->Width != Width || this->Height != Height)
	{
		this->Width	 = Width;
		this->Height = Height;

		assert(Width > 0 && Height > 0);
	}

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	VERIFY_D3D12_API(SwapChain4->GetDesc1(&Desc));
	VERIFY_D3D12_API(SwapChain4->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Desc.Flags));

	for (UINT i = 0; i < BackBufferCount; ++i)
	{
		VERIFY_D3D12_API(SwapChain4->GetBuffer(i, IID_PPV_ARGS(BackBuffers[i].ReleaseAndGetAddressOf())));
	}

	CreateRenderTargetViews();
}

void D3D12SwapChain::Present(bool VSync, IPresent& Present)
{
	Present.PrePresent();
	{
		UINT	SyncInterval = VSync ? 1u : 0u;
		UINT	PresentFlags = (TearingSupport && !VSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;
		HRESULT Result		 = SwapChain4->Present(SyncInterval, PresentFlags);
	}
	Present.PostPresent();

	UINT64 ValueToWaitFor = Fence.Signal(GetParentDevice()->GetDevice()->GetGraphicsQueue());
	SyncHandle			  = D3D12SyncHandle(&Fence, ValueToWaitFor);
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> D3D12SwapChain::InitializeSwapChain()
{
	IDXGIFactory6*		Factory		 = GetParentDevice()->GetDxgiFactory6();
	ID3D12CommandQueue* CommandQueue = GetParentDevice()->GetDevice()->GetGraphicsQueue()->GetCommandQueue();

	// Check tearing support
	BOOL AllowTearing = FALSE;
	if (FAILED(Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing))))
	{
		AllowTearing = FALSE;
	}
	TearingSupport = AllowTearing == TRUE;

	// Width/Height can be set to 0
	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	Desc.Format				   = Format;
	Desc.Stereo				   = FALSE;
	Desc.SampleDesc			   = DefaultSampleDesc();
	Desc.BufferUsage		   = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	Desc.BufferCount		   = BackBufferCount;
	Desc.Scaling			   = DXGI_SCALING_NONE;
	Desc.SwapEffect			   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	Desc.AlphaMode			   = DXGI_ALPHA_MODE_UNSPECIFIED;
	Desc.Flags				   = TearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain1;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;
	VERIFY_D3D12_API(Factory->CreateSwapChainForHwnd(CommandQueue, WindowHandle, &Desc, nullptr, nullptr, SwapChain1.GetAddressOf()));
	// No full screen via alt + enter
	VERIFY_D3D12_API(Factory->MakeWindowAssociation(WindowHandle, DXGI_MWA_NO_ALT_ENTER));
	VERIFY_D3D12_API(SwapChain1->QueryInterface(IID_PPV_ARGS(SwapChain4.ReleaseAndGetAddressOf())));

	return SwapChain4;
}

void D3D12SwapChain::CreateRenderTargetViews()
{
	for (UINT i = 0; i < BackBufferCount; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC ViewDesc = {};
		ViewDesc.Format						   = Format;
		ViewDesc.ViewDimension				   = D3D12_RTV_DIMENSION_TEXTURE2D;
		ViewDesc.Texture2D.MipSlice			   = 0;
		ViewDesc.Texture2D.PlaneSlice		   = 0;

		GetParentDevice()->GetD3D12Device()->CreateRenderTargetView(
			BackBuffers[i].Get(),
			&ViewDesc,
			RenderTargetViews[i]);
	}
}
