#include "Dgml.h"
#include "XmlWriter.h"

namespace Dgml
{
	void Graph::Serialize(std::ostream& Output) const
	{
		// <DirectedGraph xmlns="http://schemas.microsoft.com/vs/2009/dgml">
		XmlWriter::BeginElement(Output, "DirectedGraph");
		XmlWriter::Attribute(Output, "xmlns", "http://schemas.microsoft.com/vs/2009/dgml");
		XmlWriter::CloseElement(Output);
		{
			// <Nodes>
			XmlWriter::BeginElement(Output, "Nodes");
			XmlWriter::CloseElement(Output);
			{
				for (const auto& Node : Nodes)
				{
					Node->Serialize(Output);
				}
			}
			XmlWriter::EndElement(Output, "Nodes");
			// <Nodes />

			// <Links>
			XmlWriter::BeginElement(Output, "Links");
			XmlWriter::CloseElement(Output);
			{
				for (const auto& Link : Links)
				{
					Link->Serialize(Output);
				}
			}
			XmlWriter::EndElement(Output, "Links");
			// <Links />
		}
		XmlWriter::EndElement(Output, "DirectedGraph");
		// </DirectedGraph>
	}

	void Node::Serialize(std::ostream& Output) const
	{
		XmlWriter::BeginElement(Output, "Node");
		{
			XmlWriter::Attribute(Output, "Id", Id);
			XmlWriter::Attribute(Output, "Label", Label);
		}
		XmlWriter::EndCloseElement(Output);
	}

	void Link::Serialize(std::ostream& Output) const
	{
		XmlWriter::BeginElement(Output, "Link");
		{
			XmlWriter::Attribute(Output, "Source", Source);
			XmlWriter::Attribute(Output, "Target", Target);
			XmlWriter::Attribute(Output, "Label", Label);
		}
		XmlWriter::EndCloseElement(Output);
	}
} // namespace Dgml
