#include "DgmlBuilder.h"
#include "XmlWriter.h"
#include <fstream>

Dgml::Node* DgmlBuilder::AddNode(std::string_view Id, std::string_view Label)
{
	auto& Node	= Graph.Nodes.emplace_back(std::make_unique<Dgml::Node>());
	Node->Id	= Id;
	Node->Label = Label;
	return Node.get();
}

Dgml::Link* DgmlBuilder::AddLink(const std::string& Source, const std::string& Target, std::string_view Label)
{
	auto& Link	 = Graph.Links.emplace_back(std::make_unique<Dgml::Link>());
	Link->Source = Source;
	Link->Target = Target;
	Link->Label	 = Label;
	return Link.get();
}

void DgmlBuilder::SaveAs(const std::filesystem::path& Path) const
{
	std::ofstream FileStream;
	FileStream.open(Path);
	Serialize(FileStream);
	FileStream.close();
}

void DgmlBuilder::Serialize(std::ostream& Output) const
{
	XmlWriter::Xml(Output);
	Graph.Serialize(Output);
}
