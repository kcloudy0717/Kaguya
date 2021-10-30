#pragma once
#include "PhysicsDevice.h"

namespace PhysicsCore
{
extern PhysicsDevice* Device;
} // namespace PhysicsCore

class PhysicsCoreInitializer
{
public:
	PhysicsCoreInitializer(const PhysicsSettings& Settings);
	~PhysicsCoreInitializer();

private:
	std::unique_ptr<PhysicsDevice> Device;
};
