#include "WinEscape.h"

#include <windows.h>
#include <shellapi.h>
#include <ShObjIdl.h>
#include <commdlg.h>

#include "StrKit.h"

static bool g_IsWine;

namespace WinEscape
{
    namespace
    {
        typedef void (WINAPI*WideOpenNativeForFn)(const wchar_t* path);
        typedef void (WINAPI*FreeFn)(void* ptr);
        struct Files
        {
            wchar_t** paths;
            int count;
        };
        typedef void(WINAPI* WideOpenFilesDialogFn)(void* hwndOwner, Wide::Filter* filters, int filtersCount, const wchar_t* initialDir, struct Files*);
        typedef wchar_t* (WINAPI* WideSaveFileDialogFn)(void* hwndOwner, Wide::Filter* filters, int filtersCount, const wchar_t* defaultName, const wchar_t* initialDir);
        typedef wchar_t* (WINAPI* WideChooseDirectoryFn)(void* hwndOwner, const wchar_t* title, const wchar_t* initialDir);

        typedef void (WINAPI*Utf8OpenNativeForFn)(const char* path);
        typedef char* (WINAPI*Utf8OpenFileDialogFn)(void* hwndOwner, bool fileMustExist, Utf8::Filter* filters, int filtersCount, const char* initialDir);
        struct Utf8Files
        {
            char** paths;
            int count;
        };
        typedef void(WINAPI*Utf8OpenFilesDialogFn)(void* hwndOwner, Utf8::Filter* filters, int filtersCount, const char* initialDir, struct Files*);
        typedef char* (WINAPI*Utf8SaveFileDialogFn)(void* hwndOwner, Utf8::Filter* filters, int filtersCount, const char* defaultName, const char* initialDir);
        typedef char* (WINAPI*Utf8ChooseDirectoryFn)(void* hwndOwner, const wchar_t* title, const char* initialDir);

        static WideOpenNativeForFn gWideOpenNativeFor = nullptr;
        static FreeFn gFree = nullptr;
        static WideOpenFilesDialogFn gWideOpenFilesDialog = nullptr;
        static WideSaveFileDialogFn gWideSaveFileDialog = nullptr;
        static WideChooseDirectoryFn gWideChooseDirectory = nullptr;
        static Utf8OpenNativeForFn gUtf8OpenNativeFor = nullptr;
        static Utf8OpenFileDialogFn gUtf8OpenFileDialog = nullptr;
        static Utf8SaveFileDialogFn gUtf8SaveFileDialog = nullptr;
        static Utf8ChooseDirectoryFn gUtf8ChooseDirectory = nullptr;

        static const COMDLG_FILTERSPEC kAllFiles[] = { { L"All files (*.*)", L"*.*" } };

        void ApplyFilter(IFileDialog* dialog, const std::vector<Wide::Filter>& filters)
        {
            if (!filters.empty())
            {
                std::vector<COMDLG_FILTERSPEC> specs;
                specs.reserve(filters.size());
                for (const auto& f : filters)
                {
                    specs.push_back({ f.name, f.extensions });
                }
                dialog->SetFileTypes((UINT)specs.size(), specs.data());
            }
            else
            {
                dialog->SetFileTypes(_countof(kAllFiles), kAllFiles);
            }
        }

        void ApplyFolder(IFileDialog* dialog, const wchar_t* initialDir)
        {
            if (initialDir == nullptr)
            {
                return;
            }
            IShellItem* folder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(initialDir, nullptr, IID_PPV_ARGS(&folder))))
            {
                dialog->SetFolder(folder);
                folder->Release();
            }
        }

        std::wstring PathFromItem(IShellItem* item)
        {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr)
            {
                std::wstring result = path;
                CoTaskMemFree(path);
                return result;
            }
            return {};
        }

        std::wstring SingleResult(IFileDialog* dialog)
        {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item)) && item != nullptr)
            {
                std::wstring path = PathFromItem(item);
                item->Release();
                return path;
            }
            return {};
        }

        std::vector<std::wstring> MultiResult(IFileOpenDialog* dialog)
        {
            std::vector<std::wstring> results;
            IShellItemArray* items = nullptr;
            if (SUCCEEDED(dialog->GetResults(&items)) && items != nullptr)
            {
                DWORD count = 0;
                items->GetCount(&count);
                for (DWORD i = 0; i < count; ++i)
                {
                    IShellItem* item = nullptr;
                    if (SUCCEEDED(items->GetItemAt(i, &item)) && item != nullptr)
                    {
                        std::wstring path = PathFromItem(item);
                        if (!path.empty())
                        {
                            results.push_back(std::move(path));
                        }
                        item->Release();
                    }
                }
                items->Release();
            }
            return results;
        }
    }

    struct CoInitializer
    {
        CoInitializer()
        {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        }
        ~CoInitializer()
        {
            CoUninitialize();
        }
    };

    namespace Wide
    {
        void OpenNativeFor(const wchar_t* path)
        {
            if (gWideOpenNativeFor)
            {
                return gWideOpenNativeFor(path);
            }

            ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
        }

        std::vector<std::wstring> OpenFilesDialog(void* hwndOwner, std::vector<Filter> filters, const wchar_t* initialDir)
        {
            if (gWideOpenFilesDialog)
            {
                struct Files files = {};
                gWideOpenFilesDialog(hwndOwner, filters.data(), (int)filters.size(), initialDir, &files);
                std::vector<std::wstring> results;
                for (int i = 0; i < files.count; ++i)
                {
                    results.push_back(files.paths[i]);
                }

                gFree(files.paths);
                return results;
            }

            CoInitializer co;
            IFileOpenDialog* od = nullptr;
            if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&od))) || od == nullptr)
            {
                return {};
            }
            DWORD options = 0;
            if (SUCCEEDED(od->GetOptions(&options)))
            {
                od->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT);
            }
            ApplyFilter(od, filters);
            ApplyFolder(od, initialDir);

            std::vector<std::wstring> results;
            if (SUCCEEDED(od->Show(static_cast<HWND>(hwndOwner))))
            {
                results = MultiResult(od);
            }
            od->Release();
            return results;
        }

        std::wstring SaveFileDialog(void* hwndOwner, std::vector<Filter> filters, const wchar_t* defaultName, const wchar_t* initialDir)
        {
            if (gWideSaveFileDialog)
            {
                wchar_t* result = gWideSaveFileDialog(hwndOwner, filters.data(), (int)filters.size(), defaultName, initialDir);
                if (result)
                {
                    std::wstring path = result;
                    gFree(result);
                    return path;
                }
                return {};
            }

            CoInitializer co;
            OPENFILENAMEW openfilename = {};

            wchar_t filePath[MAX_PATH];
            if (defaultName == nullptr)
            {
                filePath[0] = L'\0';
            }
            else
            {
                wsprintf(filePath, defaultName);
            }

            std::wstring filter;
            if (!filters.empty())
            {
                for (const auto& f : filters)
                {
                    filter += f.name;
                    filter += L'\0';
                    filter += f.extensions;
                    filter += L'\0';
                }
                filter += L'\0';
            }

            openfilename.lStructSize = sizeof(openfilename);
            openfilename.hwndOwner = (HWND)hwndOwner;
            openfilename.lpstrFilter = filters.empty() ? L"All files (*.*)\0*.*\0\0" : filter.c_str();
            openfilename.lpstrFile = filePath;
            openfilename.lpstrInitialDir = initialDir;
            openfilename.nMaxFile = MAX_PATH;
            openfilename.Flags = OFN_HIDEREADONLY;

            bool res = GetSaveFileNameW(&openfilename) != 0;
            return res ? filePath : std::wstring();
        }

        std::wstring ChooseDirectory(void* hwndOwner, const wchar_t* title, const wchar_t* initialDir)
        {
            if (gWideChooseDirectory)
            {
                wchar_t* result = gWideChooseDirectory(hwndOwner, title, initialDir);
                if (result)
                {
                    std::wstring path = result;
                    gFree(result);
                    return path;
                }
                return {};
            }

            CoInitializer co;
            IFileOpenDialog* od = nullptr;
            if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&od))) || od == nullptr)
            {
                return {};
            }
            DWORD options = 0;
            if (SUCCEEDED(od->GetOptions(&options)))
            {
                od->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_PICKFOLDERS);
            }
            if (title != nullptr)
            {
                od->SetTitle(title);
            }
            ApplyFolder(od, initialDir);

            std::wstring result;
            if (SUCCEEDED(od->Show(static_cast<HWND>(hwndOwner))))
            {
                result = SingleResult(od);
            }
            od->Release();
            return result;
        }

    }

    static bool isWine(void)
    {
        HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
        if (!ntdll)
            return false;

        return NULL != GetProcAddress(ntdll, "wine_get_version");
    }

    void Init()
    {
        g_IsWine = isWine();
        if (!g_IsWine)
            return;

        HMODULE hWinEscape = LoadLibrary(L"wine_xdg_portal.dll.so");
        if (!hWinEscape)
            return;

        gFree = (FreeFn)GetProcAddress(hWinEscape, "wine_portal_free");
        gWideOpenNativeFor = (WideOpenNativeForFn)GetProcAddress(hWinEscape, "wine_portal_wide_open_native_for");
        gWideOpenFilesDialog = (WideOpenFilesDialogFn)GetProcAddress(hWinEscape, "wine_portal_wide_open_files_dialog");
        gWideSaveFileDialog = (WideSaveFileDialogFn)GetProcAddress(hWinEscape, "wine_portal_wide_save_file_dialog");
        gWideChooseDirectory = (WideChooseDirectoryFn)GetProcAddress(hWinEscape, "wine_portal_wide_choose_directory");
        gUtf8OpenNativeFor = (Utf8OpenNativeForFn)GetProcAddress(hWinEscape, "wine_portal_utf8_open_native_for");
        gUtf8OpenFileDialog = (Utf8OpenFileDialogFn)GetProcAddress(hWinEscape, "wine_portal_utf8_open_file_dialog");
        gUtf8SaveFileDialog = (Utf8SaveFileDialogFn)GetProcAddress(hWinEscape, "wine_portal_utf8_save_file_dialog");
        gUtf8ChooseDirectory = (Utf8ChooseDirectoryFn)GetProcAddress(hWinEscape, "wine_portal_utf8_choose_directory");
    }

    namespace Utf8
    {
        void OpenNativeFor(const char* path)
        {
            if (gUtf8OpenNativeFor)
            {
                return gUtf8OpenNativeFor(path);
            }

            ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOWNORMAL);
        }

        std::string OpenFileDialog(void* hwndOwner, bool fileMustExist, std::vector<Filter> filters, const char* initialDir)
        {
            if (gUtf8OpenFileDialog)
            {
                char* result = gUtf8OpenFileDialog(hwndOwner, fileMustExist, filters.data(), (int)filters.size(), initialDir);
                if (result)
                {
                    std::string path = result;
                    gFree(result);
                    return path;
                }
                return {};
            }

            CoInitializer co;
            OPENFILENAMEA openfilename = {};
            char FileName[MAX_PATH] = {};
            std::string filter;
            if (!filters.empty())
            {
                for (const auto& f : filters)
                {
                    filter += f.name;
                    filter += '\0';
                    filter += f.extensions;
                    filter += '\0';
                }
                filter += '\0';
            }

            openfilename.lStructSize = sizeof(openfilename);
            openfilename.hwndOwner = (HWND)hwndOwner;
            openfilename.lpstrFilter = filters.empty() ? "All files (*.*)\0*.*\0\0" : filter.c_str();
            openfilename.lpstrFile = FileName;
            openfilename.lpstrInitialDir = initialDir;
            openfilename.nMaxFile = MAX_PATH;
            openfilename.Flags = OFN_HIDEREADONLY | (fileMustExist ? OFN_FILEMUSTEXIST : 0);

            bool res = GetOpenFileNameA(&openfilename) != 0;
            return res ? FileName : std::string();
        }

        std::string SaveFileDialog(void* hwndOwner, std::vector<Filter> filters, const char* defaultName, const char* initialDir)
        {
            if (gUtf8SaveFileDialog)
            {
                char* result = gUtf8SaveFileDialog(hwndOwner, filters.data(), (int)filters.size(), defaultName, initialDir);
                if (result)
                {
                    std::string path = result;
                    gFree(result);
                    return path;
                }
                return {};
            }

            CoInitializer co;
            OPENFILENAMEA openfilename = {};

            char filePath[MAX_PATH];
            if (defaultName)
                sprintf(filePath, defaultName);
            else
                filePath[0] = '\0';

            std::string filter;
            if (!filters.empty())
            {
                for (const auto& f : filters)
                {
                    filter += f.name;
                    filter += '\0';
                    filter += f.extensions;
                    filter += '\0';
                }
                filter += '\0';
            }

            openfilename.lStructSize = sizeof(openfilename);
            openfilename.hwndOwner = (HWND)hwndOwner;
            openfilename.lpstrFilter = filters.empty() ? "All files (*.*)\0*.*\0\0" : filter.c_str();
            openfilename.lpstrFile = filePath;
            openfilename.lpstrInitialDir = initialDir;
            openfilename.nMaxFile = MAX_PATH;
            openfilename.Flags = OFN_HIDEREADONLY;

            bool res = GetSaveFileNameA(&openfilename) != 0;
            return res ? filePath : std::string();
        }

        std::string ChooseDirectory(void* hwndOwner, const wchar_t* title, const char* _initialDir)
        {
            if (gUtf8ChooseDirectory)
            {
                char* result = gUtf8ChooseDirectory(hwndOwner, title, _initialDir);
                if (result)
                {
                    std::string path = result;
                    gFree(result);
                    return path;
                }
                return {};
            }

            CoInitializer co;
            std::wstring initialDir;
            if (_initialDir != nullptr)
                initialDir = ToWide(_initialDir);

            return ToUtf8(WinEscape::Wide::ChooseDirectory(hwndOwner, title, _initialDir ? initialDir.c_str() : nullptr));
        }
    }
}
