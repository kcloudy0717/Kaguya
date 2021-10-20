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
	Never,		  // Comparison always fails
	Less,		  // Passes if source is less than the destination
	Equal,		  // Passes if source is equal to the destination
	LessEqual,	  // Passes if source is less than or equal to the destination
	Greater,	  // Passes if source is greater than to the destination
	NotEqual,	  // Passes if source is not equal to the destination
	GreaterEqual, // Passes if source is greater than or equal to the destination
	Always		  // Comparison always succeeds
};

enum class ESamplerFilter
{
	Point,
	Linear,
	Anisotropic
};

enum class ESamplerAddressMode
{
	Wrap,
	Mirror,
	Clamp,
	Border,
};

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopologyType(PrimitiveTopology Topology)
{
	// clang-format off
	switch (Topology)
	{
	case PrimitiveTopology::Undefined:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	case PrimitiveTopology::Point:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case PrimitiveTopology::Line:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case PrimitiveTopology::Triangle:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case PrimitiveTopology::Patch:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	default:							return D3D12_PRIMITIVE_TOPOLOGY_TYPE();
	}
	// clang-format on
}

constexpr D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(ComparisonFunc Func)
{
	// clang-format off
	switch (Func)
	{
	case ComparisonFunc::Never:			return D3D12_COMPARISON_FUNC_NEVER;
	case ComparisonFunc::Less:			return D3D12_COMPARISON_FUNC_LESS;
	case ComparisonFunc::Equal:			return D3D12_COMPARISON_FUNC_EQUAL;
	case ComparisonFunc::LessEqual:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ComparisonFunc::Greater:		return D3D12_COMPARISON_FUNC_GREATER;
	case ComparisonFunc::NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ComparisonFunc::GreaterEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ComparisonFunc::Always:		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:							return D3D12_COMPARISON_FUNC();
	}
	// clang-format on
}
