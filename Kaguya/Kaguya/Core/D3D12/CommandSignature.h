#pragma once
#include "D3D12Common.h"

class CommandSignatureBuilder
{
public:
	D3D12_COMMAND_SIGNATURE_DESC Build() noexcept;

	void AddDraw()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
	}

	void AddDrawIndexed()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	}

	void AddDispatch()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	}

	void AddVertexBufferView(UINT Slot)
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
		Desc.VertexBuffer.Slot			   = Slot;
	}

	void AddIndexBufferView()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	}

	void AddConstant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc	  = Parameters.emplace_back();
		Desc.Type							  = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		Desc.Constant.RootParameterIndex	  = RootParameterIndex;
		Desc.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
		Desc.Constant.Num32BitValuesToSet	  = Num32BitValuesToSet;
	}

	void AddConstantBufferView(UINT RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc		   = Parameters.emplace_back();
		Desc.Type								   = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
		Desc.ConstantBufferView.RootParameterIndex = RootParameterIndex;
	}

	void AddShaderResourceView(UINT RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc		   = Parameters.emplace_back();
		Desc.Type								   = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		Desc.ShaderResourceView.RootParameterIndex = RootParameterIndex;
	}

	void AddUnorderedAccessView(UINT RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc			= Parameters.emplace_back();
		Desc.Type									= D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
		Desc.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
	}

	void AddDispatchRays()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
	}

	void AddDispatchMesh()
	{
		D3D12_INDIRECT_ARGUMENT_DESC& Desc = Parameters.emplace_back();
		Desc.Type						   = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
	}

private:
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> Parameters;
};

class CommandSignature
{
public:
	// The root signature must be specified if and only if the command signature changes one of the root arguments.

	CommandSignature() noexcept = default;

	CommandSignature(
		_In_ ID3D12Device* Device,
		_In_ CommandSignatureBuilder& Builder,
		_In_opt_ ID3D12RootSignature* RootSignature);

	operator ID3D12CommandSignature*() const { return pCommandSignature.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> pCommandSignature;
};
