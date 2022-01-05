#pragma once
#include <DirectXMath.h>
#include "World/Components.h"

// CURRENTLY UNSTABLE

#define ENABLE_DEBUG_RENDERER 1

struct DebugVertex
{
	Vec3f Position;
	Vec3f Color;
};

class DebugRenderer
{
public:
	static void Initialize();
	static void Shutdown();

	static void AddLine(Vec3f V0, Vec3f V1, Vec3f Color = Vec3f(1.0f));
	static void AddBoundingBox(const Transform& Transform, const BoundingBox& Box, Vec3f Color = Vec3f(1.0f));

	static void Render(const DirectX::XMFLOAT4X4& ViewProjection, D3D12CommandContext& Context);

	inline static bool Enable = true;

private:
	inline static Shader VS, PS;

	inline static std::unique_ptr<D3D12RootSignature> Rs;
	inline static std::unique_ptr<D3D12PipelineState> Pso;

	inline static std::unique_ptr<D3D12Buffer> VertexBuffer;

	inline static std::vector<DebugVertex> Lines;
};

#if _DEBUG || ENABLE_DEBUG_RENDERER
#define DEBUG_RENDERER_INITIALIZE(RenderPass)				  DebugRenderer::Initialize(RenderPass)
#define DEBUG_RENDERER_SHUTDOWN()							  DebugRenderer::Shutdown();
#define DEBUG_RENDERER_ADD_LINE(V0, V1, Color)				  DebugRenderer::AddLine(V0, V1, Color)
#define DEBUG_RENDERER_ADD_BOUNDINGBOX(Transform, Box, Color) DebugRenderer::AddBoundingBox(Transform, Box, Color)
#define DEBUG_RENDERER_RENDER(ViewProjection, Context)		  DebugRenderer::Render(ViewProjection, Context)
#else
#define DEBUG_RENDERER_INITIALIZE(RenderPass)
#define DEBUG_RENDERER_SHUTDOWN()
#define DEBUG_RENDERER_ADD_LINE(V0, V1, Color)
#define DEBUG_RENDERER_ADD_BOUNDINGBOX(Matrix, Box, Color)
#define DEBUG_RENDERER_RENDER(ViewProjection, Context)
#endif
