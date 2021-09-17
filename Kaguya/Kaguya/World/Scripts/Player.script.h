#pragma once
#include "../Entity.h"
#include "../Components.h"
#include <Physics/PhysXUtility.h>

struct Jump
{
	void startJump(physx::PxF32 v0)
	{
		if (IsJumping)
			return;
		JumpTime  = 0.0f;
		V0		  = v0;
		IsJumping = true;
	}

	void stopJump()
	{
		if (!IsJumping)
			return;
		IsJumping = false;
	}

	physx::PxF32 getHeight(physx::PxF32 elapsedTime)
	{
		static physx::PxF32 gJumpGravity = -50.0f;

		if (!IsJumping)
			return 0.0f;
		JumpTime += elapsedTime;
		const physx::PxF32 h = gJumpGravity * JumpTime * JumpTime + V0 * JumpTime;
		return h * elapsedTime;
	}

	physx::PxF32 V0		   = 0.0f;
	physx::PxF32 JumpTime  = 0.0f;
	bool		 IsJumping = false;
};

inline static bool canJump(physx::PxController* controller)
{
	physx::PxControllerState cctState;
	controller->getState(cctState);
	return (cctState.collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN) != 0;
}

class PlayerScript : public ScriptableEntity
{
public:
	void OnCreate() override;

	void OnDestroy() override;

	void OnUpdate(float DeltaTime) override;

	static constexpr float Gravity = -9.81f;

	float MoveSpeed;
	float StrafeSpeed;
	float MouseSensitivityX;
	float MouseSensitivityY;

	bool Momentum;

	float LastForward;
	float LastStrafe;
	float LastAscent;

	Jump Jump;
};
