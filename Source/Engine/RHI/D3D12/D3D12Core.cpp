#include "D3D12Core.h"
#include "D3D12Fence.h"

namespace RHI
{
	D3D12Exception::D3D12Exception(std::string_view File, int Line, HRESULT ErrorCode)
		: Exception(File, Line)
		, ErrorCode(ErrorCode)
	{
	}

	const char* D3D12Exception::GetErrorType() const noexcept
	{
		return "[D3D12]";
	}

	std::string D3D12Exception::GetError() const
	{
#define STRINGIFY(x) \
	case x:          \
		Error = #x;  \
		break

		std::string Error;
		// https://docs.microsoft.com/en-us/windows/win32/direct3d12/d3d12-graphics-reference-returnvalues
		switch (ErrorCode)
		{
			STRINGIFY(D3D12_ERROR_ADAPTER_NOT_FOUND);
			STRINGIFY(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
			STRINGIFY(DXGI_ERROR_INVALID_CALL);
			STRINGIFY(DXGI_ERROR_WAS_STILL_DRAWING);
			STRINGIFY(E_FAIL);
			STRINGIFY(E_INVALIDARG);
			STRINGIFY(E_OUTOFMEMORY);
			STRINGIFY(E_NOTIMPL);
			STRINGIFY(E_NOINTERFACE);

			// This just follows the documentation
			// DXGI
			STRINGIFY(DXGI_ERROR_ACCESS_DENIED);
			STRINGIFY(DXGI_ERROR_ACCESS_LOST);
			STRINGIFY(DXGI_ERROR_ALREADY_EXISTS);
			STRINGIFY(DXGI_ERROR_CANNOT_PROTECT_CONTENT);
			STRINGIFY(DXGI_ERROR_DEVICE_HUNG);
			STRINGIFY(DXGI_ERROR_DEVICE_REMOVED);
			STRINGIFY(DXGI_ERROR_DEVICE_RESET);
			STRINGIFY(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
			STRINGIFY(DXGI_ERROR_FRAME_STATISTICS_DISJOINT);
			STRINGIFY(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE);
			// DXERR(DXGI_ERROR_INVALID_CALL); // Already defined
			STRINGIFY(DXGI_ERROR_MORE_DATA);
			STRINGIFY(DXGI_ERROR_NAME_ALREADY_EXISTS);
			STRINGIFY(DXGI_ERROR_NONEXCLUSIVE);
			STRINGIFY(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
			STRINGIFY(DXGI_ERROR_NOT_FOUND);
			// DXERR(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED); // Reserved
			// DXERR(DXGI_ERROR_REMOTE_OUTOFMEMORY); // Reserved
			STRINGIFY(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE);
			STRINGIFY(DXGI_ERROR_SDK_COMPONENT_MISSING);
			STRINGIFY(DXGI_ERROR_SESSION_DISCONNECTED);
			STRINGIFY(DXGI_ERROR_UNSUPPORTED);
			STRINGIFY(DXGI_ERROR_WAIT_TIMEOUT);
			// DXERR(DXGI_ERROR_WAS_STILL_DRAWING); // Already defined
		default:
		{
			char Buffer[64] = {};
			sprintf_s(Buffer, "HRESULT of 0x%08X", static_cast<UINT>(ErrorCode));
			Error = Buffer;
		}
		break;
		}

		return Error;
#undef STRINGIFY
	}

	D3D12SyncHandle::operator bool() const noexcept
	{
		return Fence != nullptr;
	}

	auto D3D12SyncHandle::GetValue() const noexcept -> UINT64
	{
		assert(static_cast<bool>(*this));
		return Value;
	}

	auto D3D12SyncHandle::IsComplete() const -> bool
	{
		assert(static_cast<bool>(*this));
		return Fence->IsFenceComplete(Value);
	}

	auto D3D12SyncHandle::WaitForCompletion() const -> void
	{
		assert(static_cast<bool>(*this));
		Fence->HostWaitForValue(Value);
	}

	D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(UINT Subresource) const
	{
		if (TrackingMode == ETrackingMode::PerResource)
		{
			return ResourceState;
		}

		return SubresourceStates[Subresource];
	}

	void CResourceState::SetSubresourceState(UINT Subresource, D3D12_RESOURCE_STATES State)
	{
		// If setting all subresources, or the resource only has a single subresource, set the per-resource state
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES || SubresourceStates.size() == 1)
		{
			TrackingMode  = ETrackingMode::PerResource;
			ResourceState = State;
		}
		else
		{
			// If we previous tracked resource per resource level, we need to update all
			// all subresource states before proceeding
			if (TrackingMode == ETrackingMode::PerResource)
			{
				TrackingMode = ETrackingMode::PerSubresource;
				for (auto& SubresourceState : SubresourceStates)
				{
					SubresourceState = ResourceState;
				}
			}
			SubresourceStates[Subresource] = State;
		}
	}
} // namespace RHI
