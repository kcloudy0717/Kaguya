#pragma once
#include "SystemCore.h"

namespace System
{
	namespace Xml
	{
		struct XmlAttribute
		{
			std::string_view Name;
			std::string_view Value;
		};

		struct XmlNode
		{
			template<typename TPredicate>
			const XmlNode* GetChild(TPredicate Predicate)
			{
				for (const auto& Child : Children)
				{
					if (Predicate(Child))
					{
						return &Child;
					}
				}
				return nullptr;
			}

			bool					  IsProlog;
			std::string_view		  Tag;
			std::vector<XmlAttribute> Attributes;

			std::vector<XmlNode> Children;
		};
	} // namespace Xml
} // namespace System
