#include "PhysicsCore.h"

namespace PhysicsCore
{
PhysicsDevice* Device = nullptr;
}

PhysicsCoreInitializer::PhysicsCoreInitializer(const PhysicsSettings& Settings)
{
	// Only allow one instance
	assert(!PhysicsCore::Device);

	Device = std::make_unique<PhysicsDevice>();
	Device->Initialize(Settings);
	PhysicsCore::Device = Device.get();
}

PhysicsCoreInitializer::~PhysicsCoreInitializer()
{
}
