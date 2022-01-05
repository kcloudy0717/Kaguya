#pragma once
#include <ostream>
#include <string_view>

class XmlWriter
{
public:
	inline static constexpr char NewLine[] = "\n";

	static void Xml(std::ostream& Output)
	{
		constexpr char Header[] = R"(<?xml version="1.0" encoding="utf-8"?>)";
		Output << Header << NewLine;
	}
	static void BeginElement(std::ostream& Output, std::string_view Name)
	{
		Output << "<" << Name;
	}
	static void CloseElement(std::ostream& Output)
	{
		Output << ">" << NewLine;
	}
	static void EndCloseElement(std::ostream& Output)
	{
		Output << " />" << NewLine;
	}
	static void EndElement(std::ostream& Output, std::string_view Name)
	{
		Output << "</" << Name << ">" << NewLine;
	}

	static void Attribute(std::ostream& Output, std::string_view Name, std::string_view Value)
	{
		if (Value.empty())
		{
			return;
		}
		Output << " " << Name << "=\"" << Value << "\"";
	}
};
