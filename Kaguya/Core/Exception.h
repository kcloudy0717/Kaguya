#pragma once
#include <winnt.h>
#include <exception>
#include <string>

class Exception : public std::exception
{
public:
	Exception() noexcept
		: m_File{}
		, m_Line(0)
	{

	}
	explicit Exception(
		std::string File,
		int Line) noexcept;
	virtual ~Exception() = default;

	const char* what() const noexcept override;

	virtual std::string Type() const noexcept;
	virtual std::string Error() const noexcept;
	std::string Origin() const noexcept;

protected:
	mutable std::string m_ErrorMessage;

private:
	std::string m_File;
	int m_Line;
};

class hresult_exception final : public Exception
{
public:
	hresult_exception() noexcept
		: m_HR(S_FALSE)
	{

	}
	explicit hresult_exception(
		std::string File,
		int Line,
		HRESULT HR) noexcept;

	std::string Type() const noexcept override;
	std::string Error() const noexcept override;
private:
	HRESULT m_HR;
};
