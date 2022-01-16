#pragma once
#include <memory>
#include <string>
#include <vector>

namespace Dgml
{
	struct Node
	{
		std::string Id;
		std::string Label;
	};

	struct Link
	{
		std::string Source;
		std::string Target;
		std::string Label;
	};

	enum class GraphDirection
	{
		Default,
		TopToBottom,
		BottomToTop,
		LeftToRight,
		RightToLeft
	};

	class Graph
	{
	public:
		explicit Graph(
			const std::string& Title,
			GraphDirection	   Direction = GraphDirection::Default);

		Node* AddNode();

		Link* AddLink();

		void Serialize(std::ostream& Stream) const;

	private:
		std::string						   Title;
		GraphDirection					   Direction;
		std::vector<std::unique_ptr<Node>> Nodes;
		std::vector<std::unique_ptr<Link>> Links;
	};
} // namespace Dgml
