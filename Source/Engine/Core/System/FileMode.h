#pragma once

enum class FileMode
{
	// Specifies that the operating system should create a new file. This requires Write permission. If the file already
	// exists, an IOException exception is thrown.
	CreateNew,

	// Specifies that the operating system should create a new file. If the file already exists, it will be overwritten.
	// This requires Write permission. FileMode.Create is equivalent to requesting that if the file does not exist, use
	// CreateNew; otherwise, use Truncate. If the file already exists but is a hidden file, an
	// UnauthorizedAccessException exception is thrown.
	Create,

	// Specifies that the operating system should open an existing file. The ability to open the file is dependent on
	// the value specified by the FileAccess enumeration. A FileNotFoundException exception is thrown if the file does
	// not exist.
	Open,

	// Specifies that the operating system should open a file if it exists; otherwise, a new file should be created. If
	// the file is opened with FileAccess.Read, Read permission is required. If the file access is FileAccess.Write,
	// Write permission is required. If the file is opened with FileAccess.ReadWrite, both Read and Write permissions
	// are required.
	OpenOrCreate,

	// Specifies that the operating system should open an existing file. When the file is opened, it should be truncated
	// so that its size is zero bytes. This requires Write permission. Attempts to read from a file opened with
	// FileMode.Truncate cause an ArgumentException exception.
	Truncate,
};
