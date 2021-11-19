#include "DebugRenderer.h"
#include <RenderCore/RenderCore.h>

void DebugRenderer::Initialize(D3D12RenderPass* RenderPass)
{
	{
		ShaderCompileOptions Options(L"VSMain");
		VS = RenderCore::Compiler->CompileShader(
			SHADER_TYPE::Vertex,
			Application::ExecutableDirectory / L"Shaders/DebugRender.hlsl",
			Options);
	}
	{
		ShaderCompileOptions Options(L"PSMain");
		PS = RenderCore::Compiler->CompileShader(
			SHADER_TYPE::Pixel,
			Application::ExecutableDirectory / L"Shaders/DebugRender.hlsl",
			Options);
	}

	Rs = RenderCore::Device->CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 0>(16); // register(b0, space0)

			Builder.AllowInputLayout();
		});

	D3D12InputLayout InputLayout(2);
	InputLayout.AddVertexLayoutElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
	InputLayout.AddVertexLayoutElement("COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);

	DepthStencilState DepthStencilState;
	DepthStencilState.DepthEnable = false;

	struct PsoStream
	{
		PipelineStateStreamRootSignature	 RootSignature;
		PipelineStateStreamInputLayout		 InputLayout;
		PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
		PipelineStateStreamVS				 VS;
		PipelineStateStreamPS				 PS;
		PipelineStateStreamDepthStencilState DepthStencilState;
		PipelineStateStreamRenderPass		 RenderPass;
	} Stream;
	Stream.RootSignature		 = Rs.get();
	Stream.InputLayout			 = InputLayout;
	Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Line;
	Stream.VS					 = &VS;
	Stream.PS					 = &PS;
	Stream.DepthStencilState	 = DepthStencilState;
	Stream.RenderPass			 = RenderPass;

	Pso = RenderCore::Device->CreatePipelineState(Stream);
}

void DebugRenderer::Shutdown()
{
	VertexBuffer.reset();
	Pso.reset();
	Rs.reset();
}

void DebugRenderer::AddLine(Vector3f V0, Vector3f V1, Vector3f Color /*= Vector3f(1.0f)*/)
{
	if (!Enable)
	{
		return;
	}
	Lines.emplace_back(V0, Color);
	Lines.emplace_back(V1, Color);
}

void DebugRenderer::AddBoundingBox(
	const Transform&   Transform,
	const BoundingBox& Box,
	Vector3f		   Color /*= Vector3f(1.0f)*/)
{
	if (!Enable)
	{
		return;
	}
	DirectX::XMFLOAT4X4 Matrix;
	XMStoreFloat4x4(&Matrix, Transform.Matrix());

	BoundingBox TransformedBox;
	Box.Transform(Matrix, TransformedBox);

	auto	 Corners		 = TransformedBox.GetCorners();
	Vector3f FarBottomLeft	 = Corners[0];
	Vector3f FarBottomRight	 = Corners[1];
	Vector3f FarTopRight	 = Corners[2];
	Vector3f FarTopLeft		 = Corners[3];
	Vector3f NearBottomLeft	 = Corners[4];
	Vector3f NearBottomRight = Corners[5];
	Vector3f NearTopRight	 = Corners[6];
	Vector3f NearTopLeft	 = Corners[7];

	// Far face
	AddLine(FarTopLeft, FarTopRight, Color);
	AddLine(FarBottomLeft, FarBottomRight, Color);
	AddLine(FarTopLeft, FarBottomLeft, Color);
	AddLine(FarTopRight, FarBottomRight, Color);
	// Near face
	AddLine(NearTopLeft, NearTopRight, Color);
	AddLine(NearBottomLeft, NearBottomRight, Color);
	AddLine(NearTopLeft, NearBottomLeft, Color);
	AddLine(NearTopRight, NearBottomRight, Color);
	// Connect near and far face
	AddLine(NearTopLeft, FarTopLeft, Color);
	AddLine(NearTopRight, FarTopRight, Color);
	AddLine(NearBottomLeft, FarBottomLeft, Color);
	AddLine(NearBottomRight, FarBottomRight, Color);
}

void DebugRenderer::Render(const DirectX::XMFLOAT4X4& ViewProjection, D3D12CommandContext& Context)
{
	if (Lines.empty())
	{
		return;
	}

	UINT64 SizeInBytes = Lines.size() * sizeof(DebugVertex);
	if (!VertexBuffer || VertexBuffer->GetDesc().Width < SizeInBytes)
	{
		VertexBuffer = std::make_unique<D3D12Buffer>(
			RenderCore::Device->GetDevice(),
			SizeInBytes,
			static_cast<UINT>(sizeof(DebugVertex)),
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_FLAG_NONE);
		VertexBuffer->Initialize();
	}

	memcpy(VertexBuffer->GetCpuVirtualAddress<DebugVertex>(), Lines.data(), SizeInBytes);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = VertexBuffer->GetVertexBufferView();

	Context.SetGraphicsRootSignature(Rs.get());
	Context.SetPipelineState(Pso.get());

	Context->SetGraphicsRoot32BitConstants(0, 16, &ViewProjection, 0);

	Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	Context->IASetVertexBuffers(0, 1, &VertexBufferView);

	UINT VertexCount = static_cast<UINT>(Lines.size());
	Context->DrawInstanced(VertexCount, 1, 0, 0);
	Lines.clear();
}
