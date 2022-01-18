#pragma once
#include "SystemCore.h"

class FileStream;

class BinaryWriter
{
public:
	explicit BinaryWriter(FileStream& Stream);

	void Write(const void* Data, u64 SizeInBytes) const;

	template<typename T>
	void Write(const T& Object) const
	{
		Write(&Object, sizeof(T));
	}

	template<>
	void Write<std::string>(const std::string& Object) const
	{
		Write(Object.data(), Object.size() * sizeof(std::string::value_type));
	}

	template<>
	void Write<std::wstring>(const std::wstring& Object) const
	{
		Write(Object.data(), Object.size() * sizeof(std::wstring::value_type));
	}

private:
	FileStream& Stream;
};
