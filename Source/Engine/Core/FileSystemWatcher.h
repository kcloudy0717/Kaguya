#pragma once
#include "Delegate.h"

enum class NotifyFilters
{
	None	   = 0,
	FileName   = 1 << 0,
	DirName	   = 1 << 1,
	Attributes = 1 << 2,
	Size	   = 1 << 3,
	LastWrite  = 1 << 4,
	LastAccess = 1 << 5,
	Creation   = 1 << 6,
	Security   = 1 << 7,

	All = 0xFF
};
ENABLE_BITMASK_OPERATORS(NotifyFilters);

struct FileSystemEventArgs
{
	std::filesystem::path Path;
};

class FileSystemWatcher
{
public:
	using Event = MulticastDelegate<const FileSystemEventArgs&>;

	explicit FileSystemWatcher(std::filesystem::path Path);

	~FileSystemWatcher();

private:
	void Watch();

public:
	std::atomic<bool>		   IncludeSubdirectories = false;
	std::atomic<NotifyFilters> NotifyFilter			 = NotifyFilters::None;
	Event					   OnAdded				 = Event(true);
	Event					   OnRemoved			 = Event(true);
	Event					   OnModified			 = Event(true);
	Event					   RenamedOldName		 = Event(true);
	Event					   RenamedNewName		 = Event(true);

private:
	std::filesystem::path Path;
	wil::unique_handle	  FileHandle;

	std::atomic<bool> Running = true;
	std::mutex		  Mutex;
	std::jthread	  Thread;
};
