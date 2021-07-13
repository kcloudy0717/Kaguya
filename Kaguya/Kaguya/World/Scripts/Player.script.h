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
	void OnCreate() override
	{
		MoveSpeed		  = 10.0f;
		StrafeSpeed		  = 10.0f;
		MouseSensitivityX = 15.0f;
		MouseSensitivityY = 15.0f;

		Momentum = true;

		LastForward = 0.0f;
		LastStrafe	= 0.0f;
		LastAscent	= 0.0f;
	}

	void OnDestroy() override {}

	void OnUpdate(float dt) override
	{
		using namespace DirectX;
		using namespace physx;

		InputHandler& InputHandler = Application::GetInputHandler();
		Mouse&		  Mouse		   = Application::GetMouse();
		Keyboard&	  Keyboard	   = Application::GetKeyboard();

		auto& transform = GetComponent<Transform>();
		auto& camera	= GetComponent<Camera>();
		// auto& controller = GetComponent<CharacterController>();

		bool Fwd = false, Bwd = false, Right = false, Left = false, Up = false, Down = false;
		if (InputHandler.RawInputEnabled)
		{
			Fwd	  = Keyboard.IsPressed('W');
			Bwd	  = Keyboard.IsPressed('S');
			Right = Keyboard.IsPressed('D');
			Left  = Keyboard.IsPressed('A');
			Up	  = Keyboard.IsPressed('E');
			Down  = Keyboard.IsPressed('Q');

			// if (Keyboard.IsPressed(' '))
			//{
			//	if (canJump(controller.Controller))
			//	{
			//		Jump.startJump(30.0f);
			//	}
			//}
		}

		auto	 v = camera.pTransform->Forward();
		XMFLOAT3 viewDir;
		XMStoreFloat3(&viewDir, v);

		PxVec3 disp;

		const PxF32 heightDelta = Jump.getHeight(dt);
		float		dy			= Gravity * dt;
		if (heightDelta != 0.0f)
		{
			dy = heightDelta;
		}

		PxVec3 targetKeyDisplacement(0);

		PxVec3 forward = ToPxVec3(viewDir);
		forward.y	   = 0;
		forward.normalize();
		PxVec3 up	 = PxVec3(0, 1, 0);
		PxVec3 right = up.cross(forward);

		{
			if (InputHandler.RawInputEnabled)
			{
				float z = MoveSpeed * ((Fwd * dt) + (Bwd * -dt));
				float x = StrafeSpeed * ((Right * dt) + (Left * -dt));
				float y = StrafeSpeed * ((Up * dt) + (Down * -dt));

				if (Momentum)
				{
					ApplyMomentum(LastForward, z, dt);
					ApplyMomentum(LastStrafe, x, dt);
					ApplyMomentum(LastAscent, y, dt);
				}

				camera.Translate(x, y, z);

				targetKeyDisplacement += z * forward;
				targetKeyDisplacement += x * right;

				/*if (Keyboard.IsPressed('W'))	targetKeyDisplacement += m_MoveSpeed * forward;
				if (Keyboard.IsPressed('S'))	targetKeyDisplacement -= m_MoveSpeed * forward;

				if (Keyboard.IsPressed('A'))	targetKeyDisplacement += m_StrafeSpeed * right;
				if (Keyboard.IsPressed('D'))	targetKeyDisplacement -= m_StrafeSpeed * right;*/

				while (const auto e = Mouse.ReadRawInput())
				{
					camera.Rotate(e->Y * dt * MouseSensitivityY, e->X * dt * MouseSensitivityX, 0.0f);
				}
			}

			// targetKeyDisplacement *= mKeyShiftDown ? mRunningSpeed : mWalkingSpeed;
			// targetKeyDisplacement *= dt;
		}

		disp   = targetKeyDisplacement;
		disp.y = dy;

		// PxControllerFilters				  filters;
		// physx::PxControllerCollisionFlags flags = controller.Controller->move(disp, 0.0f, dt, filters);
		// if (flags & PxControllerCollisionFlag::eCOLLISION_DOWN)
		//{
		//	Jump.stopJump();
		//}

		// const auto& p			  = controller.Controller->getFootPosition();
		// camera.Transform.Position = XMFLOAT3(float(p.x), float(p.y + 1.75f), float(p.z));

		// if (InputHandler.RawInputEnabled)
		//{
		//	float forward = m_MoveSpeed * (
		//		(Keyboard.IsPressed('W') * dt) +
		//		(Keyboard.IsPressed('S') * -dt));
		//	float strafe = m_StrafeSpeed * (
		//		(Keyboard.IsPressed('D') * dt) +
		//		(Keyboard.IsPressed('A') * -dt));
		//	float ascent = m_StrafeSpeed * (
		//		(Keyboard.IsPressed('E') * dt) +
		//		(Keyboard.IsPressed('Q') * -dt));
		//
		//	if (m_Momentum)
		//	{
		//		ApplyMomentum(m_LastForward, forward, dt);
		//		ApplyMomentum(m_LastStrafe, strafe, dt);
		//		ApplyMomentum(m_LastAscent, ascent, dt);
		//	}
		//
		//	camera.Transform.Translate(strafe, ascent, forward);
		//
		//}

		camera.Update();
	}

	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/CameraController.cpp
	void ApplyMomentum(float& oldValue, float& newValue, float deltaTime)
	{
		float blendedValue;
		if (abs(newValue) > abs(oldValue))
			blendedValue = lerp(newValue, oldValue, pow(0.6f, deltaTime * 60.0f));
		else
			blendedValue = lerp(newValue, oldValue, pow(0.8f, deltaTime * 60.0f));
		oldValue = blendedValue;
		newValue = blendedValue;
	}

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
