#include <Windows.h>
#include <DbgHelp.h>

#include <Shlobj.h>
#include <Shlobj_core.h>
#include <shlwapi.h>

#include <Common/path.h>
#include <Project64-core/Settings.h>

CPath sMinidumpPath;

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
    sMinidumpPath = CPath(g_Settings->LoadStringVal(Cmd_AppdataDirectory).c_str(), "minidump.dmp");
    SetUnhandledExceptionFilter(&unhandledExceptionFilter);
}
