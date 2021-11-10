#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Actor.h>

class WorldWindow : public UIWindow
{
public:
	WorldWindow()
		: UIWindow("World", 0)
	{
	}

	void SetContext(World* pWorld)
	{
		this->pWorld  = pWorld;
		SelectedIndex = {};
	}

	[[nodiscard]] Actor GetSelectedActor() const
	{
		Actor SelectedActor = {};
		if (SelectedIndex)
		{
			SelectedActor = pWorld->Actors[SelectedIndex.value()];
		}
		return SelectedActor;
	}

protected:
	void OnRender() override;

private:
	World*				  pWorld = nullptr;
	std::optional<size_t> SelectedIndex;
};
