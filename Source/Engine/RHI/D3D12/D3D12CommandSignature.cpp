#include "D3D12CommandSignature.h"
#include "D3D12Device.h"

D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc::Build() noexcept
{
	// for (const auto& Parameter : Parameters)
	//{
	//	switch (Parameter.Type)
	//	{
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
	//		ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
	//		ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
	//		ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
	//		ByteStride += Parameter.Constant.Num32BitValuesToSet * 4;
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
	//		ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
	//		ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
	//		[[fallthrough]];
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
	//		[[fallthrough]];
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
	//		ByteStride += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS:
	//		ByteStride += sizeof(D3D12_DISPATCH_RAYS_DESC);
	//		break;
	//	case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
	//		ByteStride += sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
	//		break;
	//
	//	default:
	//		assert(false && "Invalid D3D12_INDIRECT_ARGUMENT_TYPE");
	//	}
	//}

	D3D12_COMMAND_SIGNATURE_DESC Desc = {};
	Desc.ByteStride					  = Stride;
	Desc.NumArgumentDescs			  = static_cast<UINT>(Parameters.size());
	Desc.pArgumentDescs				  = Parameters.data();
	Desc.NodeMask					  = 1;
	return Desc;
}

D3D12CommandSignature::D3D12CommandSignature(
	D3D12Device*			 Parent,
	CommandSignatureDesc& Builder,
	ID3D12RootSignature*	 RootSignature)
	: D3D12DeviceChild(Parent)
{
	D3D12_COMMAND_SIGNATURE_DESC Desc = Builder.Build();

	VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateCommandSignature(&Desc, RootSignature, IID_PPV_ARGS(&CommandSignature)));
}
