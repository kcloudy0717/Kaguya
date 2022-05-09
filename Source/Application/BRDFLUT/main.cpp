// main.cpp : Defines the entry point for the application.
#include "System/System.h"
#include "RHI/RHI.h"

#include "Core/Asset/Texture.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 602;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

int main(int /*argc*/, char* /*argv*/[])
{
	RHI::DeviceOptions DeviceOptions = {};
#if _DEBUG
	DeviceOptions.EnableDebugLayer		   = true;
	DeviceOptions.EnableGpuBasedValidation = false;
	DeviceOptions.EnableAutoDebugName	   = true;
#endif
	DeviceOptions.FeatureLevel	   = D3D_FEATURE_LEVEL_12_0;
	DeviceOptions.Raytracing	   = true;
	DeviceOptions.DynamicResources = true;
	DeviceOptions.MeshShaders	   = true;
	ShaderCompiler	 Compiler;
	RHI::D3D12Device Device(DeviceOptions);

	Compiler.SetShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6);

	RHI::D3D12Texture			  Lut = { Device.GetLinkedDevice(), CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT, 256, 256, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) };
	RHI::D3D12UnorderedAccessView Uav = RHI::D3D12UnorderedAccessView{ Device.GetLinkedDevice(), &Lut, {}, {} };

	ShaderCompileOptions Options(L"CSMain");
	Shader				 Shader = Compiler.CompileCS(Process::ExecutableDirectory / "Shaders/Utility/BRDFLUT.cs.hlsl", Options);

	RHI::D3D12RootSignature RootSignature = Device.CreateRootSignature(
		RHI::RootSignatureDesc()
			.Add32BitConstants<0, 0>(1) // Parameters	register(b0, space0)
			.AllowResourceDescriptorHeapIndexing()
			.AllowSampleDescriptorHeapIndexing());

	struct PsoStream
	{
		PipelineStateStreamRootSignature RootSignature;
		PipelineStateStreamCS			 CS;
	} Stream;
	Stream.RootSignature = &RootSignature;
	Stream.CS			 = &Shader;

	RHI::D3D12PipelineState PipelineState = Device.CreatePipelineState(L"PipelineState", Stream);

	RHI::D3D12CommandContext& Context = Device.GetLinkedDevice()->GetCommandContext();
	Context.Open();
	{
		Context.SetPipelineState(&PipelineState);
		Context.SetComputeRootSignature(&RootSignature);
		Context->SetComputeRoot32BitConstant(0, Uav.GetIndex(), 0);
		Context.Dispatch2D<8, 8>(256, 256);
	}
	Context.Close();
	Context.Execute(true);

	Asset::Texture::SaveToDisk(Lut, "BrdfLut.dds");
}
