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
	return "[Core]";
}

std::string Exception::GetError() const
{
	return "<unknown>";
}

std::string Exception::GetExceptionOrigin() const
{
	std::ostringstream oss;
	oss << "[File] " << File << std::endl << "[Line] " << Line;
	return oss.str();
}

std::string Exception::GetErrorString() const
{
	std::ostringstream oss;
	oss << GetErrorType() << std::endl << GetExceptionOrigin() << std::endl << GetError();
	return oss.str();
}
