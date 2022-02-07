#include "XmlReader.h"
#include <IO/FileStream.h>

namespace System
{
	namespace Xml
	{
		XmlReader::XmlReader(FileStream& Stream)
			: Stream(Stream)
			, BaseAddress(Stream.ReadAll())
			, Ptr(reinterpret_cast<const char*>(BaseAddress.get()))
			, Sentinel(Ptr + Stream.GetSizeInBytes())
		{
		}

		XmlNode XmlReader::ParseRoot()
		{
			XmlNode RootNode  = {};
			RootNode.Location = Location;
			while (!ReachedEnd())
			{
				SkipXmlWhitespace();
				RootNode.Children.push_back(ParseNode());
				SkipXmlWhitespace();
			}
			return RootNode;
		}

		bool XmlReader::IsDigit(char Character)
		{
			return Character >= '0' && Character <= '9';
		}

		bool XmlReader::IsSpace(char Character)
		{
			return Character == ' ' || Character == '\t';
		}

		bool XmlReader::IsNewline(char Character)

		{
			return Character == '\r' || Character == '\n';
		}

	} // namespace Xml
} // namespace System
