#pragma once
#include "VulkanCommon.h"

class VulkanDescriptorHeap
{
public:
	void Initialize(const std::vector<VkDescriptorPoolSize>& PoolSizes);

	UINT Allocate(size_t PoolIndex);

	void Release(size_t PoolIndex, UINT Index);

private:
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
		size_t				FreeStart;
		size_t				NumActiveElements;
	};

private:
	std::vector<IndexPool> IndexPools;
};

class VulkanDescriptorPool final : public IRHIDescriptorPool
{
public:
	[[nodiscard]] auto AllocateDescriptorHandle(EDescriptorType DescriptorType) -> DescriptorHandle override;

	void UpdateDescriptor(const DescriptorHandle& Handle) const;

public:
	explicit VulkanDescriptorPool(IRHIDevice* Parent) noexcept
		: IRHIDescriptorPool(Parent)
	{
	}
	~VulkanDescriptorPool() override;

	[[nodiscard]] auto GetApiHandle() const noexcept -> VkDescriptorPool { return DescriptorPool; }
	[[nodiscard]] auto GetDescriptorSet() const noexcept -> VkDescriptorSet { return DescriptorSet; }

	void Initialize(std::vector<VkDescriptorPoolSize> PoolSizes, VkDescriptorSetLayout DescriptorSetLayout);

private:
	VkDescriptorPoolCreateInfo Desc			  = {};
	VkDescriptorPool		   DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet			   DescriptorSet  = VK_NULL_HANDLE;

	VulkanDescriptorHeap DescriptorHeap;
};
