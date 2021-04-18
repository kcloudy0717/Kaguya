#include "pch.h"
#include "AbstractionLayer.h"

D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3DPrimitiveTopologyType(PrimitiveTopology Topology)
{
	switch (Topology)
	{
	case PrimitiveTopology::Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	case PrimitiveTopology::Point: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case PrimitiveTopology::Line: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case PrimitiveTopology::Triangle: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case PrimitiveTopology::Patch: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	}
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE();
}

D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc(ComparisonFunc Func)
{
	switch (Func)
	{
	case ComparisonFunc::Never: return D3D12_COMPARISON_FUNC_NEVER;
	case ComparisonFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
	case ComparisonFunc::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
	case ComparisonFunc::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ComparisonFunc::Greater: return D3D12_COMPARISON_FUNC_GREATER;
	case ComparisonFunc::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ComparisonFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ComparisonFunc::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	return D3D12_COMPARISON_FUNC();
}