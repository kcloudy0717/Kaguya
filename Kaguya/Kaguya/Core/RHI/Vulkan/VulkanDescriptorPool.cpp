#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(IRHIDevice* Parent, const DescriptorPoolDesc& Desc)
	: IRHIDescriptorPool(Parent)
{
	VkDescriptorPoolSize PoolSizes[] = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Desc.NumConstantBufferDescriptors },
										 { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, Desc.NumShaderResourceDescriptors },
										 { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, Desc.NumUnorderedAccessDescriptors },
										 { VK_DESCRIPTOR_TYPE_SAMPLER, Desc.NumSamplers } };

	auto DescriptorPoolCreateInfo		   = VkStruct<VkDescriptorPoolCreateInfo>();
	DescriptorPoolCreateInfo.flags		   = 0;
	DescriptorPoolCreateInfo.maxSets	   = 1;
	DescriptorPoolCreateInfo.poolSizeCount = 4;
	DescriptorPoolCreateInfo.pPoolSizes	   = PoolSizes;
	VERIFY_VULKAN_API(vkCreateDescriptorPool(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorPoolCreateInfo,
		nullptr,
		&DescriptorPool));

	const VkDescriptorSetLayout DescriptorSetLayouts[] = {
		Desc.DescriptorTable->As<VulkanDescriptorTable>()->GetApiHandle()
	};
	auto DescriptorSetAllocateInfo				 = VkStruct<VkDescriptorSetAllocateInfo>();
	DescriptorSetAllocateInfo.descriptorPool	 = this->GetApiHandle();
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pSetLayouts		 = DescriptorSetLayouts;
	VERIFY_VULKAN_API(vkAllocateDescriptorSets(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetAllocateInfo,
		&DescriptorSet));

	IndexPoolArray[0] = IndexPool(Desc.NumConstantBufferDescriptors);
	IndexPoolArray[1] = IndexPool(Desc.NumShaderResourceDescriptors);
	IndexPoolArray[2] = IndexPool(Desc.NumUnorderedAccessDescriptors);
	IndexPoolArray[3] = IndexPool(Desc.NumSamplers);
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	if (Parent && DescriptorPool)
	{
		vkDestroyDescriptorPool(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(DescriptorPool, {}), nullptr);
	}
}

auto VulkanDescriptorPool::AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle
{
	const UINT Index = IndexPoolArray[static_cast<size_t>(DescriptorType)].Allocate();

	return { .Resource = nullptr, .Type = DescriptorType, .Index = Index };
}

VkSamplerCreateInfo sampler_create_info(
	VkFilter			 filters,
	VkSamplerAddressMode samplerAddressMode /*= VK_SAMPLER_ADDRESS_MODE_REPEAT*/)
{
	VkSamplerCreateInfo info = {};
	info.sType				 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext				 = nullptr;

	info.magFilter	  = filters;
	info.minFilter	  = filters;
	info.addressModeU = samplerAddressMode;
	info.addressModeV = samplerAddressMode;
	info.addressModeW = samplerAddressMode;

	return info;
}

void VulkanDescriptorPool::UpdateDescriptor(const DescriptorHandle& Handle) const
{
	VkDescriptorImageInfo  DescriptorImageInfo	= {};
	VkDescriptorBufferInfo DescriptorBufferInfo = {};
	if (Handle.Type == EDescriptorType::ConstantBuffer)
	{
		assert(Handle.Resource);

		VulkanBuffer*			 Buffer = Handle.Resource->As<VulkanBuffer>();
		const VkBufferCreateInfo Desc	= Buffer->GetDesc();

		DescriptorBufferInfo.buffer = Buffer->GetApiHandle();
		DescriptorBufferInfo.offset = 0;
		DescriptorBufferInfo.range	= Desc.size;
	}
	if (Handle.Type == EDescriptorType::Texture)
	{
		assert(Handle.Resource);

		VulkanTexture* Texture = Handle.Resource->As<VulkanTexture>();

		DescriptorImageInfo.sampler		= nullptr;
		DescriptorImageInfo.imageView	= Texture->GetImageView();
		DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	if (Handle.Type == EDescriptorType::Sampler)
	{
		VkSamplerCreateInfo samplerInfo = sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		VkSampler			blockySampler;
		vkCreateSampler(Parent->As<VulkanDevice>()->GetVkDevice(), &samplerInfo, nullptr, &blockySampler);

		DescriptorImageInfo.sampler = blockySampler;
	}

	auto WriteDescriptorSet			   = VkStruct<VkWriteDescriptorSet>();
	WriteDescriptorSet.dstSet		   = DescriptorSet;
	WriteDescriptorSet.dstBinding	   = static_cast<size_t>(Handle.Type);
	WriteDescriptorSet.dstArrayElement = Handle.Index;
	WriteDescriptorSet.descriptorCount = 1;
	WriteDescriptorSet.descriptorType  = ToVkDescriptorType(Handle.Type);
	if (Handle.Type == EDescriptorType::ConstantBuffer)
	{
		WriteDescriptorSet.pBufferInfo = &DescriptorBufferInfo;
	}
	if (Handle.Type == EDescriptorType::Texture || Handle.Type == EDescriptorType::Sampler)
	{
		WriteDescriptorSet.pImageInfo = &DescriptorImageInfo;
	}

	vkUpdateDescriptorSets(Parent->As<VulkanDevice>()->GetVkDevice(), 1, &WriteDescriptorSet, 0, nullptr);
}
