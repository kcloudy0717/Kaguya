#include "Exception.h"
#include <sstream>

Exception::Exception(std::string_view Source, int Line)
	: Source(Source)
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
	Stream << "[File] " << Source << std::endl
		   << "[Line] " << Line;
	return Stream.str();
}

std::string Exception::GetErrorString() const
{
	std::ostringstream Stream;
	Stream << GetErrorType() << std::endl
		   << GetExceptionOrigin() << std::endl
		   << GetError();
	return Stream.str();
}

const char* ExceptionIO::GetErrorType() const noexcept
{
	return "[IO]";
}

std::string ExceptionIO::GetError() const
{
	return "[IO Error]";
}

std::string ExceptionFileNotFound::GetError() const
{
	return "[File Not Found]";
}

std::string ExceptionPathNotFound::GetError() const
{
	return "[Path Not Found]";
}
