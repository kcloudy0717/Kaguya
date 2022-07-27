#pragma once
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include "Types.h"
#include "Platform.h"
#include "Delegate.h"

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

private:
	std::filesystem::path Path;
	ScopedFileHandle	  FileHandle;

public:
	std::atomic<bool>		   IncludeSubdirectories = false;
	std::atomic<NotifyFilters> NotifyFilter			 = NotifyFilters::None;
	Event					   OnAdded				 = Event(true);
	Event					   OnRemoved			 = Event(true);
	Event					   OnModified			 = Event(true);
	Event					   RenamedOldName		 = Event(true);
	Event					   RenamedNewName		 = Event(true);

private:
	std::atomic<bool> Running = true;
	std::mutex		  Mutex;
	std::jthread	  Thread;
};
