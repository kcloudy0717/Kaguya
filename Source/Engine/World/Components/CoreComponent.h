#pragma once

class CoreComponent
{
public:
	std::string Name;

	Transform Transform;
};

REGISTER_CLASS_ATTRIBUTES(
	Transform,
	"Transform",
	CLASS_ATTRIBUTE(Transform, Position),
	CLASS_ATTRIBUTE(Transform, Scale),
	CLASS_ATTRIBUTE(Transform, Orientation))

REGISTER_CLASS_ATTRIBUTES(
	CoreComponent,
	"Core",
	CLASS_ATTRIBUTE(CoreComponent, Name),
	CLASS_ATTRIBUTE(CoreComponent, Transform))
