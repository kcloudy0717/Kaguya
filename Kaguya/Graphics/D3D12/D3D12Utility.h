#pragma once
#include "d3dx12.h"
#include <DXProgrammableCapture.h>
#include <pix3.h>

class PIXCapture
{
public:
	PIXCapture();
	~PIXCapture();

private:
	Microsoft::WRL::ComPtr<IDXGraphicsAnalysis> GraphicsAnalysis;
};

#ifdef _DEBUG
#define GetScopedCaptureVariableName(a, b) PIXConcatenate(a, b)
#define PIXScopedCapture()				   PIXCapture GetScopedCaptureVariableName(pixCapture, __LINE__)
#else
#define PIXScopedCapture()
#endif

struct ShaderIdentifier
{
	ShaderIdentifier() noexcept = default;

	ShaderIdentifier(void* Data) { std::memcpy(this->Data, Data, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES); }

	BYTE Data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

// Represents a Fence and Value pair, similar to that of a coroutine handle
// you can query the status of a command execution point and wait for it
class CommandSyncPoint
{
public:
	CommandSyncPoint()
		: Fence(nullptr)
		, Value(0)
	{
	}
	CommandSyncPoint(ID3D12Fence* Fence, UINT64 Value)
		: Fence(Fence)
		, Value(Value)
	{
	}

	bool   IsValid() const;
	UINT64 GetValue() const;
	bool   IsComplete() const;
	void   WaitForCompletion() const;

private:
	friend class CommandQueue;

	ID3D12Fence* Fence;
	UINT64		 Value;
};

inline DXGI_FORMAT GetValidDepthStencilViewFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		return Format;
	};
}

inline DXGI_FORMAT GetValidSRVFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	default:
		return Format;
	}
};

class InputLayout
{
public:
	void AddVertexLayoutElement(
		std::string_view SemanticName,
		UINT			 SemanticIndex,
		DXGI_FORMAT		 Format,
		UINT			 InputSlot,
		UINT			 AlignedByteOffset);

	void AddInstanceLayoutElement(
		std::string_view SemanticName,
		UINT			 SemanticIndex,
		DXGI_FORMAT		 Format,
		UINT			 InputSlot,
		UINT			 AlignedByteOffset,
		UINT			 InstanceDataStepRate);

	operator D3D12_INPUT_LAYOUT_DESC() const noexcept;

private:
	std::vector<std::string>					  SemanticNames;
	mutable std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements; // must be mutable, as operator() resolves SemanticName
};
