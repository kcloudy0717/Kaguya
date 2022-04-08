#pragma once
#include "SystemCore.h"
#include "Delegate.h"

class FileSystemWatcher
{
public:
	using Event = MulticastDelegate<const FileSystemEventArgs&>;

	explicit FileSystemWatcher(const std::filesystem::path& Path);

	~FileSystemWatcher();

private:
	void Watch();

private:
	std::filesystem::path Path;
	wil::unique_handle	  FileHandle;

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
