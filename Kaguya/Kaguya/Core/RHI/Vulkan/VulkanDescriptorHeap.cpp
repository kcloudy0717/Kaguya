#include "VulkanDescriptorHeap.h"

VkDescriptorType ToVkDescriptorType(EDescriptorType DescriptorType)
{
	switch (DescriptorType)
	{
	case EDescriptorType::ConstantBuffer:
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case EDescriptorType::Texture:
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case EDescriptorType::RWTexture:
		return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case EDescriptorType::Sampler:
		return VK_DESCRIPTOR_TYPE_SAMPLER;
	default:
		break;
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

auto VulkanDescriptorPool::AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle
{
	const UINT Index = DescriptorHeap.Allocate(static_cast<size_t>(DescriptorType));

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
	if (Handle.Type == EDescriptorType::Texture)
	{
		WriteDescriptorSet.pImageInfo = &DescriptorImageInfo;
	}
	if (Handle.Type == EDescriptorType::Sampler)
	{
		WriteDescriptorSet.pImageInfo = &DescriptorImageInfo;
	}

	vkUpdateDescriptorSets(Parent->As<VulkanDevice>()->GetVkDevice(), 1, &WriteDescriptorSet, 0, nullptr);
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	if (Parent && DescriptorPool)
	{
		vkDestroyDescriptorPool(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(DescriptorPool, {}), nullptr);
	}
}

void VulkanDescriptorPool::Initialize(
	std::vector<VkDescriptorPoolSize> PoolSizes,
	VkDescriptorSetLayout			  DescriptorSetLayout)
{
	Desc			   = VkStruct<VkDescriptorPoolCreateInfo>();
	Desc.flags		   = 0;
	Desc.maxSets	   = 1;
	Desc.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	Desc.pPoolSizes	   = PoolSizes.data();
	VERIFY_VULKAN_API(
		vkCreateDescriptorPool(Parent->As<VulkanDevice>()->GetVkDevice(), &Desc, nullptr, &DescriptorPool));

	auto DescriptorSetAllocateInfo				 = VkStruct<VkDescriptorSetAllocateInfo>();
	DescriptorSetAllocateInfo.descriptorPool	 = this->GetApiHandle();
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pSetLayouts		 = &DescriptorSetLayout;
	VERIFY_VULKAN_API(vkAllocateDescriptorSets(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetAllocateInfo,
		&DescriptorSet));

	DescriptorHeap.Initialize(PoolSizes);
}

void VulkanDescriptorHeap::Initialize(const std::vector<VkDescriptorPoolSize>& PoolSizes)
{
	IndexPools.resize(PoolSizes.size());
	for (size_t i = 0; i < IndexPools.size(); ++i)
	{
		IndexPools[i] = IndexPool(PoolSizes[i].descriptorCount);
	}
}

UINT VulkanDescriptorHeap::Allocate(size_t PoolIndex)
{
	return static_cast<UINT>(IndexPools[PoolIndex].Allocate());
}

void VulkanDescriptorHeap::Release(size_t PoolIndex, UINT Index)
{
	IndexPools[PoolIndex].Release(static_cast<size_t>(Index));
}
