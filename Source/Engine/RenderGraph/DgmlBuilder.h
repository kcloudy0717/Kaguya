#pragma once
#include <filesystem>
#include "Dgml.h"

class DgmlBuilder
{
public:
	[[nodiscard]] Dgml::Node* AddNode(
		std::string_view Id,
		std::string_view Label);

	[[nodiscard]] Dgml::Link* AddLink(
		const std::string& Source,
		const std::string& Target,
		std::string_view   Label);

	void SaveAs(const std::filesystem::path& Path) const;

private:
	void Serialize(std::ostream& Output) const;

private:
	Dgml::Graph Graph;
};
