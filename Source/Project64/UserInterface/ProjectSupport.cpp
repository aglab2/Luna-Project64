#include "stdafx.h"
#include <Common/md5.h>
#include <time.h>
#include <windows.h>
#include <Wininet.h>

#pragma comment(lib, "Wininet.lib")

CProjectSupport::CProjectSupport() :
    m_SupportInfo({0})
{
    LoadSupportInfo();
}

bool CProjectSupport::RequestCode(const char * Email)
{
    if (Email == nullptr || strlen(Email) == 0 || _stricmp(Email, "thank you from project64") == 0)
    {
        return false;
    }
    stdstr_f PostData("task=RequestCode&email=%s&machine=%s", Email, MachineID());
    std::vector<std::string> Headers;
    DWORD StatusCode;
    if (!PerformRequest(L"/index.php?option=com_project64", PostData, StatusCode, Headers))
    {
        return false;
    }
    if (StatusCode == 200)
    {
        for (size_t i = 0, n = Headers.size(); i < n; i++)
        {
            if (strcmp(Headers[i].c_str(), "Email: Sent") == 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool CProjectSupport::ValidateCode(const char * Code)
{
    stdstr_f PostData("task=ValidateCode&code=%s&machine=%s", Code, MachineID());
    std::vector<std::string> Headers;
    DWORD StatusCode;
    if (PerformRequest(L"/index.php?option=com_project64", PostData, StatusCode, Headers) && StatusCode == 200)
    {
        std::string Name, Email;
        struct
        {
            const char * Key;
            std::string & Value;
        }
        Test[] =
        {
            { "SupporterName: ", Name },
            { "SupporterEmail: ", Email },
        };

        for (size_t i = 0, n = Headers.size(); i < n; i++)
        {
            for (size_t t = 0; t < sizeof(Test) / sizeof(Test[0]); t++)
            {
                size_t KeyLen = strlen(Test[t].Key);
                if (strncmp(Headers[i].c_str(), Test[t].Key, KeyLen) == 0)
                {
                    Test[t].Value = stdstr(&Headers[i][KeyLen]).Trim();
                    break;
                }
            }
        }

        if (Email.length() > 0)
        {
            strncpy(m_SupportInfo.Code, Code, sizeof(m_SupportInfo.Code));
            strncpy(m_SupportInfo.Email, Email.c_str(), sizeof(m_SupportInfo.Email));
            strncpy(m_SupportInfo.Name, Name.c_str(), sizeof(m_SupportInfo.Name));
            m_SupportInfo.Validated = true;
            SaveSupportInfo();
        }
    } 
    return m_SupportInfo.Validated;
}

std::string CProjectSupport::GenerateMachineID(void)
{
    wchar_t ComputerName[256];
    DWORD Length = sizeof(ComputerName) / sizeof(ComputerName[0]);
    GetComputerName(ComputerName, &Length);

    wchar_t SysPath[MAX_PATH] = { 0 }, VolumePath[MAX_PATH] = { 0 };
    GetSystemDirectory(SysPath, sizeof(SysPath) / sizeof(SysPath[0]));

    GetVolumePathName(SysPath, VolumePath, sizeof(VolumePath) / sizeof(VolumePath[0]));

    DWORD SerialNumber = 0;
    GetVolumeInformation(VolumePath, nullptr, NULL, &SerialNumber, nullptr, nullptr, nullptr, NULL);

    wchar_t MachineGuid[200] = { 0 };
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        DWORD Type, dwDataSize = sizeof(MachineGuid);
        RegQueryValueEx(hKey, L"MachineGuid", nullptr, &Type, (LPBYTE)MachineGuid, &dwDataSize);
        RegCloseKey(hKey);
    }

    stdstr_f Machine("%s.%ud.%s", stdstr().FromUTF16(ComputerName).c_str(), SerialNumber, stdstr().FromUTF16(MachineGuid).c_str());
    return std::string(MD5((const unsigned char *)Machine.c_str(), Machine.size()).hex_digest());
}

void CProjectSupport::IncrementRunCount()
{
    time_t now = time(nullptr);
    if (m_SupportInfo.LastUpdated <= now && ((now - m_SupportInfo.LastUpdated) / 60) < 60)
    {
        return;
    }
    m_SupportInfo.RunCount += 1;
    m_SupportInfo.LastUpdated = now;
    SaveSupportInfo();
}

bool CProjectSupport::ShowSuppotWindow()
{
    return false;
}

bool CProjectSupport::PerformRequest(const wchar_t * Url, const std::string & PostData, DWORD & StatusCode, std::vector<std::string> & Headers)
{
    StatusCode = 0;
    Headers.clear();

    HINTERNET hSession = InternetOpen(L"Project64", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (hSession == nullptr)
    {
        return false;
    }

    HINTERNET hConnect = InternetConnect(hSession, L"www.pj64-emu.com", INTERNET_DEFAULT_HTTPS_PORT, L"", L"", INTERNET_SERVICE_HTTP, 0, 0);
    if (hConnect == nullptr)
    {
        return false;
    }
    LPCWSTR lpszAcceptTypes[] =
    {
        L"text/*",
        nullptr
    };
    HINTERNET hRequest = HttpOpenRequest(hConnect, L"POST", Url, nullptr, nullptr, lpszAcceptTypes, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE, (LPARAM)0);
    if (hRequest == nullptr)
    {
        InternetCloseHandle(hRequest);
        return false;
    }
    wchar_t hdheaders[] = _T("Content-Type: application/x-www-form-urlencoded\r\n");
    BOOL Success = HttpSendRequest(hRequest, hdheaders, _tcslen(hdheaders), (LPVOID)PostData.c_str(), PostData.length());
    if (!Success)
    {
        InternetCloseHandle(hRequest);
        return false;
    }

    DWORD StatusCodeSize = sizeof(StatusCode);
    if (!HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &StatusCode, &StatusCodeSize, nullptr))
    {
        InternetCloseHandle(hRequest);
        return false;
    }

    DWORD dwSize = 0;
    if (!HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, nullptr, &dwSize, nullptr) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        std::vector<uint8_t> RawHeaderData(dwSize);
        if (!HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, RawHeaderData.data(), &dwSize, nullptr))
        {
            InternetCloseHandle(hRequest);
            return false;
        }
        strvector RawHeader = stdstr().FromUTF16((wchar_t *)RawHeaderData.data()).Tokenize("\n");
        for (size_t i = 0, n = RawHeader.size(); i < n; i++)
        {
            RawHeader[i].Replace("\r", "");
            RawHeader[i].Trim();
            if (RawHeader[i].length() > 0)
            {
                Headers.push_back(RawHeader[i]);
            }
        }
    }
    return true;
}

void CProjectSupport::SaveSupportInfo(void)
{
}

void CProjectSupport::LoadSupportInfo(void)
{
}
