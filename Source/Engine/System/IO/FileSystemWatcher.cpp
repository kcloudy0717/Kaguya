#include "FileSystemWatcher.h"

FileSystemWatcher::FileSystemWatcher(const std::filesystem::path& Path)
	: Path(Path)
	, FileHandle(CreateFile(
		  this->Path.c_str(),
		  FILE_GENERIC_READ,
		  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		  nullptr,
		  OPEN_EXISTING,
		  FILE_FLAG_BACKUP_SEMANTICS,
		  nullptr))
	, Thread(
		  [this]()
		  {
			  this->Watch();
		  })
{
}

FileSystemWatcher::~FileSystemWatcher()
{
	Running = false;
	CancelIoEx(FileHandle.get(), nullptr);
}

void FileSystemWatcher::Watch()
{
	auto			lpBuffer	  = std::make_unique<BYTE[]>(32 * 1028);
	constexpr DWORD nBufferLength = sizeof(BYTE) * 32 * 1028;

	while (Running)
	{
		DWORD BytesReturned = 0;

		BOOL  bWatchSubtree	 = IncludeSubdirectories ? TRUE : FALSE;
		DWORD dwNotifyFilter = 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::FileName) ? FILE_NOTIFY_CHANGE_FILE_NAME : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::DirName) ? FILE_NOTIFY_CHANGE_DIR_NAME : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::Attributes) ? FILE_NOTIFY_CHANGE_ATTRIBUTES : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::Size) ? FILE_NOTIFY_CHANGE_SIZE : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::LastWrite) ? FILE_NOTIFY_CHANGE_LAST_WRITE : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::LastAccess) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::Creation) ? FILE_NOTIFY_CHANGE_CREATION : 0;
		dwNotifyFilter |= EnumMaskBitSet(NotifyFilter, NotifyFilters::Security) ? FILE_NOTIFY_CHANGE_SECURITY : 0;

		if (ReadDirectoryChangesW(
				FileHandle.get(),
				lpBuffer.get(),
				nBufferLength,
				bWatchSubtree,
				dwNotifyFilter,
				&BytesReturned,
				nullptr,
				nullptr))
		{
			BYTE*					 Ptr				= lpBuffer.get();
			char					 FileName[MAX_PATH] = {};
			FILE_NOTIFY_INFORMATION* Information		= nullptr;
			do
			{
				Information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(Ptr);

				int Length = WideCharToMultiByte(
					CP_ACP,
					0,
					Information->FileName,
					Information->FileNameLength / sizeof(wchar_t),
					FileName,
					sizeof(FileName),
					nullptr,
					nullptr);
				FileName[Length] = '\0';

				FileSystemEventArgs Args = {};
				Args.Path				 = FileName;

				std::scoped_lock _(Mutex);
				switch (Information->Action)
				{
				case FILE_ACTION_ADDED:
					OnAdded(Args);
					break;
				case FILE_ACTION_REMOVED:
					OnRemoved(Args);
					break;
				case FILE_ACTION_MODIFIED:
					OnModified(Args);
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					RenamedOldName(Args);
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					RenamedNewName(Args);
					break;
				default:
					break;
				}

				Ptr += Information->NextEntryOffset;
			} while (Information->NextEntryOffset);
		}
	}
}
