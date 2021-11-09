#include "D3D12Common.h"

LPCWSTR GetCommandQueueTypeString(ED3D12CommandQueueType CommandQueueType)
{
	// clang-format off
	switch (CommandQueueType)
	{
	case ED3D12CommandQueueType::Direct:		return L"3D";
	case ED3D12CommandQueueType::AsyncCompute:	return L"Async Compute";
	case ED3D12CommandQueueType::Copy1:			return L"Copy 1";
	case ED3D12CommandQueueType::Copy2:			return L"Copy 2";
	default:									return L"<unknown>";
	}
	// clang-format on
}

LPCWSTR GetCommandQueueTypeFenceString(ED3D12CommandQueueType CommandQueueType)
{
	// clang-format off
	switch (CommandQueueType)
	{
	case ED3D12CommandQueueType::Direct:		return L"3D Fence";
	case ED3D12CommandQueueType::AsyncCompute:	return L"Async Compute Fence";
	case ED3D12CommandQueueType::Copy1:			return L"Copy 1 Fence";
	case ED3D12CommandQueueType::Copy2:			return L"Copy 2 Fence";
	default:									return L"<unknown>";
	}
	// clang-format on
}

LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op)
{
#define STRINGIFY(x)                                                                                                   \
	case x:                                                                                                            \
		return L#x

	switch (Op)
	{
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETMARKER);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDEVENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCH);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTILES);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PRESENT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME);
		STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA);
	default:
		return L"<unknown>";
	}

#undef STRINGIFY
}

LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type)
{
#define STRINGIFY(x)                                                                                                   \
	case x:                                                                                                            \
		return L#x

	switch (Type)
	{
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_FENCE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_RESOURCE);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PASS);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_METACOMMAND);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP);
		STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_INVALID);
	default:
		return L"<unknown>";
	}

#undef STRINGIFY
}

D3D12Exception::D3D12Exception(const char* File, int Line, HRESULT ErrorCode)
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
#define STRINGIFY(x)                                                                                                   \
	case x:                                                                                                            \
		Error = #x;                                                                                                    \
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