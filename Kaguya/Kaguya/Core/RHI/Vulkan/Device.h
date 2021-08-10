#pragma once
#include "VulkanCommon.h"
#include "CommandQueue.h"

class Device : public AdapterChild
{
public:
	Device(Adapter* Parent);
	~Device() {}

	void Initialize(uint32_t GraphicsQueueFamilyIndex);
	void Destroy();

	[[nodiscard]] auto GetDevice() const -> VkDevice;
	[[nodiscard]] auto GetGraphicsQueue() -> CommandQueue* { return &GraphicsQueue; }

private:
	CommandQueue GraphicsQueue;
};
