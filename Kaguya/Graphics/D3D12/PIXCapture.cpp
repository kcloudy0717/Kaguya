#include "pch.h"
#include "PIXCapture.h"

PIXCapture::PIXCapture()
{
	if (SUCCEEDED(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_GraphicsAnalysis))))
	{
		m_GraphicsAnalysis->BeginCapture();
	}
}

PIXCapture::~PIXCapture()
{
	if (m_GraphicsAnalysis)
	{
		m_GraphicsAnalysis->EndCapture();
	}
}