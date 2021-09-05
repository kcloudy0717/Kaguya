#include "VulkanDescriptorHeap.h"

VulkanResourceDescriptorHeap::VulkanResourceDescriptorHeap(VulkanDevice* Parent, const DescriptorHeapDesc& Desc)
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

auto VulkanResourceDescriptorHeap::AllocateDescriptorHandle(EDescriptorType DescriptorType) -> VulkanDescriptorHandle
{
	size_t	   PoolIndex = DescriptorType != EDescriptorType::Sampler ? static_cast<size_t>(DescriptorType) : 0;
	const UINT Index	 = IndexPoolArray[PoolIndex].Allocate();
	return { .Resource = nullptr, .Type = DescriptorType, .Index = Index };
}

void VulkanResourceDescriptorHeap::UpdateDescriptor(const VulkanDescriptorHandle& Handle)
{
	VkDescriptorImageInfo  DescriptorImageInfo	= {};
	VkDescriptorBufferInfo DescriptorBufferInfo = {};
	if (Handle.Type == EDescriptorType::ConstantBuffer)
	{
		assert(Handle.Resource);

		VulkanBuffer*	   ApiBuffer = Handle.Resource->As<VulkanBuffer>();
		VkBufferCreateInfo Desc		 = ApiBuffer->GetDesc();

		DescriptorBufferInfo.buffer = ApiBuffer->GetApiHandle();
		DescriptorBufferInfo.offset = 0;
		DescriptorBufferInfo.range	= Desc.size;
	}
	if (Handle.Type == EDescriptorType::Texture)
	{
		assert(Handle.Resource);

		VulkanTexture* ApiTexture = Handle.Resource->As<VulkanTexture>();

		DescriptorImageInfo.sampler		= nullptr;
		DescriptorImageInfo.imageView	= ApiTexture->GetImageView();
		DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
