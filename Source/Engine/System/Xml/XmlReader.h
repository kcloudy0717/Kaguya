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

			bool Eof() const
			{
				return Ptr >= Sentinel;
			}

			void Advance(size_t NumCharacters = 1) const
			{
				if (Ptr + NumCharacters > Sentinel)
				{
					throw std::exception("Unexpected end of file!");
				}

				Ptr += NumCharacters;
			}

			template<typename TPredicate>
			size_t AdvanceUntil(TPredicate Predicate)
			{
				size_t NumCharactersSkipped = 0;
				while (Ptr < Sentinel)
				{
					if (Predicate(*Ptr))
					{
						break;
					}
					Advance();
					++NumCharactersSkipped;
				}
				return NumCharactersSkipped;
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
				if (Eof())
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

			void SkipXmlWhitespaceAndComments()
			{
				SkipSpaceOrNewline();

				while (Match("<!--"))
				{
					while (!Eof() && !Match("-->"))
					{
						Advance();
					}
					SkipSpaceOrNewline();
				}
			}

			XmlNode ParseNode();

		private:
			FileStream&					 Stream;
			std::unique_ptr<std::byte[]> BaseAddress;
			mutable const char*			 Ptr;
			const char*					 Sentinel;
		};
	} // namespace Xml
} // namespace System
