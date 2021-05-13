#include "pch.h"
#include "CameraController.h"

using namespace DirectX;

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

FPSCamera::FPSCamera(Camera& Camera)
	: CameraController(Camera)
{
	m_MoveSpeed = 50.0f;
	m_StrafeSpeed = 50.0f;
	m_MouseSensitivityX = 15.0f;
	m_MouseSensitivityY = 15.0f;

	m_Momentum = true;

	m_LastForward = 0.0f;
	m_LastStrafe = 0.0f;
	m_LastAscent = 0.0f;
}

void FPSCamera::Update(float dt)
{
	InputHandler& InputHandler = Application::GetInputHandler();
	Mouse& Mouse = Application::GetMouse();
	Keyboard& Keyboard = Application::GetKeyboard();

	m_TargetCamera.AspectRatio = Application::AspectRatio();

	if (InputHandler.RawInputEnabled)
	{
		float forward = m_MoveSpeed * (
			(Keyboard.IsPressed('W') * dt) +
			(Keyboard.IsPressed('S') * -dt));
		float strafe = m_StrafeSpeed * (
			(Keyboard.IsPressed('D') * dt) +
			(Keyboard.IsPressed('A') * -dt));
		float ascent = m_StrafeSpeed * (
			(Keyboard.IsPressed('E') * dt) +
			(Keyboard.IsPressed('Q') * -dt));

		if (m_Momentum)
		{
			ApplyMomentum(m_LastForward, forward, dt);
			ApplyMomentum(m_LastStrafe, strafe, dt);
			ApplyMomentum(m_LastAscent, ascent, dt);
		}

		m_TargetCamera.Transform.Translate(strafe, ascent, forward);

		while (const auto e = Mouse.ReadRawInput())
		{
			m_TargetCamera.Rotate(e->Y * dt * m_MouseSensitivityY, e->X * dt * m_MouseSensitivityX);
		}
	}

	m_TargetCamera.Update();
}
