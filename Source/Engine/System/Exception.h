#pragma once
#include <stdexcept>
#include <string_view>

class Exception : public std::exception
{
public:
	Exception(std::string_view Source, int Line);

	const char* what() const noexcept final;

protected:
	virtual const char* GetErrorType() const noexcept;
	virtual std::string GetError() const;

private:
	std::string GetExceptionOrigin() const;
	std::string GetErrorString() const;

private:
	std::string_view Source;
	int				 Line;

	mutable std::string Error;
};

class ExceptionIO : public Exception
{
public:
	using Exception::Exception;

private:
	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;
};

class ExceptionFileNotFound final : public ExceptionIO
{
public:
	ExceptionFileNotFound(std::string_view Source, int Line)
		: ExceptionIO(Source, Line)
	{
	}

private:
	std::string GetError() const override;
};

class ExceptionPathNotFound final : public ExceptionIO
{
public:
	ExceptionPathNotFound(std::string_view Source, int Line)
		: ExceptionIO(Source, Line)
	{
	}

private:
	std::string GetError() const override;
};
