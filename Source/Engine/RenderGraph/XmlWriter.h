#pragma once
#include <ostream>

class XmlWriter
{
public:
	inline static constexpr char NewLine[] = "\n";

	static void Xml(std::ostream& Stream)
	{
		constexpr char Header[] = R"(<?xml version="1.0" encoding="utf-8"?>)";
		Stream << Header << NewLine;
	}
	static void BeginElement(std::ostream& Stream, std::string_view Name)
	{
		Stream << "<" << Name;
	}
	static void CloseElement(std::ostream& Stream)
	{
		Stream << ">" << NewLine;
	}
	static void EndCloseElement(std::ostream& Stream)
	{
		Stream << " />" << NewLine;
	}
	static void EndElement(std::ostream& Stream, std::string_view Name)
	{
		Stream << "</" << Name << ">" << NewLine;
	}

	static void Attribute(std::ostream& Stream, std::string_view Name, std::string_view Value)
	{
		if (Value.empty())
		{
			return;
		}
		Stream << " " << Name << "=\"" << Value << "\"";
	}
};
