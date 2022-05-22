#include "Platform.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

void Platform::Exit(std::wstring_view Message)
{
	// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code?redirectedfrom=MSDN

	LPCTSTR lpszFunction = Message.data();

	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	DWORD  dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		nullptr);

	// Display the error message and exit the process
	LPVOID lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen(lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf(
		(LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction,
		dw,
		lpMsgBuf);
	MessageBox(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}
