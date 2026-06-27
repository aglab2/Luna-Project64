#pragma once
#include <string>
#include <vector>

// Various tools to escape from the emulator window.
// On Windows this is available natively but using Wine causes major pain because wine ui horrible.
// This escape kit tries to smooth out the experience.
namespace WinEscape
{
    namespace Wide
    {
        struct Filter
        {
            const wchar_t* name;
            const wchar_t* extensions;
        };

        // Opens path in its native application.
        void OpenNativeFor(const wchar_t* path);

        // Open-file dialog, multiple selection.
        // Returns vector of selected paths (empty if cancelled).
        std::vector<std::wstring> OpenFilesDialog(void* hwndOwner, std::vector<Filter> filters = {}, const wchar_t* initialDir = nullptr);

        std::wstring SaveFileDialog(void* hwndOwner, std::vector<Filter> filters = {}, const wchar_t* defaultName = nullptr, const wchar_t* initialDir = nullptr);

        // Folder-picker dialog. Returns the selected directory path, or an empty string if cancelled.
        std::wstring ChooseDirectory(void* hwndOwner, const wchar_t* title = nullptr, const wchar_t* initialDir = nullptr);
    }

    namespace Utf8
    {
        struct Filter
        {
            const char* name;
            const char* extensions;
        };

        void OpenNativeFor(const char* path);

        // Open-file dialog, single selection.
        // Returns the selected file path, or an empty string if cancelled.
        std::string OpenFileDialog(void* hwndOwner, bool fileMustExist, std::vector<Filter> filters = {}, const char* initialDir = nullptr);

        // Save-file dialog.
        // Returns the selected file path, or an empty string if cancelled.
        std::string SaveFileDialog(void* hwndOwner, std::vector<Filter> filters = {}, const char* defaultName = nullptr, const char* initialDir = nullptr);

        std::string ChooseDirectory(void* hwndOwner, const wchar_t* title = nullptr, const char* initialDir = nullptr);
    }

    void Init();
}
