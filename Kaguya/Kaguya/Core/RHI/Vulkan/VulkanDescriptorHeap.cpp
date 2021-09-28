#include "VulkanDescriptorHeap.h"

VulkanResourceDescriptorHeap::VulkanResourceDescriptorHeap(VulkanDevice* Parent, const ResourceDescriptorHeapDesc& Desc)
	: VulkanDeviceChild(Parent)
{
	VkDescriptorPoolSize PoolSizes[] = { // SRV
										 { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, Desc.NumTextureDescriptors },
										 // UAV
										 { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, Desc.NumRWTextureDescriptors }
	};
	std::vector<VkDescriptorSetLayoutBinding> Bindings;
	std::vector<VkDescriptorBindingFlagsEXT>  BindingFlags;

	IndexPoolArray.resize(std::size(PoolSizes));
	IndexPoolArray[0] = DescriptorIndexPool(Desc.NumTextureDescriptors);
	IndexPoolArray[1] = DescriptorIndexPool(Desc.NumRWTextureDescriptors);

	Bindings.reserve(std::size(PoolSizes));
	BindingFlags.reserve(std::size(PoolSizes));

	for (size_t i = 0; i < std::size(PoolSizes); ++i)
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
	DescriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(std::size(PoolSizes));
	DescriptorPoolCreateInfo.pPoolSizes	   = PoolSizes;
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

void VulkanResourceDescriptorHeap::Destroy()
{
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

void VulkanResourceDescriptorHeap::Allocate(UINT PoolIndex, UINT& Index)
{
	Index = static_cast<UINT>(IndexPoolArray[PoolIndex].Allocate());
}

void VulkanResourceDescriptorHeap::Release(UINT PoolIndex, UINT Index)
{
	IndexPoolArray[PoolIndex].Release(static_cast<size_t>(Index));
}

VulkanSamplerDescriptorHeap::VulkanSamplerDescriptorHeap(VulkanDevice* Parent, UINT NumDescriptors)
	: VulkanDeviceChild(Parent)
	, IndexPool(NumDescriptors)
{
	VkDescriptorPoolSize		 PoolSize	 = { VK_DESCRIPTOR_TYPE_SAMPLER, NumDescriptors };
	VkDescriptorSetLayoutBinding Binding	 = { .binding		  = 0,
											 .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
											 .descriptorCount = NumDescriptors,
											 .stageFlags	  = VK_SHADER_STAGE_ALL };
	VkDescriptorBindingFlagsEXT	 BindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	auto BindingFlagsInfo				 = VkStruct<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>();
	BindingFlagsInfo.bindingCount		 = 1;
	BindingFlagsInfo.pBindingFlags		 = &BindingFlag;
	auto DescriptorSetLayoutDesc		 = VkStruct<VkDescriptorSetLayoutCreateInfo>();
	DescriptorSetLayoutDesc.pNext		 = &BindingFlagsInfo;
	DescriptorSetLayoutDesc.flags		 = 0;
	DescriptorSetLayoutDesc.bindingCount = 1;
	DescriptorSetLayoutDesc.pBindings	 = &Binding;
	VERIFY_VULKAN_API(vkCreateDescriptorSetLayout(
		Parent->As<VulkanDevice>()->GetVkDevice(),
		&DescriptorSetLayoutDesc,
		nullptr,
		&DescriptorSetLayout));

	auto DescriptorPoolCreateInfo		   = VkStruct<VkDescriptorPoolCreateInfo>();
	DescriptorPoolCreateInfo.flags		   = 0;
	DescriptorPoolCreateInfo.maxSets	   = 1;
	DescriptorPoolCreateInfo.poolSizeCount = 1;
	DescriptorPoolCreateInfo.pPoolSizes	   = &PoolSize;
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

void VulkanSamplerDescriptorHeap::Destroy()
{
	for (auto Sampler : SamplerTable | std::views::values)
	{
		vkDestroySampler(Parent->GetVkDevice(), Sampler, nullptr);
	}

	if (DescriptorPool)
	{
		vkDestroyDescriptorPool(Parent->GetVkDevice(), std::exchange(DescriptorPool, {}), nullptr);
	}
	if (DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(Parent->GetVkDevice(), std::exchange(DescriptorSetLayout, {}), nullptr);
	}
}

void VulkanSamplerDescriptorHeap::Allocate(UINT& Index)
{
	Index = static_cast<UINT>(IndexPool.Allocate());
}

void VulkanSamplerDescriptorHeap::Release(UINT Index)
{
	IndexPool.Release(static_cast<size_t>(Index));
}
