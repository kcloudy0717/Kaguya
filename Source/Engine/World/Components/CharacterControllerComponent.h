#pragma once

class CharacterControllerComponent
{
public:
	CharacterControllerComponent() noexcept = default;
	explicit CharacterControllerComponent(physx::PxController* Controller)
		: Controller(Controller)
	{
	}

	physx::PxController* Controller = nullptr;
};
