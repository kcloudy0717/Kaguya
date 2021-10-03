#include "D3D12Utility.h"

PIXCapture::PIXCapture()
{
	if (SUCCEEDED(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(GraphicsAnalysis.ReleaseAndGetAddressOf()))))
	{
		GraphicsAnalysis->BeginCapture();
	}
}

PIXCapture::~PIXCapture()
{
	if (GraphicsAnalysis)
	{
		GraphicsAnalysis->EndCapture();
	}
}
