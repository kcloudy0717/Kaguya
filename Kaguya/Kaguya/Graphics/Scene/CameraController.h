#pragma once
#include "Camera.h"

class CameraController
{
public:
	CameraController(Camera& Camera)
		: m_TargetCamera(Camera)
	{
	}

	virtual void Update(float dt) = 0;

protected:
	Camera& m_TargetCamera;
};

class FPSCamera : public CameraController
{
public:
	FPSCamera(Camera& Camera);

	void Update(float dt) override;

public:
private:
	float m_MoveSpeed;
	float m_StrafeSpeed;
	float m_MouseSensitivityX;
	float m_MouseSensitivityY;

	bool m_Momentum;

	float m_LastForward;
	float m_LastStrafe;
	float m_LastAscent;
};
