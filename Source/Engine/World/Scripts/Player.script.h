#pragma once
#include "../Entity.h"

class PlayerScript : public ScriptableEntity
{
public:
	void OnCreate() override;

	void OnDestroy() override;

	void OnUpdate(float DeltaTime) override;

	float LastForward;
	float LastStrafe;
	float LastAscent;
};
