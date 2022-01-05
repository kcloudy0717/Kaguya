#pragma once
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace Dgml
{
	class Node;
	class Link;

	class Graph
	{
	public:
		std::string							   Title;
		std::vector<std::unique_ptr<Node>>	   Nodes;
		std::vector<std::unique_ptr<Link>>	   Links;

		void Serialize(std::ostream& Output) const;
	};

	class Node
	{
	public:
		std::string Id;
		std::string Label;

		void Serialize(std::ostream& Output) const;
	};

	class Link
	{
	public:
		std::string Source;
		std::string Target;
		std::string Label;

		void Serialize(std::ostream& Output) const;
	};
} // namespace Dgml
