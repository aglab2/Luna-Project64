#include "stdafx.h"

#include "SdcardFsUI.h"

#include "ff.h"
#include "diskio.h"

#include <algorithm>
#include <cwctype>
#include <shobjidl.h>

namespace
{
    struct TextInputState
    {
        std::string Title;
        std::string Prompt;
        std::string Value;
    };

    std::wstring FileNameFromPath(const std::wstring & path)
    {
        size_t slashPos = path.find_last_of(L"\\/");
        if (slashPos == std::wstring::npos)
        {
            return path;
        }
        return path.substr(slashPos + 1);
    }

    std::wstring FileNameFromFsPath(const std::string & path)
    {
        auto toWide = [](const std::string & text) -> std::wstring
        {
            if (text.empty())
            {
                return std::wstring();
            }

            int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
            if (sizeNeeded <= 0)
            {
                return std::wstring();
            }

            std::wstring wideText((size_t)sizeNeeded, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], sizeNeeded);
            if (!wideText.empty() && wideText.back() == L'\0')
            {
                wideText.pop_back();
            }
            return wideText;
        };

        size_t slashPos = path.find_last_of('/');
        if (slashPos == std::string::npos)
        {
            return toWide(path);
        }
        return toWide(path.substr(slashPos + 1));
    }

    std::string ToUtf8(const std::wstring & text)
    {
        if (text.empty())
        {
            return std::string();
        }

        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0)
        {
            return std::string();
        }

        std::string utf8Text((size_t)sizeNeeded, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8Text[0], sizeNeeded, nullptr, nullptr);

        if (!utf8Text.empty() && utf8Text.back() == '\0')
        {
            utf8Text.pop_back();
        }
        return utf8Text;
    }

    void Trim(std::string & text)
    {
        while (!text.empty() && isspace(text.front()))
        {
            text.erase(text.begin());
        }
        while (!text.empty() && isspace(text.back()))
        {
            text.pop_back();
        }
    }

    INT_PTR CALLBACK TextInputDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
            TextInputState * state = reinterpret_cast<TextInputState *>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            if (state != nullptr)
            {
                SetWindowTextA(hWnd, state->Title.c_str());
                SetDlgItemTextA(hWnd, IDC_SDCARD_PROMPT, state->Prompt.c_str());
                SetDlgItemTextA(hWnd, IDC_SDCARD_INPUT_NAME, state->Value.c_str());
                SendDlgItemMessage(hWnd, IDC_SDCARD_INPUT_NAME, EM_SETSEL, 0, -1);
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK)
            {
                TextInputState * state = reinterpret_cast<TextInputState *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                if (state != nullptr)
                {
                    char inputBuffer[260] = { 0 };
                    GetDlgItemTextA(hWnd, IDC_SDCARD_INPUT_NAME, inputBuffer, sizeof(inputBuffer));
                    state->Value = inputBuffer;
                }
                EndDialog(hWnd, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hWnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        default:
            break;
        }
        return FALSE;
    }

    bool PromptForText(HWND owner, const char * title, const char * prompt, const char * initialValue, std::string & value)
    {
        TextInputState state;
        state.Title = title != nullptr ? title : "";
        state.Prompt = prompt != nullptr ? prompt : "";
        state.Value = initialValue != nullptr ? initialValue : "";

        INT_PTR result = DialogBoxParam(
            _AtlBaseModule.GetResourceInstance(),
            MAKEINTRESOURCE(IDD_SDCARD_NAME_INPUT),
            owner,
            TextInputDialogProc,
            reinterpret_cast<LPARAM>(&state));

        if (result != IDOK)
        {
            return false;
        }

        Trim(state.Value);
        if (state.Value.empty())
        {
            return false;
        }

        value = state.Value;
        return true;
    }

    std::string ParentPathOf(const std::string & path)
    {
        if (path.empty() || path == "/")
        {
            return "/";
        }

        size_t slash = path.find_last_of('/');
        if (slash == std::string::npos || slash == 0)
        {
            return "/";
        }
        return path.substr(0, slash);
    }

    std::vector<std::wstring> ShowOpenFileDialog(HWND owner)
    {
        std::vector<std::wstring> filePaths;
        IFileOpenDialog * openDialog = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&openDialog));
        if (FAILED(hr))
        {
            return filePaths;
        }

        DWORD options = 0;
        if (SUCCEEDED(openDialog->GetOptions(&options)))
        {
            openDialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_ALLOWMULTISELECT);
        }

        const COMDLG_FILTERSPEC filters[] =
        {
            { L"All files (*.*)", L"*.*" },
        };
        openDialog->SetFileTypes(_countof(filters), filters);

        hr = openDialog->Show(owner);
        if (SUCCEEDED(hr))
        {
            IShellItemArray * resultArray = nullptr;
            hr = openDialog->GetResults(&resultArray);
            if (SUCCEEDED(hr) && resultArray != nullptr)
            {
                DWORD count = 0;
                if (SUCCEEDED(resultArray->GetCount(&count)) && count > 0)
                {
                    for (DWORD i = 0; i < count; ++i)
                    {
                        IShellItem * item = nullptr;
                        if (SUCCEEDED(resultArray->GetItemAt(i, &item)) && item != nullptr)
                        {
                            PWSTR selectedPath = nullptr;
                            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath)) && selectedPath != nullptr)
                            {
                                filePaths.emplace_back(selectedPath);
                                CoTaskMemFree(selectedPath);
                            }
                            item->Release();
                        }
                    }
                }
                resultArray->Release();
            }
        }

        openDialog->Release();
        return filePaths;
    }

    bool ShowSaveFileDialog(HWND owner, const std::wstring & suggestedName, std::wstring & filePath)
    {
        IFileSaveDialog * saveDialog = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&saveDialog));
        if (FAILED(hr))
        {
            return false;
        }

        DWORD options = 0;
        if (SUCCEEDED(saveDialog->GetOptions(&options)))
        {
            saveDialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
        }

        const COMDLG_FILTERSPEC filters[] =
        {
            { L"All files (*.*)", L"*.*" },
        };
        saveDialog->SetFileTypes(_countof(filters), filters);
        saveDialog->SetFileName(suggestedName.c_str());

        hr = saveDialog->Show(owner);
        if (SUCCEEDED(hr))
        {
            IShellItem * resultItem = nullptr;
            hr = saveDialog->GetResult(&resultItem);
            if (SUCCEEDED(hr) && resultItem != nullptr)
            {
                PWSTR selectedPath = nullptr;
                hr = resultItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath);
                if (SUCCEEDED(hr) && selectedPath != nullptr)
                {
                    filePath = selectedPath;
                    CoTaskMemFree(selectedPath);
                }
                resultItem->Release();
            }
        }

        saveDialog->Release();
        return !filePath.empty();
    }
}

CSdcardFsUI::CSdcardFsUI()
    : m_CurrentPath("/")
{
	FF::f_mount(&m_Fs, "", 1);
}

CSdcardFsUI::~CSdcardFsUI()
{
    if (m_FileList.m_hWnd != nullptr)
    {
        m_FileList.DeleteAllItems();
    }
    FF::f_unmount("");
	FF::disk_ioctl(0, CTRL_EJECT, nullptr);
}

void CSdcardFsUI::Display(void * ParentWindow)
{
    BOOL result = m_thunk.Init(nullptr, nullptr);
    if (result)
    {
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
#ifdef _DEBUG
        m_bModal = true;
#endif //_DEBUG
        ::DialogBoxParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDD), (HWND)ParentWindow, StartDialogProc, NULL);
    }
}

LRESULT CSdcardFsUI::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetWindowText(L"SD Card Filesystem");

    m_FileList.Attach(GetDlgItem(IDC_SDCARD_LIST));
    m_PathLabel.Attach(GetDlgItem(IDC_SDCARD_PATH_LABEL));

    m_FileList.ModifyStyle(0, LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER);
    m_FileList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_FileList.InsertColumn(0, L"Name", LVCFMT_LEFT, 220, 0);

    NavigateToPath("/");
    ResizeControlsToClient();

    return TRUE;
}

LRESULT CSdcardFsUI::OnSize(UINT, WPARAM, LPARAM, BOOL&)
{
    ResizeControlsToClient();
    return 0;
}

LRESULT CSdcardFsUI::OnContextMenu(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND targetWnd = (HWND)wParam;
    if (targetWnd != m_FileList.m_hWnd)
    {
        bHandled = FALSE;
        return 0;
    }

    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    int itemIndex = -1;

    if (pt.x == -1 && pt.y == -1)
    {
        itemIndex = m_FileList.GetNextItem(-1, LVNI_SELECTED);
        if (itemIndex >= 0)
        {
            RECT itemRect;
            if (m_FileList.GetItemRect(itemIndex, &itemRect, LVIR_BOUNDS))
            {
                pt.x = itemRect.left + ((itemRect.right - itemRect.left) / 2);
                pt.y = itemRect.top + ((itemRect.bottom - itemRect.top) / 2);
                ::ClientToScreen(m_FileList, &pt);
            }
        }
        else
        {
            RECT listRect;
            m_FileList.GetClientRect(&listRect);
            pt.x = (listRect.left + listRect.right) / 2;
            pt.y = (listRect.top + listRect.bottom) / 2;
            ::ClientToScreen(m_FileList, &pt);
        }
    }
    else
    {
        itemIndex = ItemFromScreenPoint(pt);
    }

    if (itemIndex >= 0)
    {
        m_FileList.SetItemState(itemIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_SDCARD_UPLOAD, L"Upload...");
    menu.AppendMenu(MF_STRING, ID_SDCARD_DOWNLOAD, L"Download...");
    menu.AppendMenu(MF_STRING, ID_SDCARD_CREATE_DIRECTORY, L"Create directory");
    menu.AppendMenu(MF_STRING, ID_SDCARD_DELETE, L"Delete");

    UINT command = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
    switch (command)
    {
    case ID_SDCARD_UPLOAD:
        OnUploadSelected();
        break;
    case ID_SDCARD_DOWNLOAD:
        OnDownloadSelected();
        break;
    case ID_SDCARD_CREATE_DIRECTORY:
        OnCreateDirectorySelected();
        break;
    case ID_SDCARD_DELETE:
        OnDeleteSelected();
        break;
    default:
        break;
    }

    return 0;
}

LRESULT CSdcardFsUI::OnCloseDialog(WORD, WORD, HWND, BOOL&)
{
    EndDialog(0);
    return 0;
}

LRESULT CSdcardFsUI::OnBackSelected(WORD, WORD, HWND, BOOL&)
{
    NavigateBack();
    return 0;
}

LRESULT CSdcardFsUI::OnUploadCommand(WORD, WORD, HWND, BOOL&)
{
    OnUploadSelected();
    return 0;
}

LRESULT CSdcardFsUI::OnDownloadCommand(WORD, WORD, HWND, BOOL&)
{
    OnDownloadSelected();
    return 0;
}

LRESULT CSdcardFsUI::OnCreateDirectoryCommand(WORD, WORD, HWND, BOOL&)
{
    OnCreateDirectorySelected();
    return 0;
}

LRESULT CSdcardFsUI::OnDeleteCommand(WORD, WORD, HWND, BOOL&)
{
    OnDeleteSelected();
    return 0;
}

LRESULT CSdcardFsUI::OnListItemActivate(NMHDR * phdr)
{
    NMITEMACTIVATE * activateInfo = (NMITEMACTIVATE *)phdr;
    if (activateInfo == nullptr || activateInfo->iItem < 0)
    {
        return 0;
    }

    ListItemData * itemData = GetItemData(activateInfo->iItem);
    if (itemData != nullptr && itemData->IsDirectory)
    {
        NavigateToPath(itemData->Path);
    }

    return 0;
}

LRESULT CSdcardFsUI::OnListItemDeleted(NMHDR * phdr)
{
    NMLISTVIEW * listView = (NMLISTVIEW *)phdr;
    ListItemData * itemData = (ListItemData *)listView->lParam;
    delete itemData;
    return 0;
}

void CSdcardFsUI::ResizeControlsToClient()
{
    if (m_FileList.m_hWnd == nullptr)
    {
        return;
    }

    RECT clientRect;
    GetClientRect(&clientRect);
    const int margin = 8;
    const int buttonHeight = 22;
    const int buttonGap = 6;
    const int rowGap = 6;
    const int pathHeight = 12;

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    const int contentWidth = std::max(0, clientWidth - (margin * 2));
    const int buttonWidth = std::max(60, (contentWidth - (buttonGap * 4)) / 5);

    int x = margin;
    int y = margin;
    ::MoveWindow(GetDlgItem(ID_SDCARD_BACK), x, y, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    ::MoveWindow(GetDlgItem(ID_SDCARD_CREATE_DIRECTORY), x, y, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    ::MoveWindow(GetDlgItem(ID_SDCARD_UPLOAD), x, y, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    ::MoveWindow(GetDlgItem(ID_SDCARD_DOWNLOAD), x, y, buttonWidth, buttonHeight, TRUE);
    x += buttonWidth + buttonGap;
    ::MoveWindow(GetDlgItem(ID_SDCARD_DELETE), x, y, buttonWidth, buttonHeight, TRUE);

    const int listTop = y + buttonHeight + rowGap;
    const int pathTop = std::max(listTop + rowGap, clientHeight - margin - pathHeight);
    const int listHeight = std::max(0, pathTop - rowGap - listTop);

    m_FileList.MoveWindow(margin, listTop, contentWidth, listHeight, TRUE);
    m_PathLabel.MoveWindow(margin, pathTop, contentWidth, pathHeight, TRUE);

    m_FileList.SetColumnWidth(0, std::max(120, contentWidth));
}

void CSdcardFsUI::RefreshCurrentDirectory(const std::string & selectPath /* = std::string() */)
{
    m_FileList.DeleteAllItems();

    std::vector<FsEntry> entries;

    FF::DIR dir;
    FF::FRESULT result = FF::f_opendir(&dir, m_CurrentPath.c_str());
    if (result != FF::FR_OK)
    {
        UpdatePathLabel();
        return;
	}

	FF::FILINFO entry;
	while (FF::FR_OK == FF::f_readdir(&dir, &entry) && entry.fname[0] != '\0')
    {
        entries.push_back(FsEntry{ entry.fname, (entry.fattrib & AM_DIR) != 0 });
    }
	FF::f_closedir(&dir);

    std::sort(entries.begin(), entries.end(), [](const FsEntry & left, const FsEntry & right)
    {
        if (left.IsDirectory != right.IsDirectory)
        {
            return left.IsDirectory > right.IsDirectory;
        }
        return _stricmp(left.Name.c_str(), right.Name.c_str()) < 0;
    });

    int selectedIndex = -1;
    for (size_t i = 0; i < entries.size(); i++)
    {
        const int newIndex = m_FileList.GetItemCount();
        InsertFsEntry(m_CurrentPath, entries[i]);

        if (!selectPath.empty())
        {
            ListItemData * data = GetItemData(newIndex);
            if (data != nullptr && data->Path == selectPath)
            {
                selectedIndex = newIndex;
            }
        }
    }

    if (selectedIndex >= 0)
    {
        m_FileList.SetItemState(selectedIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    UpdatePathLabel();
    ::EnableWindow(GetDlgItem(ID_SDCARD_BACK), m_CurrentPath != "/");
}

void CSdcardFsUI::InsertFsEntry(const std::string & parentPath, const FsEntry & entry)
{
    ListItemData * itemData = new ListItemData;
    itemData->Path = JoinFsPath(parentPath, entry.Name);
    itemData->IsDirectory = entry.IsDirectory;

    std::string displayName = entry.Name;
    if (entry.IsDirectory)
    {
        displayName += "/";
    }

    LVITEMA item = {};
    item.mask = LVIF_TEXT;
    item.iItem = m_FileList.GetItemCount();
    item.iSubItem = 0;
    item.pszText = (LPSTR)displayName.c_str();
    item.state = 0;
    item.stateMask = 0;
    item.iImage = 0;
    item.lParam = 0;
    int index = ::SendMessageA(m_FileList, LVM_INSERTITEMA, 0, (LPARAM)&item);
    m_FileList.SetItemData(index, (DWORD_PTR)itemData);
}

int CSdcardFsUI::ItemFromScreenPoint(POINT pt) const
{
    POINT clientPt = pt;
    ::ScreenToClient(m_FileList, &clientPt);

    LVHITTESTINFO hitInfo;
    memset(&hitInfo, 0, sizeof(hitInfo));
    hitInfo.pt = clientPt;

    return m_FileList.SubItemHitTest(&hitInfo);
}

CSdcardFsUI::ListItemData * CSdcardFsUI::GetItemData(int index) const
{
    if (index < 0)
    {
        return nullptr;
    }

    return (ListItemData *)m_FileList.GetItemData(index);
}

CSdcardFsUI::ListItemData * CSdcardFsUI::GetSelectedItemData() const
{
    return GetItemData(m_FileList.GetNextItem(-1, LVNI_SELECTED));
}

void CSdcardFsUI::NavigateToPath(const std::string & path)
{
    m_CurrentPath = path.empty() ? "/" : path;
    RefreshCurrentDirectory();
}

void CSdcardFsUI::NavigateBack()
{
    if (m_CurrentPath == "/")
    {
        return;
    }

    NavigateToPath(ParentPathOf(m_CurrentPath));
}

void CSdcardFsUI::UpdatePathLabel()
{
    SetWindowTextA(m_PathLabel, FormatDisplayPath(m_CurrentPath).c_str());
}

std::string CSdcardFsUI::FormatDisplayPath(const std::string & path)
{
    if (path.empty() || path == "/")
    {
        return "sdcard:/";
    }
    return "sdcard:" + path;
}

void CSdcardFsUI::OnUploadSelected()
{
    const std::string destinationDir = m_CurrentPath;

    std::vector<std::wstring> selectedHostPaths = ShowOpenFileDialog(m_hWnd);

    for (const auto& selectedHostPath : selectedHostPaths)
    {
        std::wstring selectedFileName = FileNameFromPath(selectedHostPath);
        if (selectedFileName.empty())
        {
            continue;
        }

        const std::string destinationFile = JoinFsPath(destinationDir, ToUtf8(selectedFileName));
        FsUploadFile(selectedHostPath, destinationFile);
    }

    RefreshCurrentDirectory();
}

void CSdcardFsUI::OnDownloadSelected()
{
    ListItemData * selected = GetSelectedItemData();
    if (selected == nullptr)
    {
        MessageBox(L"Select a file item to download.", L"Download", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (selected->IsDirectory)
    {
        MessageBox(L"Select a file item to download.", L"Download", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring defaultName = FileNameFromFsPath(selected->Path);
    if (defaultName.empty())
    {
        defaultName = L"download.bin";
    }

    std::wstring destinationHostPath;
    if (!ShowSaveFileDialog(m_hWnd, defaultName, destinationHostPath))
    {
        return;
    }

    if (FsDownloadFile(selected->Path, destinationHostPath))
    {
        std::wstring message = L"Download complete:\n" + destinationHostPath;
        MessageBox(message.c_str(), L"Download", MB_OK | MB_ICONINFORMATION);
    }
}

void CSdcardFsUI::OnCreateDirectorySelected()
{
    const std::string targetParent = m_CurrentPath;

    std::string directoryName;
    if (!PromptForText(m_hWnd, "Create directory", "Directory name:", "New Folder", directoryName))
    {
        return;
    }

    const std::string newDirectoryPath = JoinFsPath(targetParent, directoryName);

    if (FF::FR_OK == FF::f_mkdir(newDirectoryPath.c_str()))
    {
        RefreshCurrentDirectory(newDirectoryPath);
    }
}

void CSdcardFsUI::OnDeleteSelected()
{
    ListItemData * selected = GetSelectedItemData();
    if (selected == nullptr)
    {
        MessageBox(L"Select an entry to delete.", L"Delete", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string message = "Delete '" + selected->Path + "'?";
    if (MessageBoxA(NULL, message.c_str(), "Delete", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
    {
        return;
    }

    FF::FRESULT result = FF::f_unlink(selected->Path.c_str());
    if (result != FF::FR_OK)
    {
        std::string error = "Unable to delete entry (error " + std::to_string((int)result) + ").";
        MessageBoxA(NULL, error.c_str(), "Delete", MB_OK | MB_ICONERROR);
        return;
    }

    RefreshCurrentDirectory();
}

bool CSdcardFsUI::FsUploadFile(const std::wstring & hostFilePath, const std::string & fsDestinationPath)
{
    FF::FIL fil{};
    FF::FRESULT result = FF::f_open(&fil, fsDestinationPath.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FF::FR_OK)
    {
        return false;
	}

	FILE* inputFile = _wfopen(hostFilePath.c_str(), L"rb");
	std::vector<char> buffer(65536);

    bool uploadOK = true;
	size_t bytesRead = 0;
    while ((bytesRead = fread(buffer.data(), 1, buffer.size(), inputFile)) > 0)
    {
        UINT bytesWritten = 0;
        FF::FRESULT res = FF::f_write(&fil, buffer.data(), (UINT)bytesRead, &bytesWritten);
        if (FF::FR_OK != res || bytesWritten != bytesRead)
        {
            uploadOK = false;
            break;
        }
    }

	fclose(inputFile);
	FF::f_close(&fil);

    return uploadOK;
}

bool CSdcardFsUI::FsDownloadFile(const std::string & fsSourcePath, const std::wstring & hostDestinationPath)
{
    FF::FIL fil{};
	FF::FRESULT result = FF::f_open(&fil, fsSourcePath.c_str(), FA_READ);

	FILE* outFile = _wfopen(hostDestinationPath.c_str(), L"wb");
    std::vector<char> buffer(65536);

    bool downloadOK = true;
	UINT bytesRead = 0;
    while (FF::FR_OK == FF::f_read(&fil, buffer.data(), buffer.size(), &bytesRead))
    {
        if (bytesRead == 0)
        {
            break;
        }
        size_t bytesWritten = fwrite(buffer.data(), 1, bytesRead, outFile);
        if (bytesWritten != bytesRead)
        {
            downloadOK = false;
            break;
		}
    }

    fclose(outFile);
    FF::f_close(&fil);

    return downloadOK;
}

std::wstring CSdcardFsUI::ToWide(const std::string & text)
{
    if (text.empty())
    {
        return std::wstring();
    }

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (sizeNeeded <= 0)
    {
        return std::wstring();
    }

    std::wstring wideText((size_t)sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], sizeNeeded);

    if (!wideText.empty() && wideText.back() == L'\0')
    {
        wideText.pop_back();
    }
    return wideText;
}

std::string CSdcardFsUI::JoinFsPath(const std::string & parent, const std::string & name)
{
    if (parent.empty() || parent == "/")
    {
        return "/" + name;
    }
    return parent + "/" + name;
}
