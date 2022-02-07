#pragma once
#include "SystemCore.h"

namespace System
{
	namespace Xml
	{
		struct SourceLocation
		{
			void Advance(char Character)
			{
				if (Character == '\n')
				{
					Line++;
					Column = 0;
				}
				else if (Character == '\t')
				{
					Column += 4;
				}
				else
				{
					Column++;
				}
			}

			int				 Line;
			int				 Column;
		};

		struct XmlAttribute
		{
			std::string_view Name;
			std::string_view Value;
			SourceLocation	 Location;
		};

		struct XmlNode
		{
			std::string_view Tag;
			bool			 IsQuestionMark;
			SourceLocation	 Location;

			std::vector<XmlNode>	  Children;
			std::vector<XmlAttribute> Attributes;
		};
	} // namespace Xml
} // namespace System
