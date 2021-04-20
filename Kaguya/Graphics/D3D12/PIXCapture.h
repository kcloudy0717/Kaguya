#pragma once
#include <DXProgrammableCapture.h>
#include <pix3.h>

class PIXCapture
{
public:
	PIXCapture();
	~PIXCapture();

private:
	Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> m_GraphicsAnalysis;
};

#ifdef _DEBUG
#define GetScopedCaptureVariableName(a, b) PIXConcatenate(a, b)
#define PIXScopedCapture() PIXCapture GetScopedCaptureVariableName(pixCapture, __LINE__)
#else
#define PIXScopedCapture()
#endif