#include <Windows.h>
#include <DbgHelp.h>

#include <Shlobj.h>
#include <Shlobj_core.h>
#include <shlwapi.h>

static char sMinidumpPath[MAX_PATH] = {};

static void collectMinidump(EXCEPTION_POINTERS* ex)
{
    HANDLE minidumpFile = CreateFileA(sMinidumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    MINIDUMP_EXCEPTION_INFORMATION exceptionInformation;
    exceptionInformation.ThreadId = GetCurrentThreadId();
    exceptionInformation.ExceptionPointers = ex;
    exceptionInformation.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), minidumpFile, (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithPrivateReadWriteMemory), &exceptionInformation, NULL, NULL);

    CloseHandle(minidumpFile);
}

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* ex)
{
    collectMinidump(ex);
    ExitProcess(1);
}

void setupExceptionFilters()
{
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, sMinidumpPath);
    PathAppendA(sMinidumpPath, "Luna-Project64");
    CreateDirectoryA(sMinidumpPath, nullptr);
    PathAppendA(sMinidumpPath, "minidump.dmp");

    SetUnhandledExceptionFilter(&unhandledExceptionFilter);
}
