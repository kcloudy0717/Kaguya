#pragma once

class Device;

class DeviceChild
{
public:
	DeviceChild() noexcept
		: Parent(nullptr)
	{
	}

	DeviceChild(Device* Parent) noexcept
		: Parent(Parent)
	{
	}

	Device* GetParentDevice() const { return Parent; }

	void SetParentDevice(Device* Parent)
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	Device* Parent;
};
