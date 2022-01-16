#include "Dgml.h"
#include "XmlWriter.h"

namespace Dgml
{
	constexpr const char* GetDirectionName(GraphDirection Direction)
	{
		switch (Direction)
		{
		case GraphDirection::TopToBottom:
			return "TopToBottom";
		case GraphDirection::BottomToTop:
			return "BottomToTop";
		case GraphDirection::LeftToRight:
			return "LeftToRight";
		case GraphDirection::RightToLeft:
			return "RightToLeft";
		}
		return nullptr;
	}

	static void SerializeNode(const Node& Node, std::ostream& Stream)
	{
		XmlWriter::BeginElement(Stream, "Node");
		{
			XmlWriter::Attribute(Stream, "Id", Node.Id);
			XmlWriter::Attribute(Stream, "Label", Node.Label);
		}
		XmlWriter::EndCloseElement(Stream);
	}

	static void SerializeLink(const Link& Link, std::ostream& Stream)
	{
		XmlWriter::BeginElement(Stream, "Link");
		{
			XmlWriter::Attribute(Stream, "Source", Link.Source);
			XmlWriter::Attribute(Stream, "Target", Link.Target);
			XmlWriter::Attribute(Stream, "Label", Link.Label);
		}
		XmlWriter::EndCloseElement(Stream);
	}

	Graph::Graph(
		const std::string& Title,
		GraphDirection	   Direction /*= GraphDirection::Default*/)
		: Title(Title)
		, Direction(Direction)
	{
	}

	Dgml::Node* Graph::AddNode()
	{
		return Nodes.emplace_back(std::make_unique<Node>()).get();
	}

	Dgml::Link* Graph::AddLink()
	{
		return Links.emplace_back(std::make_unique<Link>()).get();
	}

	void Graph::Serialize(std::ostream& Stream) const
	{
		// <DirectedGraph Title=Title xmlns="http://schemas.microsoft.com/vs/2009/dgml">
		XmlWriter::BeginElement(Stream, "DirectedGraph");
		XmlWriter::Attribute(Stream, "Title", Title.data());
		if (Direction != GraphDirection::Default)
		{
			XmlWriter::Attribute(Stream, "Layout", "Sugiyama");
			XmlWriter::Attribute(Stream, "GraphDirection", GetDirectionName(Direction));
		}
		XmlWriter::Attribute(Stream, "xmlns", "http://schemas.microsoft.com/vs/2009/dgml");
		XmlWriter::CloseElement(Stream);
		{
			// <Nodes>
			XmlWriter::BeginElement(Stream, "Nodes");
			XmlWriter::CloseElement(Stream);
			{
				for (const auto& Node : Nodes)
				{
					SerializeNode(*Node, Stream);
				}
			}
			XmlWriter::EndElement(Stream, "Nodes");
			// <Nodes />

			// <Links>
			XmlWriter::BeginElement(Stream, "Links");
			XmlWriter::CloseElement(Stream);
			{
				for (const auto& Link : Links)
				{
					SerializeLink(*Link, Stream);
				}
			}
			XmlWriter::EndElement(Stream, "Links");
			// <Links />
		}
		XmlWriter::EndElement(Stream, "DirectedGraph");
		// </DirectedGraph>
	}
} // namespace Dgml
