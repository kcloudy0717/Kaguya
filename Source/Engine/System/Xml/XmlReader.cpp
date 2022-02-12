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

		XmlNode XmlReader::ParseRootNode()
		{
			XmlNode RootNode = {};
			while (!Eof())
			{
				SkipXmlWhitespaceAndComments();
				RootNode.Children.push_back(ParseNode());
				SkipXmlWhitespaceAndComments();
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

		XmlNode XmlReader::ParseNode()
		{
			if (Eof())
			{
				return {};
			}

			Expect('<');

			XmlNode Node  = {};
			Node.IsProlog = Match('?');

			// Parse node tag
			const char* Begin				 = Ptr;
			size_t		NumCharactersSkipped = AdvanceUntil([](char Character)
														{
															return IsSpace(Character) || Character == '>';
														});
			Node.Tag						 = { Begin, Begin + NumCharactersSkipped };

			if (Node.Tag.empty())
			{
				throw std::exception("Empty open tag!");
			}
			if (Node.Tag[0] == '/')
			{
				throw std::exception("Unexpected closing tag, expected open tag!");
			}

			SkipXmlWhitespaceAndComments();

			// Parse attributes, <Tag ...Attributes...>
			while (!Eof() && !Match('>'))
			{
				XmlAttribute Attribute = {};

				// Parse attribute name
				Begin				 = Ptr;
				NumCharactersSkipped = AdvanceUntil([](char Character)
													{
														return Character == '=';
													});
				Attribute.Name		 = { Begin, Begin + NumCharactersSkipped };

				Expect('=');
				// Advance until we hit a single/double quote
				AdvanceUntil([](char Character)
							 {
								 return Character == '\'' || Character == '"';
							 });
				char QuoteType;
				if (Match('\''))
				{
					QuoteType = '\'';
				}
				else if (Match('"'))
				{
					QuoteType = '"';
				}
				else
				{
					throw std::exception("An attribute must begin with either single or double quotes!");
				}

				// Parse attribute value
				Begin				 = Ptr;
				NumCharactersSkipped = AdvanceUntil([QuoteType](char Character)
													{
														return Character == QuoteType;
													});
				Attribute.Value		 = { Begin, Begin + NumCharactersSkipped };

				Expect(QuoteType);
				SkipXmlWhitespaceAndComments();

				Node.Attributes.push_back(Attribute);

				// Check if this is an inline tag (i.e. <tag/> or <?tag?>), if so return
				if (Match('/') || (Node.IsProlog && Match('?')))
				{
					Expect('>');
					return Node;
				}
			}

			SkipXmlWhitespaceAndComments();

			// Parse children
			while (!Match("</"))
			{
				Node.Children.push_back(ParseNode());
				SkipXmlWhitespaceAndComments();
			}

			const char* ClosingTagStart = Ptr;
			while (!Eof() && !Match('>'))
			{
				Advance();
			}

			std::string_view ClosingTag = { ClosingTagStart, Ptr - 1 };
			if (Node.Tag != ClosingTag)
			{
				throw std::exception("Non matching closing tag!");
			}

			return Node;
		}
	} // namespace Xml
} // namespace System
