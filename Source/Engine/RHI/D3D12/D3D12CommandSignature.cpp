#include "D3D12CommandSignature.h"
#include "D3D12Device.h"

namespace RHI
{
	D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc::Build() noexcept
	{
		D3D12_COMMAND_SIGNATURE_DESC Desc = {};
		Desc.ByteStride					  = Stride;
		Desc.NumArgumentDescs			  = static_cast<UINT>(Parameters.size());
		Desc.pArgumentDescs				  = Parameters.data();
		Desc.NodeMask					  = 0;
		return Desc;
	}

	D3D12CommandSignature::D3D12CommandSignature(
		D3D12Device*		  Parent,
		CommandSignatureDesc& Builder,
		ID3D12RootSignature*  RootSignature)
		: D3D12DeviceChild(Parent)
	{
		D3D12_COMMAND_SIGNATURE_DESC Desc = Builder.Build();
		Desc.NodeMask					  = Parent->GetAllNodeMask();

		VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateCommandSignature(&Desc, RootSignature, IID_PPV_ARGS(&CommandSignature)));
	}
} // namespace RHI
