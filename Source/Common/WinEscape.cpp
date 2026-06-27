#include "WinEscape.h"

#include <windows.h>
#include <shellapi.h>
#include "WinEscape.h"
#include <Windows.h>
#include <ShObjIdl.h>

#include "StrKit.h"

namespace WinEscape
{
    namespace
    {
        static const COMDLG_FILTERSPEC kAllFiles[] = { { L"All files (*.*)", L"*.*" } };

        void ApplyFilter(IFileDialog* dialog, const std::vector<Filter>& filters)
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

    void OpenNativeFor(const wchar_t* path)
    {
        ShellExecuteW(nullptr, L"open", path, nullptr, nullptr, SW_SHOWNORMAL);
    }

    std::wstring OpenFileDialog(void* hwndOwner, std::vector<Filter> filters, const wchar_t* initialDir)
    {
        IFileOpenDialog* od = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&od))) || od == nullptr)
        {
            return {};
        }
        DWORD options = 0;
        if (SUCCEEDED(od->GetOptions(&options)))
        {
            od->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
        }
        ApplyFilter(od, filters);
        ApplyFolder(od, initialDir);

        std::wstring result;
        if (SUCCEEDED(od->Show(static_cast<HWND>(hwndOwner))))
        {
            result = SingleResult(od);
        }
        od->Release();
        return result;
    }

    std::vector<std::wstring> OpenFilesDialog(void* hwndOwner, std::vector<Filter> filters, const wchar_t* initialDir)
    {
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
        IFileSaveDialog* sd = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&sd))) || sd == nullptr)
        {
            return {};
        }
        DWORD options = 0;
        if (SUCCEEDED(sd->GetOptions(&options)))
        {
            sd->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
        }
        ApplyFilter(sd, filters);
        ApplyFolder(sd, initialDir);
        if (defaultName != nullptr)
        {
            sd->SetFileName(defaultName);
        }

        std::wstring result;
        if (SUCCEEDED(sd->Show(static_cast<HWND>(hwndOwner))))
        {
            result = SingleResult(sd);
        }
        sd->Release();
        return result;
    }

    std::wstring ChooseDirectory(void* hwndOwner, const wchar_t* title, const wchar_t* initialDir)
    {
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

    namespace Utf8
    {
        std::string OpenFileDialog(void* hwndOwner, std::vector<Filter> filters, const char* initialDir)
        {
            return ToUtf8(WinEscape::OpenFileDialog(hwndOwner, std::move(filters), ToWide(initialDir).c_str()));
        }

        std::vector<std::string> OpenFilesDialog(void* hwndOwner, std::vector<Filter> filters, const char* initialDir)
        {
            std::vector<std::wstring> wpaths = WinEscape::OpenFilesDialog(hwndOwner, std::move(filters), ToWide(initialDir).c_str());
            std::vector<std::string> result;
            result.reserve(wpaths.size());
            for (const auto& wp : wpaths)
            {
                result.push_back(ToUtf8(wp));
            }
            return result;
        }

        std::string SaveFileDialog(void* hwndOwner, std::vector<Filter> filters, const char* defaultName, const char* initialDir)
        {
            return ToUtf8(WinEscape::SaveFileDialog(hwndOwner, std::move(filters), ToWide(defaultName).c_str(), ToWide(initialDir).c_str()));
        }

        std::string ChooseDirectory(void* hwndOwner, const wchar_t* title, const char* initialDir)
        {
            return ToUtf8(WinEscape::ChooseDirectory(hwndOwner, title, ToWide(initialDir).c_str()));
        }
    }
}
