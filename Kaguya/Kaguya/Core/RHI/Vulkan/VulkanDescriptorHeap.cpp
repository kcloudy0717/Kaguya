#include "VulkanDescriptorHeap.h"

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

VulkanDescriptorHeap::VulkanDescriptorHeap(VulkanDevice* Parent, const DescriptorHeapDesc& Desc)
	: VulkanDeviceChild(Parent)
{
	std::vector<VkDescriptorPoolSize>		  PoolSizes;
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	std::vector<VkDescriptorBindingFlagsEXT>  BindingFlags;

	if (Desc.Type == DescriptorHeapType::Resource)
	{
		PoolSizes = { // CBV
					  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Desc.Resource.NumConstantBufferDescriptors },
					  // SRV
					  { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, Desc.Resource.NumTextureDescriptors },
					  // UAV
					  { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, Desc.Resource.NumRWTextureDescriptors }
		};
		IndexPoolArray.resize(PoolSizes.size());
		IndexPoolArray[0] = IndexPool(Desc.Resource.NumConstantBufferDescriptors);
		IndexPoolArray[1] = IndexPool(Desc.Resource.NumTextureDescriptors);
		IndexPoolArray[2] = IndexPool(Desc.Resource.NumRWTextureDescriptors);
	}
	else if (Desc.Type == DescriptorHeapType::Sampler)
	{
		PoolSizes = {
			// Sampler
			{ VK_DESCRIPTOR_TYPE_SAMPLER, Desc.Sampler.NumDescriptors },
		};
		IndexPoolArray.resize(PoolSizes.size());
		IndexPoolArray[0] = IndexPool(Desc.Sampler.NumDescriptors);
	}

	Bindings.reserve(PoolSizes.size());
	BindingFlags.reserve(PoolSizes.size());

	for (size_t i = 0; i < PoolSizes.size(); ++i)
	{
		VkDescriptorSetLayoutBinding& VkBinding = Bindings.emplace_back();
		VkBinding.binding						= static_cast<uint32_t>(i);
		VkBinding.descriptorType				= PoolSizes[i].type;
		VkBinding.descriptorCount				= PoolSizes[i].descriptorCount;
		VkBinding.stageFlags					= VK_SHADER_STAGE_ALL;
		BindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
	}

	auto BindingFlagsInfo				 = VkStruct<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>();
	BindingFlagsInfo.bindingCount		 = static_cast<uint32_t>(BindingFlags.size());
	BindingFlagsInfo.pBindingFlags		 = BindingFlags.data();
	auto DescriptorSetLayoutDesc		 = VkStruct<VkDescriptorSetLayoutCreateInfo>();
	DescriptorSetLayoutDesc.pNext		 = &BindingFlagsInfo;
	DescriptorSetLayoutDesc.flags		 = 0;
	DescriptorSetLayoutDesc.bindingCount = static_cast<uint32_t>(Bindings.size());
	DescriptorSetLayoutDesc.pBindings	 = Bindings.data();
	VERIFY_VULKAN_API(vkCreateDescriptorSetLayout(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetLayoutDesc,
		nullptr,
		&DescriptorSetLayout));

	auto DescriptorPoolCreateInfo		   = VkStruct<VkDescriptorPoolCreateInfo>();
	DescriptorPoolCreateInfo.flags		   = 0;
	DescriptorPoolCreateInfo.maxSets	   = 1;
	DescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
	DescriptorPoolCreateInfo.pPoolSizes	   = PoolSizes.data();
	VERIFY_VULKAN_API(vkCreateDescriptorPool(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorPoolCreateInfo,
		nullptr,
		&DescriptorPool));

	auto DescriptorSetAllocateInfo				 = VkStruct<VkDescriptorSetAllocateInfo>();
	DescriptorSetAllocateInfo.descriptorPool	 = DescriptorPool;
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pSetLayouts		 = &DescriptorSetLayout;
	VERIFY_VULKAN_API(vkAllocateDescriptorSets(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetAllocateInfo,
		&DescriptorSet));
}

void VulkanDescriptorHeap::Destroy()
{
	for (auto Sampler : SamplerTable | std::views::values)
	{
		vkDestroySampler(Parent->As<VulkanDevice>()->GetVkDevice(), Sampler, nullptr);
	}

	if (DescriptorPool)
	{
		vkDestroyDescriptorPool(Parent->As<VulkanDevice>()->GetVkDevice(), std::exchange(DescriptorPool, {}), nullptr);
	}
	if (DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(
			Parent->As<VulkanDevice>()->GetVkDevice(),
			std::exchange(DescriptorSetLayout, {}),
			nullptr);
	}
}

auto VulkanDescriptorHeap::AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle
{
	size_t	   PoolIndex = DescriptorType != EDescriptorType::Sampler ? static_cast<size_t>(DescriptorType) : 0;
	const UINT Index	 = IndexPoolArray[PoolIndex].Allocate();
	return { .Resource = nullptr, .Type = DescriptorType, .Index = Index };
}

void VulkanDescriptorHeap::UpdateDescriptor(const DescriptorHandle& Handle)
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
		UINT64				Hash = CityHash64(reinterpret_cast<char*>(&samplerInfo), sizeof(VkSamplerCreateInfo));
		if (auto iter = SamplerTable.find(Hash); iter != SamplerTable.end())
		{
			return;
		}

		VkSampler blockySampler;
		vkCreateSampler(Parent->As<VulkanDevice>()->GetVkDevice(), &samplerInfo, nullptr, &blockySampler);

		SamplerTable[Hash] = blockySampler;

		DescriptorImageInfo.sampler = blockySampler;
	}

	auto WriteDescriptorSet			   = VkStruct<VkWriteDescriptorSet>();
	WriteDescriptorSet.dstSet		   = DescriptorSet;
	WriteDescriptorSet.dstBinding	   = Handle.Type != EDescriptorType::Sampler ? static_cast<size_t>(Handle.Type) : 0;
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
