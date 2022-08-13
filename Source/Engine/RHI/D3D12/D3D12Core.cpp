#include "D3D12Core.h"
#include "D3D12Fence.h"

namespace RHI
{
	const char* ExceptionD3D12::GetErrorType() const noexcept
	{
		return "[D3D12]";
	}

	std::string ExceptionD3D12::GetError() const
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
		return !!Fence;
	}

	UINT64 D3D12SyncHandle::GetValue() const noexcept
	{
		return Value;
	}

	bool D3D12SyncHandle::IsComplete() const
	{
		return Fence ? Fence->IsFenceComplete(Value) : true;
	}

	void D3D12SyncHandle::WaitForCompletion() const
	{
		if (Fence)
		{
			Fence->HostWaitForValue(Value);
		}
	}
} // namespace RHI
