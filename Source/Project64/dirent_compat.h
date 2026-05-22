/*
MIT License
Copyright (c) 2019 win32ports
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#ifndef __DIRENT_H_9DE6B42C_8D0C_4D31_A8EF_8E4C30E6C46A__
#define __DIRENT_H_9DE6B42C_8D0C_4D31_A8EF_8E4C30E6C46A__

#ifndef _WIN32

#pragma message("this dirent.h implementation is for Windows only!")

#else /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>
#include <stdint.h>
#include <errno.h>

#include <windows.h>

#include <shlwapi.h>

#ifdef _MSC_VER
#pragma comment(lib, "Shlwapi.lib")
#endif

#ifndef NAME_MAX
#define NAME_MAX 260
#endif /* NAME_MAX */

#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#endif /* DT_UNKNOWN */

#ifndef DT_FIFO
#define DT_FIFO 1
#endif /* DT_FIFO */

#ifndef DT_CHR
#define DT_CHR 2
#endif /* DT_CHR */

#ifndef DT_DIR
#define DT_DIR 4
#endif /* DT_DIR */

#ifndef DT_BLK
#define DT_BLK 6
#endif /* DT_BLK */

#ifndef DT_REG
#define DT_REG 8
#endif /* DT_REF */

#ifndef DT_LNK
#define DT_LNK 10
#endif /* DT_LNK */

#ifndef DT_SOCK
#define DT_SOCK 12
#endif /* DT_SOCK */

#ifndef DT_WHT
#define DT_WHT 14
#endif /* DT_WHT */

#ifndef _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_NAMLEN 1
#endif /* _DIRENT_HAVE_D_NAMLEN */

#ifndef _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_RECLEN 1
#endif /* _DIRENT_HAVE_D_RECLEN */

#ifndef _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_OFF 1
#endif /* _DIRENT_HAVE_D_OFF */

#ifndef _DIRENT_HAVE_D_TYPE 
#define _DIRENT_HAVE_D_TYPE 1
#endif /* _DIRENT_HAVE_D_TYPE  */

#ifndef NTFS_MAX_PATH
#define NTFS_MAX_PATH 32768
#endif /* NTFS_MAX_PATH */

#ifndef FSCTL_GET_REPARSE_POINT
#define FSCTL_GET_REPARSE_POINT 0x900a8
#endif /* FSCTL_GET_REPARSE_POINT */

#ifndef FILE_NAME_NORMALIZED
#define FILE_NAME_NORMALIZED 0
#endif /* FILE_NAME_NORMALIZED */

	typedef void* DIR;

	struct dirent
	{
		unsigned char d_namelen;
		unsigned char d_type;
		wchar_t d_name[NAME_MAX];
	};

	struct __dir
	{
		struct dirent* entries;
		long int count;
		long int index;
	};

	static int closedir(DIR* dirp)
	{
		struct __dir* data = NULL;
		if (!dirp) {
			errno = EBADF;
			return -1;
		}
		data = (struct __dir*)dirp;
		free(data->entries);
		free(data);
		return 0;
	}

	static void __seterrno(int value)
	{
#ifdef _MSC_VER
		_set_errno(value);
#else /* _MSC_VER */
		errno = value;
#endif /* _MSC_VER */
	}

	static int __islink(const wchar_t* name, char* buffer)
	{
		DWORD io_result = 0;
		DWORD bytes_returned = 0;
		HANDLE hFile = CreateFileW(name, 0, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (hFile == INVALID_HANDLE_VALUE)
			return 0;

		io_result = DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0,
			buffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes_returned, NULL);

		CloseHandle(hFile);

		if (io_result == 0)
			return 0;

		return ((REPARSE_GUID_DATA_BUFFER*)buffer)->ReparseTag == IO_REPARSE_TAG_SYMLINK;
	}

	static DIR* __internal_opendir(wchar_t* wname, int size)
	{
		struct __dir* data = NULL;
		struct dirent* tmp_entries = NULL;
		static char default_char = '?';
		static wchar_t* prefix = L"\\\\?\\";
		static wchar_t* suffix = L"\\*.*";
		static int extra_prefix = 4; /* use prefix "\\?\" to handle long file names */
		static int extra_suffix = 4; /* use suffix "\*.*" to find everything */
		WIN32_FIND_DATAW w32fd = { 0 };
		HANDLE hFindFile = INVALID_HANDLE_VALUE;
		static int grow_factor = 2;
		char* buffer = NULL;

		BOOL relative = PathIsRelativeW(wname + extra_prefix);

		memcpy(wname + size - 1, suffix, sizeof(wchar_t) * extra_suffix);
		wname[size + extra_suffix - 1] = 0;

		if (relative) {
			wname += extra_prefix;
			size -= extra_prefix;
		}
		hFindFile = FindFirstFileW(wname, &w32fd);
		if (INVALID_HANDLE_VALUE == hFindFile)
		{
			__seterrno(ENOENT);
			return NULL;
		}

		data = (struct __dir*)malloc(sizeof(struct __dir));
		if (!data)
			goto out_of_memory;
		wname[size - 1] = L'\\';
		data->count = 16;
		data->index = 0;
		data->entries = (struct dirent*)malloc(sizeof(struct dirent) * data->count);
		if (!data->entries)
			goto out_of_memory;
		buffer = (char*) malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		if (!buffer)
			goto out_of_memory;
		do
		{
			memcpy(data->entries[data->index].d_name, w32fd.cFileName, sizeof(wchar_t) * NAME_MAX);
			memcpy(wname + size, w32fd.cFileName, sizeof(wchar_t) * NAME_MAX);

			if (((w32fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT) && __islink(wname, buffer))
				data->entries[data->index].d_type = DT_LNK;
			else if ((w32fd.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) == FILE_ATTRIBUTE_DEVICE)
				data->entries[data->index].d_type = DT_CHR;
			else if ((w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				data->entries[data->index].d_type = DT_DIR;
			else
				data->entries[data->index].d_type = DT_REG;

			data->entries[data->index].d_namelen = (unsigned char)wcslen(w32fd.cFileName);

			if (++data->index == data->count) {
				tmp_entries = (struct dirent*)realloc(data->entries, sizeof(struct dirent) * data->count * grow_factor);
				if (!tmp_entries)
					goto out_of_memory;
				data->entries = tmp_entries;
				data->count *= grow_factor;
			}
		} while (FindNextFileW(hFindFile, &w32fd) != 0);

		free(buffer);
		FindClose(hFindFile);

		data->count = data->index;
		data->index = 0;
		return (DIR*)data;
	out_of_memory:
		if (data)
		{
			free(data->entries);
		}
		free(buffer);
		free(data);
		if (INVALID_HANDLE_VALUE != hFindFile)
			FindClose(hFindFile);
		__seterrno(ENOMEM);
		return NULL;
	}

	static wchar_t* __get_buffer()
	{
		wchar_t* name = (wchar_t*)malloc(sizeof(wchar_t) * (NTFS_MAX_PATH + NAME_MAX + 8));
		if (name)
			memcpy(name, L"\\\\?\\", sizeof(wchar_t) * 4);
		return name;
	}

	static inline DIR* opendir(const char* name)
	{
		DIR* dirp = NULL;
		wchar_t* wname = __get_buffer();
		int size = 0;
		if (!wname)
		{
			errno = ENOMEM;
			return NULL;
		}
		size = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname + 4, NTFS_MAX_PATH);
		if (0 == size)
		{
			free(wname);
			return NULL;
		}
		dirp = __internal_opendir(wname, size + 4);
		free(wname);
		return dirp;
	}

	static DIR* _wopendir(const wchar_t* name)
	{
		DIR* dirp = NULL;
		wchar_t* wname = __get_buffer();
		int size = 0;
		if (!wname)
		{
			errno = ENOMEM;
			return NULL;
		}
		size = (int)wcslen(name);
		if (size > NTFS_MAX_PATH)
		{
			free(wname);
			return NULL;
		}
		memcpy(wname + 4, name, sizeof(wchar_t) * (size + 1));
		dirp = __internal_opendir(wname, size + 5);
		free(wname);
		return dirp;
	}

	static struct dirent* readdir(DIR* dirp)
	{
		struct __dir* data = (struct __dir*)dirp;
		if (!data) {
			errno = EBADF;
			return NULL;
		}
		if (data->index < data->count)
		{
			return &data->entries[data->index++];
		}
		return NULL;
	}

	static void seekdir(DIR* dirp, long int offset)
	{
		if (dirp)
		{
			struct __dir* data = (struct __dir*)dirp;
			data->index = (offset < data->count) ? offset : data->index;
		}
	}

	static inline void rewinddir(DIR* dirp)
	{
		seekdir(dirp, 0);
	}

	static inline long int telldir(DIR* dirp)
	{
		if (!dirp) {
			errno = EBADF;
			return -1;
		}
		return ((struct __dir*)dirp)->count;
	}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WIN32 */

#endif /* __DIRENT_H_9DE6B42C_8D0C_4D31_A8EF_8E4C30E6C46A__ */