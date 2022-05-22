#pragma once

struct CoreComponent
{
	std::string		Name;
	Math::Transform Transform;
};

REGISTER_CLASS_ATTRIBUTES(
	Math::Transform,
	"Transform",
	CLASS_ATTRIBUTE(Math::Transform, Position),
	CLASS_ATTRIBUTE(Math::Transform, Scale),
	CLASS_ATTRIBUTE(Math::Transform, Orientation))

REGISTER_CLASS_ATTRIBUTES(
	CoreComponent,
	"Core",
	CLASS_ATTRIBUTE(CoreComponent, Name),
	CLASS_ATTRIBUTE(CoreComponent, Transform))
