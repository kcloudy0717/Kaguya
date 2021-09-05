#include "VulkanCommon.h"

const char* VulkanException::GetErrorType() const noexcept
{
	return "[Vulkan]";
}

std::string VulkanException::GetError() const
{
#define VKERR(x)                                                                                                       \
	case x:                                                                                                            \
		Error = #x;                                                                                                    \
		break

	std::string Error;
	switch (ErrorCode)
	{
		VKERR(VK_NOT_READY);
		VKERR(VK_TIMEOUT);
		VKERR(VK_EVENT_SET);
		VKERR(VK_EVENT_RESET);
		VKERR(VK_INCOMPLETE);
		VKERR(VK_ERROR_OUT_OF_HOST_MEMORY);
		VKERR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		VKERR(VK_ERROR_INITIALIZATION_FAILED);
		VKERR(VK_ERROR_DEVICE_LOST);
		VKERR(VK_ERROR_MEMORY_MAP_FAILED);
		VKERR(VK_ERROR_LAYER_NOT_PRESENT);
		VKERR(VK_ERROR_EXTENSION_NOT_PRESENT);
		VKERR(VK_ERROR_FEATURE_NOT_PRESENT);
		VKERR(VK_ERROR_INCOMPATIBLE_DRIVER);
		VKERR(VK_ERROR_TOO_MANY_OBJECTS);
		VKERR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		VKERR(VK_ERROR_FRAGMENTED_POOL);
		VKERR(VK_ERROR_UNKNOWN);
		VKERR(VK_ERROR_OUT_OF_POOL_MEMORY);
		VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
		VKERR(VK_ERROR_FRAGMENTATION);
		VKERR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
		VKERR(VK_ERROR_SURFACE_LOST_KHR);
		VKERR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
		VKERR(VK_SUBOPTIMAL_KHR);
		VKERR(VK_ERROR_OUT_OF_DATE_KHR);
		VKERR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
		VKERR(VK_ERROR_VALIDATION_FAILED_EXT);
		VKERR(VK_ERROR_INVALID_SHADER_NV);
		VKERR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
		VKERR(VK_ERROR_NOT_PERMITTED_EXT);
		VKERR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
		VKERR(VK_THREAD_IDLE_KHR);
		VKERR(VK_THREAD_DONE_KHR);
		VKERR(VK_OPERATION_DEFERRED_KHR);
		VKERR(VK_OPERATION_NOT_DEFERRED_KHR);
		VKERR(VK_PIPELINE_COMPILE_REQUIRED_EXT);

		// VKERR(VK_ERROR_OUT_OF_POOL_MEMORY_KHR);
		// VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR);
		// VKERR(VK_ERROR_FRAGMENTATION_EXT);
		// VKERR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
		// VKERR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
		// VKERR(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
	default:
	{
		char Buffer[64] = {};
		sprintf_s(Buffer, "VkResult of 0x%08X", static_cast<UINT>(ErrorCode));
		Error = Buffer;
	}
	break;
	}
#undef VKERR
	return Error;
}

auto VulkanCommandSyncPoint::IsValid() const noexcept -> bool
{
	return Fence != nullptr;
}

auto VulkanCommandSyncPoint::GetValue() const noexcept -> UINT64
{
	assert(IsValid());
	return Value;
}

auto VulkanCommandSyncPoint::IsComplete() const -> bool
{
	assert(IsValid());
	return Fence->GetCompletedValue() >= Value;
}

auto VulkanCommandSyncPoint::WaitForCompletion() const -> void
{
	assert(IsValid());
	const VkSemaphore Semaphores[] = { Fence->GetApiHandle() };

	auto SemaphoreWaitInfo			 = VkStruct<VkSemaphoreWaitInfo>();
	SemaphoreWaitInfo.flags			 = 0;
	SemaphoreWaitInfo.semaphoreCount = 1;
	SemaphoreWaitInfo.pSemaphores	 = Semaphores;
	SemaphoreWaitInfo.pValues		 = &Value;

	VERIFY_VULKAN_API(vkWaitSemaphores(Fence->GetParentDevice()->GetVkDevice(), &SemaphoreWaitInfo, UINT64_MAX));
}

enum TextLineType
{
	TEXT_LINE_TYPE_BLOCK_BEGIN	= 0x01,
	TEXT_LINE_TYPE_BLOCK_END	= 0x02,
	TEXT_LINE_TYPE_STRUCT_BEGIN = 0x04,
	TEXT_LINE_TYPE_STRUCT_END	= 0x08,
	TEXT_LINE_TYPE_LINES		= 0x10,
};

struct TextLine
{
	std::string indent;
	std::string modifier;
	std::string type_name;
	std::string name;
	uint32_t	absolute_offset;
	uint32_t	relative_offset;
	uint32_t	size;
	uint32_t	padded_size;
	uint32_t	array_stride;
	uint32_t	block_variable_flags;
	// Text Data
	uint32_t			  text_line_flags;
	std::vector<TextLine> lines;
	std::string			  formatted_line;
	std::string			  formatted_absolute_offset;
	std::string			  formatted_relative_offset;
	std::string			  formatted_size;
	std::string			  formatted_padded_size;
	std::string			  formatted_array_stride;
	std::string			  formatted_block_variable_flags;
};

static std::string AsHexString(uint32_t n)
{
	// std::iomanip can die in a fire.
	char out_word[11];
	int	 len = snprintf(out_word, 11, "0x%08X", n);
	assert(len == 10);
	(void)len;
	return std::string(out_word);
}

std::string ToStringGenerator(SpvReflectGenerator generator)
{
	switch (generator)
	{
	case SPV_REFLECT_GENERATOR_KHRONOS_LLVM_SPIRV_TRANSLATOR:
		return "Khronos LLVM/SPIR-V Translator";
		break;
	case SPV_REFLECT_GENERATOR_KHRONOS_SPIRV_TOOLS_ASSEMBLER:
		return "Khronos SPIR-V Tools Assembler";
		break;
	case SPV_REFLECT_GENERATOR_KHRONOS_GLSLANG_REFERENCE_FRONT_END:
		return "Khronos Glslang Reference Front End";
		break;
	case SPV_REFLECT_GENERATOR_GOOGLE_SHADERC_OVER_GLSLANG:
		return "Google Shaderc over Glslang";
		break;
	case SPV_REFLECT_GENERATOR_GOOGLE_SPIREGG:
		return "Google spiregg";
		break;
	case SPV_REFLECT_GENERATOR_GOOGLE_RSPIRV:
		return "Google rspirv";
		break;
	case SPV_REFLECT_GENERATOR_X_LEGEND_MESA_MESAIR_SPIRV_TRANSLATOR:
		return "X-LEGEND Mesa-IR/SPIR-V Translator";
		break;
	case SPV_REFLECT_GENERATOR_KHRONOS_SPIRV_TOOLS_LINKER:
		return "Khronos SPIR-V Tools Linker";
		break;
	case SPV_REFLECT_GENERATOR_WINE_VKD3D_SHADER_COMPILER:
		return "Wine VKD3D Shader Compiler";
		break;
	case SPV_REFLECT_GENERATOR_CLAY_CLAY_SHADER_COMPILER:
		return "Clay Clay Shader Compiler";
		break;
	}
	// unhandled SpvReflectGenerator enum value
	return "???";
}

std::string ToStringSpvSourceLanguage(SpvSourceLanguage lang)
{
	switch (lang)
	{
	case SpvSourceLanguageUnknown:
		return "Unknown";
	case SpvSourceLanguageESSL:
		return "ESSL";
	case SpvSourceLanguageGLSL:
		return "GLSL";
	case SpvSourceLanguageOpenCL_C:
		return "OpenCL_C";
	case SpvSourceLanguageOpenCL_CPP:
		return "OpenCL_CPP";
	case SpvSourceLanguageHLSL:
		return "HLSL";

	case SpvSourceLanguageMax:
		break;
	}
	// unhandled SpvSourceLanguage enum value
	return "???";
}

std::string ToStringSpvExecutionModel(SpvExecutionModel model)
{
	switch (model)
	{
	case SpvExecutionModelVertex:
		return "Vertex";
	case SpvExecutionModelTessellationControl:
		return "TessellationControl";
	case SpvExecutionModelTessellationEvaluation:
		return "TessellationEvaluation";
	case SpvExecutionModelGeometry:
		return "Geometry";
	case SpvExecutionModelFragment:
		return "Fragment";
	case SpvExecutionModelGLCompute:
		return "GLCompute";
	case SpvExecutionModelKernel:
		return "Kernel";
	case SpvExecutionModelTaskNV:
		return "TaskNV";
	case SpvExecutionModelMeshNV:
		return "MeshNV";
	case SpvExecutionModelRayGenerationKHR:
		return "RayGenerationKHR";
	case SpvExecutionModelIntersectionKHR:
		return "IntersectionKHR";
	case SpvExecutionModelAnyHitKHR:
		return "AnyHitKHR";
	case SpvExecutionModelClosestHitKHR:
		return "ClosestHitKHR";
	case SpvExecutionModelMissKHR:
		return "MissKHR";
	case SpvExecutionModelCallableKHR:
		return "CallableKHR";

	case SpvExecutionModelMax:
		break;
	}
	// unhandled SpvExecutionModel enum value
	return "???";
}

std::string ToStringShaderStage(SpvReflectShaderStageFlagBits stage)
{
	switch (stage)
	{
	case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
		return "VS";
	case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return "HS";
	case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return "DS";
	case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
		return "GS";
	case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
		return "PS";
	case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
		return "CS";
	case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV:
		return "TASK";
	case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV:
		return "MESH";
	case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
		return "RAYGEN";
	case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
		return "ANY_HIT";
	case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
		return "CLOSEST_HIT";
	case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
		return "MISS";
	case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
		return "INTERSECTION";
	case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
		return "CALLABLE";
	}
	// Unhandled SpvReflectShaderStageFlagBits enum value
	return "???";
}

std::string ToStringSpvStorageClass(SpvStorageClass storage_class)
{
	switch (storage_class)
	{
	case SpvStorageClassUniformConstant:
		return "UniformConstant";
	case SpvStorageClassInput:
		return "Input";
	case SpvStorageClassUniform:
		return "Uniform";
	case SpvStorageClassOutput:
		return "Output";
	case SpvStorageClassWorkgroup:
		return "Workgroup";
	case SpvStorageClassCrossWorkgroup:
		return "CrossWorkgroup";
	case SpvStorageClassPrivate:
		return "Private";
	case SpvStorageClassFunction:
		return "Function";
	case SpvStorageClassGeneric:
		return "Generic";
	case SpvStorageClassPushConstant:
		return "PushConstant";
	case SpvStorageClassAtomicCounter:
		return "AtomicCounter";
	case SpvStorageClassImage:
		return "Image";
	case SpvStorageClassStorageBuffer:
		return "StorageBuffer";
	case SpvStorageClassCallableDataKHR:
		return "CallableDataKHR";
	case SpvStorageClassIncomingCallableDataKHR:
		return "IncomingCallableDataKHR";
	case SpvStorageClassRayPayloadKHR:
		return "RayPayloadKHR";
	case SpvStorageClassHitAttributeKHR:
		return "HitAttributeKHR";
	case SpvStorageClassIncomingRayPayloadKHR:
		return "IncomingRayPayloadKHR";
	case SpvStorageClassShaderRecordBufferKHR:
		return "ShaderRecordBufferKHR";
	case SpvStorageClassPhysicalStorageBuffer:
		return "PhysicalStorageBuffer";
	case SpvStorageClassCodeSectionINTEL:
		return "CodeSectionINTEL";

	case SpvStorageClassMax:
		break;
	}

	// Special case: this specific "unhandled" value does actually seem to show up.
	if (storage_class == (SpvStorageClass)-1)
	{
		return "NOT APPLICABLE";
	}

	// unhandled SpvStorageClass enum value
	return "???";
}

std::string ToStringSpvDim(SpvDim dim)
{
	switch (dim)
	{
	case SpvDim1D:
		return "1D";
	case SpvDim2D:
		return "2D";
	case SpvDim3D:
		return "3D";
	case SpvDimCube:
		return "Cube";
	case SpvDimRect:
		return "Rect";
	case SpvDimBuffer:
		return "Buffer";
	case SpvDimSubpassData:
		return "SubpassData";

	case SpvDimMax:
		break;
	}
	// unhandled SpvDim enum value
	return "???";
}

std::string ToStringResourceType(SpvReflectResourceType res_type)
{
	switch (res_type)
	{
	case SPV_REFLECT_RESOURCE_FLAG_UNDEFINED:
		return "UNDEFINED";
	case SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
		return "SAMPLER";
	case SPV_REFLECT_RESOURCE_FLAG_CBV:
		return "CBV";
	case SPV_REFLECT_RESOURCE_FLAG_SRV:
		return "SRV";
	case SPV_REFLECT_RESOURCE_FLAG_UAV:
		return "UAV";
	}
	// unhandled SpvReflectResourceType enum value
	return "???";
}

std::string ToStringDescriptorType(SpvReflectDescriptorType value)
{
	switch (value)
	{
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
		return "VK_DESCRIPTOR_TYPE_SAMPLER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
	case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
	case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
	case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
	case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
		return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
	}
	// unhandled SpvReflectDescriptorType enum value
	return "VK_DESCRIPTOR_TYPE_???";
}

std::string ToStringSpvBuiltIn(SpvBuiltIn built_in)
{
	switch (built_in)
	{
	case SpvBuiltInPosition:
		return "Position";
	case SpvBuiltInPointSize:
		return "PointSize";
	case SpvBuiltInClipDistance:
		return "ClipDistance";
	case SpvBuiltInCullDistance:
		return "CullDistance";
	case SpvBuiltInVertexId:
		return "VertexId";
	case SpvBuiltInInstanceId:
		return "InstanceId";
	case SpvBuiltInPrimitiveId:
		return "PrimitiveId";
	case SpvBuiltInInvocationId:
		return "InvocationId";
	case SpvBuiltInLayer:
		return "Layer";
	case SpvBuiltInViewportIndex:
		return "ViewportIndex";
	case SpvBuiltInTessLevelOuter:
		return "TessLevelOuter";
	case SpvBuiltInTessLevelInner:
		return "TessLevelInner";
	case SpvBuiltInTessCoord:
		return "TessCoord";
	case SpvBuiltInPatchVertices:
		return "PatchVertices";
	case SpvBuiltInFragCoord:
		return "FragCoord";
	case SpvBuiltInPointCoord:
		return "PointCoord";
	case SpvBuiltInFrontFacing:
		return "FrontFacing";
	case SpvBuiltInSampleId:
		return "SampleId";
	case SpvBuiltInSamplePosition:
		return "SamplePosition";
	case SpvBuiltInSampleMask:
		return "SampleMask";
	case SpvBuiltInFragDepth:
		return "FragDepth";
	case SpvBuiltInHelperInvocation:
		return "HelperInvocation";
	case SpvBuiltInNumWorkgroups:
		return "NumWorkgroups";
	case SpvBuiltInWorkgroupSize:
		return "WorkgroupSize";
	case SpvBuiltInWorkgroupId:
		return "WorkgroupId";
	case SpvBuiltInLocalInvocationId:
		return "LocalInvocationId";
	case SpvBuiltInGlobalInvocationId:
		return "GlobalInvocationId";
	case SpvBuiltInLocalInvocationIndex:
		return "LocalInvocationIndex";
	case SpvBuiltInWorkDim:
		return "WorkDim";
	case SpvBuiltInGlobalSize:
		return "GlobalSize";
	case SpvBuiltInEnqueuedWorkgroupSize:
		return "EnqueuedWorkgroupSize";
	case SpvBuiltInGlobalOffset:
		return "GlobalOffset";
	case SpvBuiltInGlobalLinearId:
		return "GlobalLinearId";
	case SpvBuiltInSubgroupSize:
		return "SubgroupSize";
	case SpvBuiltInSubgroupMaxSize:
		return "SubgroupMaxSize";
	case SpvBuiltInNumSubgroups:
		return "NumSubgroups";
	case SpvBuiltInNumEnqueuedSubgroups:
		return "NumEnqueuedSubgroups";
	case SpvBuiltInSubgroupId:
		return "SubgroupId";
	case SpvBuiltInSubgroupLocalInvocationId:
		return "SubgroupLocalInvocationId";
	case SpvBuiltInVertexIndex:
		return "VertexIndex";
	case SpvBuiltInInstanceIndex:
		return "InstanceIndex";
	case SpvBuiltInSubgroupEqMaskKHR:
		return "SubgroupEqMaskKHR";
	case SpvBuiltInSubgroupGeMaskKHR:
		return "SubgroupGeMaskKHR";
	case SpvBuiltInSubgroupGtMaskKHR:
		return "SubgroupGtMaskKHR";
	case SpvBuiltInSubgroupLeMaskKHR:
		return "SubgroupLeMaskKHR";
	case SpvBuiltInSubgroupLtMaskKHR:
		return "SubgroupLtMaskKHR";
	case SpvBuiltInBaseVertex:
		return "BaseVertex";
	case SpvBuiltInBaseInstance:
		return "BaseInstance";
	case SpvBuiltInDrawIndex:
		return "DrawIndex";
	case SpvBuiltInDeviceIndex:
		return "DeviceIndex";
	case SpvBuiltInViewIndex:
		return "ViewIndex";
	case SpvBuiltInBaryCoordNoPerspAMD:
		return "BaryCoordNoPerspAMD";
	case SpvBuiltInBaryCoordNoPerspCentroidAMD:
		return "BaryCoordNoPerspCentroidAMD";
	case SpvBuiltInBaryCoordNoPerspSampleAMD:
		return "BaryCoordNoPerspSampleAMD";
	case SpvBuiltInBaryCoordSmoothAMD:
		return "BaryCoordSmoothAMD";
	case SpvBuiltInBaryCoordSmoothCentroidAMD:
		return "BaryCoordSmoothCentroidAMD";
	case SpvBuiltInBaryCoordSmoothSampleAMD:
		return "BaryCoordSmoothSampleAMD";
	case SpvBuiltInBaryCoordPullModelAMD:
		return "BaryCoordPullModelAMD";
	case SpvBuiltInFragStencilRefEXT:
		return "FragStencilRefEXT";
	case SpvBuiltInViewportMaskNV:
		return "ViewportMaskNV";
	case SpvBuiltInSecondaryPositionNV:
		return "SecondaryPositionNV";
	case SpvBuiltInSecondaryViewportMaskNV:
		return "SecondaryViewportMaskNV";
	case SpvBuiltInPositionPerViewNV:
		return "PositionPerViewNV";
	case SpvBuiltInViewportMaskPerViewNV:
		return "ViewportMaskPerViewNV";
	case SpvBuiltInLaunchIdKHR:
		return "InLaunchIdKHR";
	case SpvBuiltInLaunchSizeKHR:
		return "InLaunchSizeKHR";
	case SpvBuiltInWorldRayOriginKHR:
		return "InWorldRayOriginKHR";
	case SpvBuiltInWorldRayDirectionKHR:
		return "InWorldRayDirectionKHR";
	case SpvBuiltInObjectRayOriginKHR:
		return "InObjectRayOriginKHR";
	case SpvBuiltInObjectRayDirectionKHR:
		return "InObjectRayDirectionKHR";
	case SpvBuiltInRayTminKHR:
		return "InRayTminKHR";
	case SpvBuiltInRayTmaxKHR:
		return "InRayTmaxKHR";
	case SpvBuiltInInstanceCustomIndexKHR:
		return "InInstanceCustomIndexKHR";
	case SpvBuiltInObjectToWorldKHR:
		return "InObjectToWorldKHR";
	case SpvBuiltInWorldToObjectKHR:
		return "InWorldToObjectKHR";
	case SpvBuiltInHitTNV:
		return "InHitTNV";
	case SpvBuiltInHitKindKHR:
		return "InHitKindKHR";
	case SpvBuiltInIncomingRayFlagsKHR:
		return "InIncomingRayFlagsKHR";
	case SpvBuiltInRayGeometryIndexKHR:
		return "InRayGeometryIndexKHR";

	case SpvBuiltInMax:
	default:
		break;
	}
	// unhandled SpvBuiltIn enum value
	std::stringstream ss;
	ss << "??? (" << built_in << ")";
	return ss.str();
}

std::string ToStringSpvImageFormat(SpvImageFormat fmt)
{
	switch (fmt)
	{
	case SpvImageFormatUnknown:
		return "Unknown";
	case SpvImageFormatRgba32f:
		return "Rgba32f";
	case SpvImageFormatRgba16f:
		return "Rgba16f";
	case SpvImageFormatR32f:
		return "R32f";
	case SpvImageFormatRgba8:
		return "Rgba8";
	case SpvImageFormatRgba8Snorm:
		return "Rgba8Snorm";
	case SpvImageFormatRg32f:
		return "Rg32f";
	case SpvImageFormatRg16f:
		return "Rg16f";
	case SpvImageFormatR11fG11fB10f:
		return "R11fG11fB10f";
	case SpvImageFormatR16f:
		return "R16f";
	case SpvImageFormatRgba16:
		return "Rgba16";
	case SpvImageFormatRgb10A2:
		return "Rgb10A2";
	case SpvImageFormatRg16:
		return "Rg16";
	case SpvImageFormatRg8:
		return "Rg8";
	case SpvImageFormatR16:
		return "R16";
	case SpvImageFormatR8:
		return "R8";
	case SpvImageFormatRgba16Snorm:
		return "Rgba16Snorm";
	case SpvImageFormatRg16Snorm:
		return "Rg16Snorm";
	case SpvImageFormatRg8Snorm:
		return "Rg8Snorm";
	case SpvImageFormatR16Snorm:
		return "R16Snorm";
	case SpvImageFormatR8Snorm:
		return "R8Snorm";
	case SpvImageFormatRgba32i:
		return "Rgba32i";
	case SpvImageFormatRgba16i:
		return "Rgba16i";
	case SpvImageFormatRgba8i:
		return "Rgba8i";
	case SpvImageFormatR32i:
		return "R32i";
	case SpvImageFormatRg32i:
		return "Rg32i";
	case SpvImageFormatRg16i:
		return "Rg16i";
	case SpvImageFormatRg8i:
		return "Rg8i";
	case SpvImageFormatR16i:
		return "R16i";
	case SpvImageFormatR8i:
		return "R8i";
	case SpvImageFormatRgba32ui:
		return "Rgba32ui";
	case SpvImageFormatRgba16ui:
		return "Rgba16ui";
	case SpvImageFormatRgba8ui:
		return "Rgba8ui";
	case SpvImageFormatR32ui:
		return "R32ui";
	case SpvImageFormatRgb10a2ui:
		return "Rgb10a2ui";
	case SpvImageFormatRg32ui:
		return "Rg32ui";
	case SpvImageFormatRg16ui:
		return "Rg16ui";
	case SpvImageFormatRg8ui:
		return "Rg8ui";
	case SpvImageFormatR16ui:
		return "R16ui";
	case SpvImageFormatR8ui:
		return "R8ui";
	case SpvImageFormatR64ui:
		return "R64ui";
	case SpvImageFormatR64i:
		return "R64i";

	case SpvImageFormatMax:
		break;
	}
	// unhandled SpvImageFormat enum value
	return "???";
}

std::string ToStringTypeFlags(SpvReflectTypeFlags type_flags)
{
	if (type_flags == SPV_REFLECT_TYPE_FLAG_UNDEFINED)
	{
		return "UNDEFINED";
	}

#define PRINT_AND_CLEAR_TYPE_FLAG(stream, flags, bit)                                                                  \
	if (((flags) & (SPV_REFLECT_TYPE_FLAG_##bit)) == (SPV_REFLECT_TYPE_FLAG_##bit))                                    \
	{                                                                                                                  \
		stream << #bit << " ";                                                                                         \
		flags ^= SPV_REFLECT_TYPE_FLAG_##bit;                                                                          \
	}
	std::stringstream sstream;
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, ARRAY);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, STRUCT);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, EXTERNAL_MASK);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, EXTERNAL_BLOCK);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, EXTERNAL_SAMPLED_IMAGE);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, EXTERNAL_SAMPLER);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, EXTERNAL_IMAGE);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, MATRIX);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, VECTOR);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, FLOAT);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, INT);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, BOOL);
	PRINT_AND_CLEAR_TYPE_FLAG(sstream, type_flags, VOID);
#undef PRINT_AND_CLEAR_TYPE_FLAG
	if (type_flags != 0)
	{
		// Unhandled SpvReflectTypeFlags bit
		sstream << "???";
	}
	return sstream.str();
}

std::string ToStringDecorationFlags(SpvReflectDecorationFlags decoration_flags)
{
	if (decoration_flags == SPV_REFLECT_DECORATION_NONE)
	{
		return "NONE";
	}

#define PRINT_AND_CLEAR_DECORATION_FLAG(stream, flags, bit)                                                            \
	if (((flags) & (SPV_REFLECT_DECORATION_##bit)) == (SPV_REFLECT_DECORATION_##bit))                                  \
	{                                                                                                                  \
		stream << #bit << " ";                                                                                         \
		flags ^= SPV_REFLECT_DECORATION_##bit;                                                                         \
	}
	std::stringstream sstream;
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, NON_WRITABLE);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, FLAT);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, NOPERSPECTIVE);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, BUILT_IN);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, COLUMN_MAJOR);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, ROW_MAJOR);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, BUFFER_BLOCK);
	PRINT_AND_CLEAR_DECORATION_FLAG(sstream, decoration_flags, BLOCK);
#undef PRINT_AND_CLEAR_DECORATION_FLAG
	if (decoration_flags != 0)
	{
		// Unhandled SpvReflectDecorationFlags bit
		sstream << "???";
	}
	return sstream.str();
}

std::string ToStringFormat(SpvReflectFormat fmt)
{
	switch (fmt)
	{
	case SPV_REFLECT_FORMAT_UNDEFINED:
		return "VK_FORMAT_UNDEFINED";
	case SPV_REFLECT_FORMAT_R32_UINT:
		return "VK_FORMAT_R32_UINT";
	case SPV_REFLECT_FORMAT_R32_SINT:
		return "VK_FORMAT_R32_SINT";
	case SPV_REFLECT_FORMAT_R32_SFLOAT:
		return "VK_FORMAT_R32_SFLOAT";
	case SPV_REFLECT_FORMAT_R32G32_UINT:
		return "VK_FORMAT_R32G32_UINT";
	case SPV_REFLECT_FORMAT_R32G32_SINT:
		return "VK_FORMAT_R32G32_SINT";
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
		return "VK_FORMAT_R32G32_SFLOAT";
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:
		return "VK_FORMAT_R32G32B32_UINT";
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:
		return "VK_FORMAT_R32G32B32_SINT";
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
		return "VK_FORMAT_R32G32B32_SFLOAT";
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
		return "VK_FORMAT_R32G32B32A32_UINT";
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
		return "VK_FORMAT_R32G32B32A32_SINT";
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
		return "VK_FORMAT_R32G32B32A32_SFLOAT";
	case SPV_REFLECT_FORMAT_R64_UINT:
		return "VK_FORMAT_R64_UINT";
	case SPV_REFLECT_FORMAT_R64_SINT:
		return "VK_FORMAT_R64_SINT";
	case SPV_REFLECT_FORMAT_R64_SFLOAT:
		return "VK_FORMAT_R64_SFLOAT";
	case SPV_REFLECT_FORMAT_R64G64_UINT:
		return "VK_FORMAT_R64G64_UINT";
	case SPV_REFLECT_FORMAT_R64G64_SINT:
		return "VK_FORMAT_R64G64_SINT";
	case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
		return "VK_FORMAT_R64G64_SFLOAT";
	case SPV_REFLECT_FORMAT_R64G64B64_UINT:
		return "VK_FORMAT_R64G64B64_UINT";
	case SPV_REFLECT_FORMAT_R64G64B64_SINT:
		return "VK_FORMAT_R64G64B64_SINT";
	case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
		return "VK_FORMAT_R64G64B64_SFLOAT";
	case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
		return "VK_FORMAT_R64G64B64A64_UINT";
	case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
		return "VK_FORMAT_R64G64B64A64_SINT";
	case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
		return "VK_FORMAT_R64G64B64A64_SFLOAT";
	}
	// unhandled SpvReflectFormat enum value
	return "VK_FORMAT_???";
}

static std::string ToStringScalarType(const SpvReflectTypeDescription& type)
{
	switch (type.op)
	{
	case SpvOpTypeVoid:
	{
		return "void";
		break;
	}
	case SpvOpTypeBool:
	{
		return "bool";
		break;
	}
	case SpvOpTypeInt:
	{
		if (type.traits.numeric.scalar.signedness)
			return "int";
		else
			return "uint";
	}
	case SpvOpTypeFloat:
	{
		switch (type.traits.numeric.scalar.width)
		{
		case 32:
			return "float";
		case 64:
			return "double";
		default:
			break;
		}
	}
	case SpvOpTypeStruct:
	{
		return "struct";
	}
	default:
	{
		break;
	}
	}
	return "";
}

static std::string ToStringGlslType(const SpvReflectTypeDescription& type)
{
	switch (type.op)
	{
	case SpvOpTypeVector:
	{
		switch (type.traits.numeric.scalar.width)
		{
		case 32:
		{
			switch (type.traits.numeric.vector.component_count)
			{
			case 2:
				return "vec2";
			case 3:
				return "vec3";
			case 4:
				return "vec4";
			}
		}
		break;

		case 64:
		{
			switch (type.traits.numeric.vector.component_count)
			{
			case 2:
				return "dvec2";
			case 3:
				return "dvec3";
			case 4:
				return "dvec4";
			}
		}
		break;
		}
	}
	break;
	default:
		break;
	}
	return ToStringScalarType(type);
}

static std::string ToStringHlslType(const SpvReflectTypeDescription& type)
{
	switch (type.op)
	{
	case SpvOpTypeVector:
	{
		switch (type.traits.numeric.scalar.width)
		{
		case 32:
		{
			switch (type.traits.numeric.vector.component_count)
			{
			case 2:
				return "float2";
			case 3:
				return "float3";
			case 4:
				return "float4";
			}
		}
		break;

		case 64:
		{
			switch (type.traits.numeric.vector.component_count)
			{
			case 2:
				return "double2";
			case 3:
				return "double3";
			case 4:
				return "double4";
			}
		}
		break;
		}
	}
	break;

	default:
		break;
	}
	return ToStringScalarType(type);
}

std::string ToStringType(SpvSourceLanguage src_lang, const SpvReflectTypeDescription& type)
{
	if (src_lang == SpvSourceLanguageHLSL)
	{
		return ToStringHlslType(type);
	}

	return ToStringGlslType(type);
}

std::string ToStringComponentType(const SpvReflectTypeDescription& type, uint32_t member_decoration_flags)
{
	uint32_t masked_type = type.type_flags & 0xF;
	if (masked_type == 0)
	{
		return "";
	}

	std::stringstream ss;

	if (type.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
	{
		if (member_decoration_flags & SPV_REFLECT_DECORATION_COLUMN_MAJOR)
		{
			ss << "column_major"
			   << " ";
		}
		else if (member_decoration_flags & SPV_REFLECT_DECORATION_ROW_MAJOR)
		{
			ss << "row_major"
			   << " ";
		}
	}

	switch (masked_type)
	{
	default:
		assert(false && "unsupported component type");
		break;
	case SPV_REFLECT_TYPE_FLAG_BOOL:
		ss << "bool";
		break;
	case SPV_REFLECT_TYPE_FLAG_INT:
		ss << (type.traits.numeric.scalar.signedness ? "int" : "uint");
		break;
	case SPV_REFLECT_TYPE_FLAG_FLOAT:
		ss << "float";
		break;
	}

	if (type.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
	{
		ss << type.traits.numeric.matrix.row_count;
		ss << "x";
		ss << type.traits.numeric.matrix.column_count;
	}
	else if (type.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
	{
		ss << type.traits.numeric.vector.component_count;
	}

	return ss.str();
}
