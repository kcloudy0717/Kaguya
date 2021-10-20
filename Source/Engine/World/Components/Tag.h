#pragma once

struct Tag : Component
{
	Tag() noexcept = default;
	Tag(std::string_view Name)
		: Name(Name)
	{
	}

	operator std::string&() { return Name; }
	operator const std::string&() const { return Name; }

	std::string Name;
};

REGISTER_CLASS_ATTRIBUTES(Tag, "Tag", CLASS_ATTRIBUTE(Tag, Name))
