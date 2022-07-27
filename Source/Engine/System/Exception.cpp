#include "Exception.h"
#include <sstream>

Exception::Exception(std::string_view File, int Line)
	: File(File)
	, Line(Line)
{
}

const char* Exception::what() const noexcept
{
	Error = GetErrorString();
	return Error.data();
}

const char* Exception::GetErrorType() const noexcept
{
	return "[Unknown]";
}

std::string Exception::GetError() const
{
	return "[Unknown]";
}

std::string Exception::GetExceptionOrigin() const
{
	std::ostringstream Stream;
	Stream << "[File] " << File << std::endl << "[Line] " << Line;
	return Stream.str();
}

std::string Exception::GetErrorString() const
{
	std::ostringstream Stream;
	Stream << GetErrorType() << std::endl << GetExceptionOrigin() << std::endl << GetError();
	return Stream.str();
}
