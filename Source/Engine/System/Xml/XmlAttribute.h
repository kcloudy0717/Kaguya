#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "Types.h"

namespace System
{
	namespace Xml
	{
		struct XmlAttribute
		{
			template<typename T>
			[[nodiscard]] T GetValue() const noexcept;

			template<>
			[[nodiscard]] std::string_view GetValue() const noexcept
			{
				return Value;
			}

			template<>
			[[nodiscard]] i32 GetValue() const noexcept
			{
				// :(
				std::string String(Value.begin(), Value.end());
				return std::stoi(String);
			}

			template<>
			[[nodiscard]] float GetValue() const noexcept
			{
				// :(
				std::string String(Value.begin(), Value.end());
				return std::stof(String);
			}

			std::string_view Name;
			std::string_view Value;
		};

		struct XmlNode
		{
			template<typename TPredicate>
			[[nodiscard]] const XmlAttribute* GetAttribute(TPredicate Predicate) const noexcept
			{
				for (const auto& Attribute : Attributes)
				{
					if (Predicate(Attribute))
					{
						return &Attribute;
					}
				}
				return nullptr;
			}

			[[nodiscard]] const XmlAttribute* GetAttributeByName(std::string_view Name) const noexcept
			{
				return GetAttribute([Name](const XmlAttribute& Attribute)
									{
										return Attribute.Name == Name;
									});
			}

			template<typename TPredicate>
			[[nodiscard]] const XmlNode* GetChild(TPredicate Predicate) const noexcept
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

			[[nodiscard]] const XmlNode* GetChildByTag(std::string_view Tag) const noexcept
			{
				return GetChild([Tag](const XmlNode& Node)
								{
									return Node.Tag == Tag;
								});
			}

			bool					  IsProlog;
			std::string_view		  Tag;
			std::vector<XmlAttribute> Attributes;

			std::vector<XmlNode> Children;
		};
	} // namespace Xml
} // namespace System
