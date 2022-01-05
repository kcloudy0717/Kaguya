#include "DgmlBuilder.h"
#include "XmlWriter.h"
#include <fstream>

DgmlBuilder::DgmlBuilder(
	const std::string&	 Title,
	Dgml::GraphDirection Direction /*= Dgml::GraphDirection::Default*/)
	: Graph(Title, Direction)
{
}

Dgml::Node* DgmlBuilder::AddNode(std::string_view Id, std::string_view Label)
{
	auto Node	= Graph.AddNode();
	Node->Id	= Id;
	Node->Label = Label;
	return Node;
}

Dgml::Link* DgmlBuilder::AddLink(const std::string& Source, const std::string& Target, std::string_view Label)
{
	auto Link	 = Graph.AddLink();
	Link->Source = Source;
	Link->Target = Target;
	Link->Label	 = Label;
	return Link;
}

void DgmlBuilder::SaveAs(const std::filesystem::path& Path) const
{
	std::ofstream FileStream;
	FileStream.open(Path);
	XmlWriter::Xml(FileStream);
	Graph.Serialize(FileStream);
	FileStream.close();
}
