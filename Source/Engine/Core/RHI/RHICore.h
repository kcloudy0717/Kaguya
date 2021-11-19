#pragma once

enum class RHI_PRIMITIVE_TOPOLOGY
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class RHI_COMPARISON_FUNC
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

enum class RHI_SAMPLER_FILTER
{
	Point,
	Linear,
	Anisotropic
};

enum class RHI_SAMPLER_ADDRESS_MODE
{
	Wrap,
	Mirror,
	Clamp,
	Border,
};

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopologyType(RHI_PRIMITIVE_TOPOLOGY Topology)
{
	// clang-format off
	switch (Topology)
	{
	case RHI_PRIMITIVE_TOPOLOGY::Undefined:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	case RHI_PRIMITIVE_TOPOLOGY::Point:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case RHI_PRIMITIVE_TOPOLOGY::Line:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case RHI_PRIMITIVE_TOPOLOGY::Triangle:	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case RHI_PRIMITIVE_TOPOLOGY::Patch:		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	default:							return D3D12_PRIMITIVE_TOPOLOGY_TYPE();
	}
	// clang-format on
}

constexpr D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(RHI_COMPARISON_FUNC Func)
{
	// clang-format off
	switch (Func)
	{
	case RHI_COMPARISON_FUNC::Never:		return D3D12_COMPARISON_FUNC_NEVER;
	case RHI_COMPARISON_FUNC::Less:			return D3D12_COMPARISON_FUNC_LESS;
	case RHI_COMPARISON_FUNC::Equal:		return D3D12_COMPARISON_FUNC_EQUAL;
	case RHI_COMPARISON_FUNC::LessEqual:	return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case RHI_COMPARISON_FUNC::Greater:		return D3D12_COMPARISON_FUNC_GREATER;
	case RHI_COMPARISON_FUNC::NotEqual:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case RHI_COMPARISON_FUNC::GreaterEqual:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case RHI_COMPARISON_FUNC::Always:		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:								return D3D12_COMPARISON_FUNC();
	}
	// clang-format on
}
