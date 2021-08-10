#include "Device.h"
#include "Adapter.h"

Device::Device(Adapter* Parent)
	: AdapterChild(Parent)
	, GraphicsQueue(this)
{
}

void Device::Initialize(uint32_t GraphicsQueueFamilyIndex)
{
	GraphicsQueue.Initialize(GraphicsQueueFamilyIndex);
}

void Device::Destroy()
{
	GraphicsQueue.Destroy();
}

auto Device::GetDevice() const -> VkDevice
{
	return GetParentAdapter()->GetVkDevice();
}
