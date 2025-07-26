#include "stdafx.h"
#include "GitHubUpdater.h"

#include <Project64-core/VersionLuna.h>
#include <shellapi.h>
#include <winhttp.h>
#include "zlib/contrib/minizip/mz_strm.h"

#include "discord-rpc/rapidjson/document.h"

#include <thread>
#include <sstream>

#define MAX_PATH_LENGTH 1024

static void TriggerUpdater(void)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char currentPath[MAX_PATH_LENGTH];
    char updaterExePath[MAX_PATH_LENGTH];
    char updaterExePathZoneIdentifier[MAX_PATH_LENGTH];

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (GetModuleFileNameA(NULL, currentPath, MAX_PATH_LENGTH) == 0) {
        MessageBox(NULL, L"Failed to get current executable path.", L"Updater error", MB_OK);
    }

    char* lastBackslash = strrchr(currentPath, '\\');
    if (lastBackslash != NULL) {
        *lastBackslash = '\0';
    }

    SetCurrentDirectoryA(currentPath);

    snprintf(updaterExePath, MAX_PATH_LENGTH, "%s\\LunaU.exe", currentPath);
#if 1
    char zoneIdentifFormat[20]{};
    size_t zoneIdentifFormatTotal = 20;
    {
        static const char stringTestZipped[] = {
            120, -100, 83, 77, -41, -13, 75, 78, -118, 84, -78, -115, -120, 76, -54, -120, -115, -118, -115, 76, -5, 2, 0, 58, -117, 6, -56
        };
        zng_uncompress((uint8_t*)zoneIdentifFormat, &zoneIdentifFormatTotal, (const uint8_t*)stringTestZipped, sizeof(stringTestZipped));
    }
    for (int i = 1; i < zoneIdentifFormatTotal; i++)
        zoneIdentifFormat[i] += 12;
#else
    char zoneIdentifFormat[] = "%s:Zone.Identifier";
    for (int i = 1; i < sizeof(zoneIdentifFormat); i++)
    {
        zoneIdentifFormat[i] -= 12;
    }
    char buf[100]{};
    size_t bufSize = 100;
    zng_compress((uint8_t*)buf, &bufSize, (const uint8_t*)zoneIdentifFormat, sizeof(zoneIdentifFormat));
    std::string content;
    for (int i = 0; i < bufSize; i++)
    {
        content += ", ";
        content += std::to_string(buf[i]);
    }
#endif

    snprintf(updaterExePathZoneIdentifier, MAX_PATH_LENGTH, zoneIdentifFormat, updaterExePath);
    DeleteFileA(updaterExePathZoneIdentifier);

    char commandLine[MAX_PATH_LENGTH + 10];
    snprintf(commandLine, sizeof(commandLine), "\"%s\" %s", updaterExePath, "v" VERSION_LUNA);

    if (!ShellExecuteA(
        NULL,
        "open",
        updaterExePath,
        commandLine,
        0,
        SW_SHOWNORMAL)
        ) {

        int err;
        err = GetLastError();
        MessageBox(NULL, L"Failed to create updater process.", L"Updater error", MB_OK);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

template<typename T>
class ScopeExit
{
public:
	ScopeExit(T func) : m_func(std::move(func))
    { }
	~ScopeExit()
    {
        m_func();
	}
private:
	T m_func;
};

static std::string latestRelease(void)
{
    LPCWSTR server = L"api.github.com";
    LPCWSTR resource = L"repos/Luna-Project64/Luna-Project64/releases/latest";

    HINTERNET hSession = WinHttpOpen(L"octokit.js/v5.0.3",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession)
        return {};

    ScopeExit hSessionCleanup([&]() { WinHttpCloseHandle(hSession); });

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        server,
        INTERNET_DEFAULT_HTTPS_PORT,
        0);
    if (!hConnect)
        return {};

    ScopeExit hConnectCleanup([&]() { WinHttpCloseHandle(hConnect); });
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        resource,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );
    if (!hRequest)
        return {};

    ScopeExit hRequestCleanup([&]() { WinHttpCloseHandle(hRequest); });
    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    ))
        return {};

    if (!WinHttpReceiveResponse(hRequest, NULL))
        return {};

    std::string result;
	while (result.size() < 1000000)
    {
        DWORD dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
            break;
        }

        if (!dwSize) break;

        size_t loc = result.size();
        if (loc + dwSize > 1000000)
			break;

		result.resize(loc + dwSize);
		char* buffer = result.data() + loc;

        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded)) {
            break;
        }
    }

    return result;
}

static void DoCheckUpdates(void)
try
{
    CoInitialize(nullptr);

    std::string latest = latestRelease();
    if (latest.empty())
    {
        return;
    }

    rapidjson::Document doc;
    doc.Parse(latest.c_str());
    std::string tag = doc["tag_name"].GetString();

    tag.erase(0, 1); // Remove the leading 'v'

    std::istringstream tagStream(tag);
    int versions[3];
    int cursor = 0;
    std::string versionPart;
    while (std::getline(tagStream, versionPart, '.'))
    {
        versions[cursor++] = std::stoi(versionPart);
        if (cursor == 3)
            break;
    }

    if (cursor != 3)
        return;

    bool needUpdate = versions[0] > VERSION_LUNA_MAJOR
                   || versions[1] > VERSION_LUNA_MINOR
		           || versions[2] > VERSION_LUNA_PATCH;
	
    if (needUpdate)
    {
		TriggerUpdater();
    }

    CoUninitialize();
}
catch (...)
{
    CoUninitialize();
}

void CheckUpdatesGitHub(void)
{
    std::thread UpdaterThread(DoCheckUpdates);
	UpdaterThread.detach();
}
