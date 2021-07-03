#pragma once
#include "D3D12/ShaderCompiler.h"

#include "D3D12/D3D12Utility.h"
#include "D3D12/Device.h"
#include "D3D12/CommandQueue.h"
#include "D3D12/PipelineState.h"
#include "D3D12/RaytracingPipelineState.h"
#include "D3D12/Profiler.h"
#include "D3D12/Resource.h"
#include "D3D12/ResourceUploader.h"
#include "D3D12/RootSignature.h"
#include "D3D12/ShaderCompiler.h"
#include "D3D12/SwapChain.h"
#include "D3D12/Raytracing.h"
#include "D3D12/RaytracingShaderTable.h"

struct RootParameters
{
	struct DescriptorTable
	{
		enum
		{
			ShaderResourceDescriptorTable,
			UnorderedAccessDescriptorTable,
			SamplerDescriptorTable,
			NumRootParameters
		};
	};
};

class RenderDevice
{
public:
	static constexpr DXGI_FORMAT DepthFormat		= DXGI_FORMAT_D32_FLOAT;
	static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static void			 Initialize();
	static void			 Shutdown();
	static RenderDevice& Instance();

	static bool IsValid();

	void Present(bool VSync);

	void Resize(UINT Width, UINT Height);

	void BindGraphicsDescriptorTable(const RootSignature& RootSignature, CommandContext& Context)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor = m_Device->GetResourceDescriptorHeap().hGPU(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor  = m_Device->GetSamplerDescriptorHeap().hGPU(0);

		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetGraphicsRootDescriptorTable(
			RootParameters::DescriptorTable::SamplerDescriptorTable + Offset,
			SamplerDescriptor);
	}

	void BindComputeDescriptorTable(const RootSignature& RootSignature, CommandContext& Context)
	{
		// Assumes the RootSignature was created with AddDescriptorTableRootParameterToBuilder function called.
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor = m_Device->GetResourceDescriptorHeap().hGPU(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor  = m_Device->GetSamplerDescriptorHeap().hGPU(0);

		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset,
			ResourceDescriptor);
		Context->SetComputeRootDescriptorTable(
			RootParameters::DescriptorTable::SamplerDescriptorTable + Offset,
			SamplerDescriptor);
	}

	[[nodiscard]] RootSignature CreateRootSignature(
		std::function<void(RootSignatureBuilder&)> Configurator,
		bool									   AddDescriptorTableRootParameters = true);

	template<typename PipelineStateStream>
	[[nodiscard]] PipelineState CreatePipelineState(PipelineStateStream& Stream)
	{
		return PipelineState(m_Device->GetDevice5(), Stream);
	}

	[[nodiscard]] RaytracingPipelineState CreateRaytracingPipelineState(
		std::function<void(RaytracingPipelineStateBuilder&)> Configurator);

	const auto&					 GetAdapterDesc() const { return AdapterDesc; }
	DXGI_QUERY_VIDEO_MEMORY_INFO QueryLocalVideoMemoryInfo() const;

	auto GetCurrentBackBufferResource() const { return m_SwapChain.GetCurrentBackBufferResource(); }

	Device* GetDevice() noexcept { return m_Device.get(); }

private:
	RenderDevice();
	~RenderDevice();

	RenderDevice(const RenderDevice&) = delete;
	RenderDevice& operator=(const RenderDevice&) = delete;

	void InitializeDXGIObjects();

	void AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory6> Factory6;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter4;
	DXGI_ADAPTER_DESC3					  AdapterDesc;

	std::unique_ptr<Device> m_Device;

	SwapChain m_SwapChain;

	D3D12_CPU_DESCRIPTOR_HANDLE m_ImGuiFontCpuDescriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE m_ImGuiFontGpuDescriptor;
};
