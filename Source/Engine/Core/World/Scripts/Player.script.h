#pragma once
#include "../Actor.h"

class PlayerScript : public ScriptableActor
{
public:
	void OnCreate() override;

	void OnDestroy() override;

	void OnUpdate(float DeltaTime) override;

	float LastForward;
	float LastStrafe;
	float LastAscent;
};
