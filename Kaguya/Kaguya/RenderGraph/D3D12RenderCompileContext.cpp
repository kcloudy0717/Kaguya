#include "D3D12RenderCompileContext.h"

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandSetCBV& Command)
{


	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandSetSRV& Command)
{
	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandSetUAV& Command)
{
	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandBeginRenderPass& Command)
{
	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandEndRenderPass& Command)
{
	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandPipelineState& Command)
{
	const D3D12RootSignature& RootSignature = Device.GetRootSignature(Command.RootSignature);
	const D3D12PipelineState& PipelineState = Device.GetPipelineState(Command.PipelineState);

	switch (Command.PipelineType)
	{
	case ERenderPipelineType::Graphics:
	{
		Context->SetGraphicsRootSignature(RootSignature);
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor =
			Context.GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor =
			Context.GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

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
	break;

	case ERenderPipelineType::Compute:
	{
		Context->SetComputeRootSignature(RootSignature);
		const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor =
			Context.GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor =
			Context.GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

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
	break;
	}

	Context->SetPipelineState(PipelineState);

	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandRaytracingPipelineState& Command)
{
	const D3D12RootSignature&			RootSignature = Device.GetRootSignature(Command.RootSignature);
	const D3D12RaytracingPipelineState& PipelineState =
		Device.GetRaytracingPipelineState(Command.RaytracingPipelineState);

	Context->SetComputeRootSignature(RootSignature);
	const UINT Offset = RootSignature.GetDesc().NumParameters - RootParameters::DescriptorTable::NumRootParameters;
	D3D12_GPU_DESCRIPTOR_HANDLE ResourceDescriptor =
		Context.GetParentLinkedDevice()->GetResourceDescriptorHeap().GetGpuDescriptorHandle(0);
	D3D12_GPU_DESCRIPTOR_HANDLE SamplerDescriptor =
		Context.GetParentLinkedDevice()->GetSamplerDescriptorHeap().GetGpuDescriptorHandle(0);

	Context->SetComputeRootDescriptorTable(
		RootParameters::DescriptorTable::ShaderResourceDescriptorTable + Offset,
		ResourceDescriptor);
	Context->SetComputeRootDescriptorTable(
		RootParameters::DescriptorTable::UnorderedAccessDescriptorTable + Offset,
		ResourceDescriptor);
	Context->SetComputeRootDescriptorTable(
		RootParameters::DescriptorTable::SamplerDescriptorTable + Offset,
		SamplerDescriptor);

	Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(PipelineState);

	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandDraw& Command)
{
	Context.DrawInstanced(
		Command.VertexCount,
		Command.InstanceCount,
		Command.StartVertexLocation,
		Command.StartInstanceLocation);

	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandDrawIndexed& Command)
{
	Context.DrawIndexedInstanced(
		Command.IndexCount,
		Command.InstanceCount,
		Command.StartIndexLocation,
		Command.BaseVertexLocation,
		Command.StartInstanceLocation);

	return true;
}

bool D3D12RenderCompileContext::CompileCommand(const RenderCommandDispatch& Command)
{
	Context.Dispatch(Command.ThreadGroupCountX, Command.ThreadGroupCountY, Command.ThreadGroupCountZ);

	return true;
}
