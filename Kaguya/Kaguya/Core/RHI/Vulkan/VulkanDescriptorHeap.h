#pragma once
#include "VulkanCommon.h"

class VulkanDescriptorHeap : public VulkanDeviceChild
{
public:
	VulkanDescriptorHeap() noexcept = default;
	explicit VulkanDescriptorHeap(VulkanDevice* Parent, const DescriptorHeapDesc& Desc);

	void Destroy();

	[[nodiscard]] auto AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle;

	void UpdateDescriptor(const DescriptorHandle& Handle) const;

	struct IndexPool
	{
		IndexPool() = default;
		IndexPool(size_t NumIndices)
		{
			Elements.resize(NumIndices);
			Reset();
		}

		auto& operator[](size_t Index) { return Elements[Index]; }

		const auto& operator[](size_t Index) const { return Elements[Index]; }

		void Reset()
		{
			FreeStart		  = 0;
			NumActiveElements = 0;
			for (size_t i = 0; i < Elements.size(); ++i)
			{
				Elements[i] = i + 1;
			}
		}

		// Removes the first element from the free list and returns its index
		size_t Allocate()
		{
			assert(NumActiveElements < Elements.size() && "Consider increasing the size of the pool");
			NumActiveElements++;
			size_t index = FreeStart;
			FreeStart	 = Elements[index];
			return index;
		}

		void Release(size_t Index)
		{
			NumActiveElements--;
			Elements[Index] = FreeStart;
			FreeStart		= Index;
		}

		std::vector<size_t> Elements;
		size_t				FreeStart		  = 0;
		size_t				NumActiveElements = 0;
	};

	VkDescriptorSetLayout  DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool	   DescriptorPool	   = VK_NULL_HANDLE;
	VkDescriptorSet		   DescriptorSet	   = VK_NULL_HANDLE;
	std::vector<IndexPool> IndexPoolArray;
};
