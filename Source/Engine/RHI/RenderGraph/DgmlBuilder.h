#pragma once
#include <filesystem>
#include "Dgml.h"

class DgmlBuilder
{
public:
	explicit DgmlBuilder(
		const std::string&	 Title,
		Dgml::GraphDirection Direction = Dgml::GraphDirection::Default);

	Dgml::Node* AddNode(
		std::string_view Id,
		std::string_view Label);

	Dgml::Link* AddLink(
		const std::string& Source,
		const std::string& Target,
		std::string_view   Label);

	void SaveAs(const std::filesystem::path& Path) const;

private:
	Dgml::Graph Graph;
};
