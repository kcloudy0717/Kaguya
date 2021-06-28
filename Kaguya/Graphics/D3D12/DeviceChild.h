#pragma once

class Device;

class DeviceChild
{
public:
	DeviceChild(Device* Parent = nullptr) noexcept
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
