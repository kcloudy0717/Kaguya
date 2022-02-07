#pragma once
#include "SystemCore.h"
#include "XmlAttribute.h"

class FileStream;

namespace System
{
	namespace Xml
	{
		class XmlReader
		{
		public:
			explicit XmlReader(FileStream& Stream);

			XmlNode ParseRoot();

		private:
			static bool IsDigit(char Character);
			static bool IsSpace(char Character);
			static bool IsNewline(char Character);

			bool ReachedEnd() const
			{
				return Ptr >= Sentinel;
			}

			char Advance(int NumCharacters = 1)
			{
				if (Ptr + NumCharacters > Sentinel)
				{
					throw std::exception("Unexpected end of file!");
				}

				char Character = *Ptr;
				for (int i = 0; i < NumCharacters; i++)
				{
					Location.Advance(*Ptr);
					Ptr++;
				}
				return Character;
			}

			template<typename TPredicate>
			void Skip(TPredicate Predicate)
			{
				while (Ptr < Sentinel && Predicate(*Ptr))
				{
					Advance();
				}
			}

			void SkipSpace()
			{
				Skip([](char Character)
					 {
						 return IsSpace(Character);
					 });
			}

			void SkipSpaceOrNewline()
			{
				Skip([](char Character)
					 {
						 return IsSpace(Character) || IsNewline(Character);
					 });
			}

			bool Match(char Character)
			{
				if (Ptr < Sentinel && *Ptr == Character)
				{
					Advance();
					return true;
				}
				return false;
			}

			bool Match(std::string_view String)
			{
				size_t Size = String.size();
				if (Ptr + Size <= Sentinel && String == std::string_view{ Ptr, Ptr + Size })
				{
					for (size_t i = 0; i < Size; i++)
					{
						Advance();
					}
					return true;
				}
				return false;
			}

			template<i32 N>
			bool Match(const char (&Array)[N])
			{
				return Match(std::string_view{ Array, Array + N - 1 });
			}

			void Expect(char Character)
			{
				if (ReachedEnd())
				{
					throw std::exception("Unexpected end of file!");
				}
				if (*Ptr != Character)
				{
					throw std::exception("Unexpected char!");
				}
				Advance();
			}

			template<i32 N>
			void Expect(const char (&Array)[N])
			{
				for (int i = 0; i < N - 1; i++)
				{
					Expect(Array[i]);
				}
			}

			f32 ParseFloat()
			{
				if (Match("nan") || Match("NAN"))
				{
					return NAN;
				}

				bool Sign = false;
				if (Match('-'))
				{
					Sign = true;
				}
				else if (Match('+'))
				{
					Sign = false;
				}
				SkipSpace();

				if (Match("inf") || Match("INF") || Match("infinity") || Match("INFINITY"))
				{
					return Sign ? -INFINITY : INFINITY;
				}

				double Value = 0.0;

				bool IntegerPart	= false;
				bool FractionalPart = false;

				// Parse integer part
				if (IsDigit(*Ptr))
				{
					Value		= ParseInt();
					IntegerPart = true;
				}

				// Parse fractional part
				if (Match('.'))
				{
					static constexpr double DigitLut[] = { 0.1, 0.01, 0.001, 0.0001, 0.00001, 0.000001, 0.0000001, 0.00000001, 0.000000001, 0.0000000001, 0.00000000001 };

					size_t Digit = 0;
					while (IsDigit(*Ptr))
					{
						double p;
						if (Digit < std::size(DigitLut))
						{
							p = DigitLut[Digit];
						}
						else
						{
							p = pow(0.1, Digit);
						}
						Value += static_cast<double>(*Ptr - '0') * p;

						Digit++;
						Advance();
					}

					FractionalPart = Digit > 0;
				}

				if (!IntegerPart && !FractionalPart)
				{
					throw std::exception("Expected float");
				}

				// Parse exponent
				if (Match('e') || Match('E'))
				{
					int Exponent = ParseInt();
					Value		 = Value * pow(10.0, Exponent);
				}

				return static_cast<f32>(Sign ? -Value : Value);
			}

			i32 ParseInt()
			{
				bool Sign = Match('-');
				if (!Sign)
				{
					Match('+');
				}

				if (!IsDigit(*Ptr))
				{
					throw std::exception("Expected integer digit");
				}

				i32 Value = 0;
				while (IsDigit(*Ptr))
				{
					Value *= 10;
					Value += *Ptr - '0';
					Advance();
				}

				return Sign ? -Value : Value;
			}

			void SkipXmlWhitespace()
			{
				SkipSpaceOrNewline();

				while (Match("<!--"))
				{
					while (!ReachedEnd() && !Match("-->"))
					{
						Advance();
					}
					SkipSpaceOrNewline();
				}
			}

			XmlNode ParseNode()
			{
				if (ReachedEnd())
				{
					return {};
				}

				Expect('<');

				XmlNode Node		= {};
				Node.Location		= Location;
				Node.IsQuestionMark = Match('?');

				// Parse node tag
				const char* Begin = Ptr;
				while (!ReachedEnd() && !IsSpace(*Ptr) && *Ptr != '>')
					Advance();
				Node.Tag = { Begin, Ptr };

				if (Node.Tag.empty())
				{
					throw std::exception("Empty open tag!");
				}
				if (Node.Tag[0] == '/')
				{
					throw std::exception("Unexpected closing tag, expected open tag!");
				}

				SkipXmlWhitespace();

				// Parse attributes
				while (!ReachedEnd() && !Match('>'))
				{
					XmlAttribute Attribute = {};

					// Parse attribute name
					const char* Begin = Ptr;
					while (!ReachedEnd() && *Ptr != '=')
					{
						Advance();
					}
					Attribute.Name = { Begin, Ptr };

					Expect('=');
					while (!ReachedEnd() && *Ptr != '"' && *Ptr != '\'')
					{
						Advance();
					}

					char QuoteType; // Either single or double quotes
					if (Match('"'))
					{
						QuoteType = '"';
					}
					else if (Match('\''))
					{
						QuoteType = '\'';
					}
					else
					{
						throw std::exception("An attribute must begin with either single or double quotes!");
					}

					Attribute.Location = Location;

					// Parse attribute value
					Begin = Ptr;
					while (!ReachedEnd() && *Ptr != QuoteType)
						Advance();
					Attribute.Value = { Begin, Ptr };

					Expect(QuoteType);
					SkipXmlWhitespace();

					Node.Attributes.push_back(Attribute);

					// Check if this is an inline tag (i.e. <tag/> or <?tag?>), if so return
					if (Match('/') || (Node.IsQuestionMark && Match('?')))
					{
						Expect('>');
						return Node;
					}
				}

				SkipXmlWhitespace();

				// Parse children
				while (!Match("</"))
				{
					Node.Children.push_back(ParseNode());
					SkipXmlWhitespace();
				}

				const char* ClosingTagStart = Ptr;
				while (!ReachedEnd() && !Match('>'))
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

		private:
			FileStream&					 Stream;
			std::unique_ptr<std::byte[]> BaseAddress;
			mutable const char*			 Ptr;
			const char*					 Sentinel;
			SourceLocation				 Location;
		};
	} // namespace Xml
} // namespace System
