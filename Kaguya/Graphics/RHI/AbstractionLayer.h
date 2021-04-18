#pragma once

enum class PrimitiveTopology
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class ComparisonFunc
{
	Never,			// Comparison always fails
	Less,			// Passes if source is less than the destination
	Equal,			// Passes if source is equal to the destination
	LessEqual,		// Passes if source is less than or equal to the destination
	Greater,		// Passes if source is greater than to the destination
	NotEqual,		// Passes if source is not equal to the destination
	GreaterEqual,	// Passes if source is greater than or equal to the destination
	Always			// Comparison always succeeds
};

D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3DPrimitiveTopologyType(PrimitiveTopology Topology);
D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc(ComparisonFunc Func);