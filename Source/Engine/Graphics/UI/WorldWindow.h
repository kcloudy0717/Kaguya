#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Entity.h>

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

	[[nodiscard]] Entity GetSelectedEntity() const
	{
		Entity SelectedEntity = {};
		if (SelectedIndex)
		{
			SelectedEntity = pWorld->Entities[SelectedIndex.value()];
		}
		return SelectedEntity;
	}

protected:
	void OnRender() override;

private:
	World*				  pWorld = nullptr;
	std::optional<size_t> SelectedIndex;
};
