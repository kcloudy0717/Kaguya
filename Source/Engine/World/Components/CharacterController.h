#pragma once

struct CharacterController : Component
{
	CharacterController() noexcept = default;
	CharacterController(physx::PxController* Controller)
		: Controller(Controller)
	{
	}

	physx::PxController* Controller = nullptr;
};
