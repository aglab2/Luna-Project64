// Courtesy of Parallel Launcher project, by devwizard

#include "stdafx.h"

#include "SummerCart.h"

#include "N64Rom.h"
#include "SystemGlobals.h"

#include <Windows.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <shlwapi.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>

#define S8 3

std::wstring CSummerCart::s_SdPath;
std::wstring CSummerCart::s_VhdPath;

uint8_t* CSummerCart::SDAddr(size_t size)
{
    uint32_t addr = m_Data0 & 0x1fffffff;
    if (addr >= 0x1ffe0000 && addr + size < 0x1ffe0000 + 8192)
    {
        return m_Buffer + (addr - 0x1ffe0000);
    }
    if (addr >= 0x10000000 && addr + size < 0x10000000 + 0x4000000)
    {
        return g_Rom->GetRomAddress() + (addr - 0x10000000);
    }
    return NULL;
}

uint32_t CSummerCart::Init()
{
    if (m_SdFile)
	{
		fclose(m_SdFile);
		m_SdFile = NULL;
	}

    m_SdFile = _wfopen(s_SdPath.c_str(), L"r+b");
    if (!m_SdFile)
    {
        if (!m_WarnedFailedToOpen)
        {
            m_WarnedFailedToOpen = true;
			MessageBox(NULL, L"Failed to open SD card image", L"Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
        }
        return 0x40000000;
    }
    fseek(m_SdFile, 0, SEEK_END);
    m_SdSize = ftell(m_SdFile);
    return 0;
}

uint32_t CSummerCart::Read()
{
    size_t i;
    uint8_t* ptr;
    long offset = 512 * m_SdSector;
    size_t size = 512 * m_Data1;
    if (offset + size > m_SdSize) return 0x40000000;
    if (!(ptr = SDAddr(size))) return 0x40000000;
    if (!m_SdFile) return 0x40000000;

    fseek(m_SdFile, offset, SEEK_SET);
    for (i = 0; i < size; ++i)
    {
        int c = fgetc(m_SdFile);
        if (c < 0)
        {
            return 0x40000000;
        }
        ptr[i ^ m_SdByteswap ^ S8] = c;
    }

    return 0;
}

uint32_t CSummerCart::Write()
{
    size_t i;
    uint8_t* ptr;
    long offset = 512 * m_SdSector;
    size_t size = 512 * m_Data1;
    if (offset + size > m_SdSize) return 0x40000000;
    if (!(ptr = SDAddr(size))) return 0x40000000;
    if (!m_SdFile) return 0x40000000;
    fseek(m_SdFile, offset, SEEK_SET);
    for (i = 0; i < size; ++i)
    {
        int c = fputc(ptr[i ^ S8], m_SdFile);
        if (c < 0)
        {
            return 0x40000000;
        }
    }

    return 0;
}

static void write_fat16_initial_image(const TCHAR* path)
{
    BYTE pt0[] = { 0xEB, 0x3C, 0x90, 0x6D, 0x6B, 0x64, 0x6F, 0x73, 0x66, 0x73, 0x00, 0x00, 0x02, 0x08, 0x01, 0x00
                 , 0x02, 0x00, 0x02, 0x00, 0x00, 0xF8, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                 , 0x00, 0xF8, 0x07, 0x00, 0x00, 0x00, 0x29, 0xD1, 0x49, 0x4B, 0x66, 0x53, 0x44, 0x43, 0x41, 0x52
                 , 0x44, 0x30, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x31, 0x36, 0x20, 0x20, 0x20, 0x0E, 0x1F
                 , 0xBE, 0x5B, 0x7C, 0xAC, 0x22, 0xC0, 0x74, 0x0B, 0x56, 0xB4, 0x0E, 0xBB, 0x07, 0x00, 0xCD, 0x10
                 , 0x5E, 0xEB, 0xF0, 0x32, 0xE4, 0xCD, 0x16, 0xCD, 0x19, 0xEB, 0xFE, 0x54, 0x68, 0x69, 0x73, 0x20
                 , 0x69, 0x73, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6F, 0x6F, 0x74, 0x61, 0x62, 0x6C
                 , 0x65, 0x20, 0x64, 0x69, 0x73, 0x6B, 0x2E, 0x20, 0x20, 0x50, 0x6C, 0x65, 0x61, 0x73, 0x65, 0x20
                 , 0x69, 0x6E, 0x73, 0x65, 0x72, 0x74, 0x20, 0x61, 0x20, 0x62, 0x6F, 0x6F, 0x74, 0x61, 0x62, 0x6C
                 , 0x65, 0x20, 0x66, 0x6C, 0x6F, 0x70, 0x70, 0x79, 0x20, 0x61, 0x6E, 0x64, 0x0D, 0x0A, 0x70, 0x72
                 , 0x65, 0x73, 0x73, 0x20, 0x61, 0x6E, 0x79, 0x20, 0x6B, 0x65, 0x79, 0x20, 0x74, 0x6F, 0x20, 0x74
                 , 0x72, 0x79, 0x20, 0x61, 0x67, 0x61, 0x69, 0x6E, 0x20, 0x2E, 0x2E, 0x2E, 0x20, 0x0D, 0x0A, 0x00 };
    BYTE pt1fe[] = { 0x55, 0xAA, 0xF8, 0xFF, 0xFF, 0xFF };
    BYTE pt20000[] = { 0xF8, 0xFF, 0xFF, 0xFF };
    BYTE pt3fe00[] = { 0x53, 0x44, 0x43, 0x41, 0x52, 0x44, 0x30, 0x20, 0x20, 0x20, 0x20, 0x08, 0x00, 0x00, 0x44, 0xA8, 0xB4, 0x58, 0xB4, 0x58, 0x00, 0x00, 0x44, 0xA8, 0xB4, 0x58 };

    int fd = _wopen(path, O_CREAT | O_WRONLY | O_BINARY, 0666);

    _write(fd, pt0, sizeof(pt0));
    _lseek(fd, 0x1FE, SEEK_SET);
    _write(fd, pt1fe, sizeof(pt1fe));
    _lseek(fd, 0x20000, SEEK_SET);
    _write(fd, pt20000, sizeof(pt20000));
    _lseek(fd, 0x3FE00, SEEK_SET);
    _write(fd, pt3fe00, sizeof(pt3fe00));

    _lseek(fd, 267386880, SEEK_SET);
    SetEndOfFile((HANDLE)_get_osfhandle(fd));

    close(fd);
}

void CSummerCart::MakeInitialImage()
{
    TCHAR _strPath[1024];

    SHGetFolderPath(NULL,
        CSIDL_APPDATA,
        NULL,
        0,
        _strPath);

    PathAppend(_strPath, L"Luna-Project64");
    CreateDirectory(_strPath, NULL); // can fail, ignore errors

    TCHAR strPath2[1024];
    wcscpy(strPath2, _strPath);

    PathAppend(_strPath, L"AUTO0.iso");
    PathAppend(strPath2, L"AUTO0.vhd");

    if (_waccess(_strPath, 0) && _waccess(strPath2, 0))
    {
        write_fat16_initial_image(_strPath);
    };

    s_SdPath = _strPath;
    s_VhdPath = strPath2;
}

CSummerCart::CSummerCart()
{
    m_SdFile = NULL;
    memset(m_Buffer, 0, 8192);
    m_SdSize = -1;
    m_Status = 0;
    m_Data0 = 0;
    m_Data1 = 0;
    m_SdSector = 0;
    m_CfgRomWrite = 0;
    m_SdByteswap = 0;
    m_Unlock = 0;
    m_LockSeq = 0;
}

CSummerCart::~CSummerCart()
{
	if (m_SdFile)
	{
		fclose(m_SdFile);
	}
}

int CSummerCart::ReadRegs(uint32_t address, uint32_t* value)
{
    uint32_t addr = address & 0xFFFF;

    *value = 0;

    if (!m_Unlock) return 0;

    switch (address & 0xFFFF)
    {
    case 0x00:  *value = m_Status; break;
    case 0x04:  *value = m_Data0;  break;
    case 0x08:  *value = m_Data1;  break;
    case 0x0C:  *value = 0x53437632;            break;
    }

    return 0;
}

int CSummerCart::WriteRegs(uint32_t address, uint32_t value, uint32_t mask)
{
    uint32_t addr = address & 0xFFFF;

    if (addr == 0x10)
    {
        switch (value & mask)
        {
        case 0xFFFFFFFF:
            m_Unlock = 0;
            break;
        case 0x5F554E4C:
            if (m_LockSeq == 0)
            {
                m_LockSeq = 2;
            }
            break;
        case 0x4F434B5F:
            if (m_LockSeq == 2)
            {
                m_Unlock = 1;
                m_LockSeq = 0;
            }
            break;
        default:
            m_LockSeq = 0;
            break;
        }
        return 0;
    }

    if (!m_Unlock) return 0;

    switch (addr)
    {
    case 0x00:
        m_Status = 0;
        switch (value & mask)
        {
        case 'c':
            switch (m_Data0)
            {
            case 1:
                m_Data1 = m_CfgRomWrite;
                break;
            case 3:
                m_Data1 = 0;
                break;
            case 6:
                m_Data1 = 0;
                break;
            default:
                m_Status = 0x40000000;
                break;
            }
            break;
        case 'C':
            switch (m_Data0)
            {
            case 1:
                if (m_Data1)
                {
                    m_Data1 = m_CfgRomWrite;
                    m_CfgRomWrite = 1;
                }
                else
                {
                    m_Data1 = m_CfgRomWrite;
                    m_CfgRomWrite = 0;
                }
                break;
            default:
                m_Status = 0x40000000;
                break;
            }
            break;
        case 'i':
            switch (m_Data1)
            {
            case 0:
                break;
            case 1:
                m_Status = Init();
                break;
            case 4:
                m_SdByteswap = 1;
                break;
            case 5:
                m_SdByteswap = 0;
                break;
            default:
                m_Status = 0x40000000;
                break;
            }
            break;
        case 'I':
            m_SdSector = m_Data0;
            break;
        case 's':
            m_Status = Read();
            break;
        case 'S':
            m_Status = Write();
            break;
        default:
            m_Status = 0x40000000;
            break;
        }
        break;
    case 0x04:
        m_Data0 = value & mask;
        break;
    case 0x08:
        m_Data1 = value & mask;
        break;
    }

    return 0;
}
