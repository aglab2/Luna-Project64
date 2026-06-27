#include "stdafx.h"

#include "SdcardFsUI.h"
#include "Common/StrKit.h"
#include "Common/WinEscape.h"

#include "dirent_compat.h"
#include "ff.h"
#include "diskio.h"

#include <algorithm>
#include <cwctype>

namespace
{
    struct TextInputState
    {
        std::string Title;
        std::string Prompt;
        std::string Value;
    };

    enum class ReplaceDecision
    {
        Skip,
        SkipAll,
        Replace,
        ReplaceAll,
        Cancel,
    };

    struct ReplacePromptState
    {
        std::wstring Title;
        std::wstring Message;
        ReplaceDecision Decision = ReplaceDecision::Cancel;
        bool ShowReplace = true;
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
        size_t slashPos = path.find_last_of('/');
        if (slashPos == std::string::npos)
        {
            return ToWide(path);
        }
        return ToWide(path.substr(slashPos + 1));
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

    INT_PTR CALLBACK ReplacePromptDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
            ReplacePromptState * state = reinterpret_cast<ReplacePromptState *>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

            if (state != nullptr)
            {
                SetWindowTextW(hWnd, state->Title.c_str());
                SetDlgItemTextW(hWnd, IDC_SDCARD_REPLACE_MESSAGE, state->Message.c_str());

                if (!state->ShowReplace)
                {
                    HWND hReplace = GetDlgItem(hWnd, IDC_SDCARD_REPLACE_REPLACE);
                    HWND hReplaceAll = GetDlgItem(hWnd, IDC_SDCARD_REPLACE_REPLACE_ALL);
                    HWND hSkip = GetDlgItem(hWnd, IDC_SDCARD_REPLACE_SKIP);
                    HWND hSkipAll = GetDlgItem(hWnd, IDC_SDCARD_REPLACE_SKIP_ALL);

                    auto GetClientTopLeft = [&](HWND btn, int & x, int & y)
                    {
                        RECT rc;
                        GetWindowRect(btn, &rc);
                        POINT pt = {rc.left, rc.top};
                        ScreenToClient(hWnd, &pt);
                        x = pt.x;
                        y = pt.y;
                    };

                    int rx, ry, rax, ray;
                    GetClientTopLeft(hReplace, rx, ry);
                    GetClientTopLeft(hReplaceAll, rax, ray);

                    SetWindowPos(hSkip, nullptr, rx, ry, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    SetWindowPos(hSkipAll, nullptr, rax, ray, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    ShowWindow(hReplace, SW_HIDE);
                    ShowWindow(hReplaceAll, SW_HIDE);
                }
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            ReplacePromptState * state = reinterpret_cast<ReplacePromptState *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            switch (LOWORD(wParam))
            {
            case IDC_SDCARD_REPLACE_SKIP:
                state->Decision = ReplaceDecision::Skip;
                EndDialog(hWnd, IDOK);
                return TRUE;
            case IDC_SDCARD_REPLACE_SKIP_ALL:
                state->Decision = ReplaceDecision::SkipAll;
                EndDialog(hWnd, IDOK);
                return TRUE;
            case IDC_SDCARD_REPLACE_REPLACE:
                state->Decision = ReplaceDecision::Replace;
                EndDialog(hWnd, IDOK);
                return TRUE;
            case IDC_SDCARD_REPLACE_REPLACE_ALL:
				state->Decision = ReplaceDecision::ReplaceAll;
                EndDialog(hWnd, IDOK);
                return TRUE;
            case IDCANCEL:
				state->Decision = ReplaceDecision::Cancel;
                EndDialog(hWnd, IDCANCEL);
                return TRUE;
            default:
                break;
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

    ReplaceDecision PromptForReplacement(HWND owner, const std::wstring & fileName, const std::wstring& reason, bool showReplace = true)
    {
        ReplacePromptState state;
        state.Title = showReplace ? L"Replace file?" : L"Proceed?";
        state.Message = L"A file named '" + fileName + L"' " + reason + L".\n\nWhat would you like to do?";
        state.ShowReplace = showReplace;

        INT_PTR result = DialogBoxParam(
            _AtlBaseModule.GetResourceInstance(),
            MAKEINTRESOURCE(IDD_SDCARD_REPLACE_CONFIRM),
            owner,
            ReplacePromptDialogProc,
            reinterpret_cast<LPARAM>(&state));

        if (result != IDOK)
        {
            return ReplaceDecision::Cancel;
        }
        return state.Decision;
    }

    bool IsDirectory(const std::wstring & hostPath)
    {
        DWORD attrs = GetFileAttributesW(hostPath.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }
        return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
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

}

CSdcardFsUI::CSdcardFsUI()
    : m_CurrentPath("/")
{
	FF::f_mount(&m_Fs, "", 1);
}

CSdcardFsUI::~CSdcardFsUI()
{
    m_FileList.DeleteAllItems();
    FF::f_unmount("");
	FF::disk_ioctl(0, CTRL_EJECT, nullptr);
}

void CSdcardFsUI::Display(void * ParentWindow)
{
    BOOL result = m_thunk.Init(nullptr, nullptr);
    if (result)
    {
        _AtlWinModule.AddCreateWndData(&m_thunk.cd, this);
        ::DialogBoxParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDD), (HWND)ParentWindow, StartDialogProc, NULL);
    }
}

LRESULT CSdcardFsUI::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    SetWindowText(L"SD Card Filesystem");

    m_FileList.Attach(GetDlgItem(IDC_SDCARD_LIST));
    m_PathLabel.Attach(GetDlgItem(IDC_SDCARD_PATH_LABEL));

    m_FileList.ModifyStyle(0, LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER);
    m_FileList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    m_FileList.InsertColumn(0, L"Name", LVCFMT_LEFT, 220, 0);

    NavigateToPath("/");
    ResizeControlsToClient();

    DragAcceptFiles(true);

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

LRESULT CSdcardFsUI::OnDropFiles(HDROP hDrop)
{
	if (hDrop == nullptr)
	{
		return 0;
	}

	std::vector<std::wstring> filePaths;

	UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	for (UINT i = 0; i < fileCount; i++)
	{
		UINT size = DragQueryFile(hDrop, i, NULL, 0);
		if (size <= 0)
		{
			continue;
		}

		std::wstring filePath(size + 1, L'\0');
		size = DragQueryFile(hDrop, i, &filePath[0], size + 1);
		filePath.resize(size);
		filePaths.push_back(std::move(filePath));
	}

	DragFinish(hDrop);

	UploadSelected(filePaths);
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

std::vector<CSdcardFsUI::ListItemData *> CSdcardFsUI::GetSelectedItemData() const
{
    std::vector<ListItemData *> selectedItems;

    int index = -1;
    while ((index = m_FileList.GetNextItem(index, LVNI_SELECTED)) >= 0)
    {
        ListItemData * itemData = GetItemData(index);
        if (itemData != nullptr)
        {
            selectedItems.push_back(itemData);
        }
    }

    return selectedItems;
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
    return "sdcard:" + path;
}

void CSdcardFsUI::OnUploadSelected()
{
    std::vector<std::wstring> selectedHostPaths = WinEscape::OpenFilesDialog(m_hWnd);
    UploadSelected(selectedHostPaths);
}

void CSdcardFsUI::UploadSelected(const std::vector<std::wstring>& selectedHostPaths)
{
    const std::string& destinationDir = m_CurrentPath;

    bool failPrompts = true;
	bool replacePrompts = true;

    struct Action
    {
        std::wstring from;
        std::string to;
    };

    std::vector<Action> actions;
    for (const auto& selectedHostPath : selectedHostPaths)
    {
        actions.push_back(Action{ selectedHostPath, destinationDir });
    }

    while (!actions.empty())
    {
        Action action = std::move(actions.back());
        actions.pop_back();

		const auto& selectedHostPath = action.from;
        std::wstring selectedFileName = FileNameFromPath(selectedHostPath);
        std::string destinationFile = JoinFsPath(action.to, ToUtf8(selectedFileName));

        // TODO: This is a pretty crazy overkill. I do not believe it is actually possible to select host directory....
        if (IsDirectory(selectedHostPath))
        {
			FF::FRESULT res = FF::f_mkdir(destinationFile.c_str());
            if (res != FF::FR_OK && res != FF::FR_EXIST)
            {
                if (failPrompts)
                {
					auto resx = PromptForReplacement(m_hWnd, selectedHostPath, false);
                    if (resx == ReplaceDecision::Cancel)
                        break;
                    if (resx == ReplaceDecision::SkipAll)
						failPrompts = false;
                }
            }
            else
            {
				DIR* dir = _wopendir(selectedHostPath.c_str());
                if (!dir)
                {
                    if (failPrompts)
                    {
                        auto resx = PromptForReplacement(m_hWnd, selectedHostPath, false);
                        if (resx == ReplaceDecision::Cancel)
                            break;
                        if (resx == ReplaceDecision::SkipAll)
                            failPrompts = false;
                    }
					continue;
                }

                while (auto entry = readdir(dir))
                {
                    if (wcscmp(entry->d_name, L".") == 0 || wcscmp(entry->d_name, L"..") == 0)
                    {
                        continue;
                    }
                    std::wstring childHostPath = selectedHostPath + L"\\" + entry->d_name;
                    std::string childDestinationPath = destinationFile;
                    actions.push_back(Action{ std::move(childHostPath), std::move(childDestinationPath) });
                }
                closedir(dir);
            }
        }
        else
        {
			FF::FRESULT res = FsUploadFile(selectedHostPath, destinationFile, !replacePrompts /*canReplace*/);
            if ((replacePrompts && res == FF::FR_EXIST) || (failPrompts && res != FF::FR_OK))
            {
                const wchar_t* whatHappened = nullptr;
                if (replacePrompts && res == FF::FR_EXIST)
                {
                    whatHappened = L"already exists";
                }
                else if (failPrompts && res != FF::FR_OK)
                {
                    whatHappened = L"failed to upload";
				}

                ReplaceDecision decision = PromptForReplacement(m_hWnd, selectedFileName, whatHappened);
                if (ReplaceDecision::SkipAll == decision)
					failPrompts = false;
				if (ReplaceDecision::ReplaceAll == decision)
					replacePrompts = false;
                if (ReplaceDecision::Cancel == decision)
                    break;

                if (decision == ReplaceDecision::Replace || decision == ReplaceDecision::ReplaceAll)
                {
                    FF::FRESULT resx = FsUploadFile(selectedHostPath, destinationFile, true);
                    if (failPrompts && resx != FF::FR_OK)
                    {
                        decision = PromptForReplacement(m_hWnd, selectedFileName, L"failed to upload", false /*showReplace*/);
                        if (ReplaceDecision::Cancel == decision)
                            break;
                        if (ReplaceDecision::SkipAll == decision)
                            failPrompts = false;
                    }
                }
            }
        }
    }

    RefreshCurrentDirectory();
}

void CSdcardFsUI::OnDownloadSelected()
{
    std::vector<ListItemData *> selectedItems = GetSelectedItemData();
    if (selectedItems.empty())
    {
        MessageBox(L"Select one or more file items to download.", L"Download", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (selectedItems.size() == 1)
    {
        ListItemData * selected = selectedItems[0];

        FF::FILINFO fi;
        FF::f_stat(selected->Path.c_str(), &fi);

        if (!(fi.fattrib & AM_DIR))
        {
            std::wstring defaultName = FileNameFromFsPath(selected->Path);
            std::wstring destinationHostPath = WinEscape::SaveFileDialog(m_hWnd, {}, defaultName.c_str());
            if (destinationHostPath.empty())
                return;

            auto res = FsDownloadFile(selected->Path, destinationHostPath, false);
            if (res == FF::FR_EXIST)
            {
				bool replace = IDYES == MessageBoxA(NULL, ("A file named '" + ToUtf8(destinationHostPath) + "' already exists.").c_str(), "Download", MB_YESNO | MB_ICONWARNING);
                if (replace)
                    res = FsDownloadFile(selected->Path, destinationHostPath, true);
			}

            if (res != FF::FR_OK)
            {
                MessageBoxA(NULL, ("Failed to download the file '" + selected->Path + "'").c_str(), "Download", MB_OK | MB_ICONERROR);
			}

            MessageBox((L"File '" + FileNameFromFsPath(selected->Path) + L"' was downloaded").c_str(), L"Download", MB_OK | MB_ICONINFORMATION);
            return;
        }
    }

    std::wstring destinationFolder = WinEscape::ChooseDirectory(m_hWnd);
    if (destinationFolder.empty())
    {
        return;
    }

    struct Action
    {
        std::string from;
        std::wstring to;
    };
    std::vector<Action> actions;

    for (const auto& item : selectedItems)
    {
        actions.push_back({ item->Path, destinationFolder });
	}

    bool failPrompts = true;
    bool replacePrompts = true;

    while (!actions.empty())
    {
		Action action = std::move(actions.back());
        actions.pop_back();

		const std::string& filePath = action.from;
        std::wstring fileName = FileNameFromFsPath(filePath);
        std::wstring destinationFile = action.to + L"\\" + fileName;

        FF::FILINFO fi;
		FF::FRESULT res = FF::f_stat(filePath.c_str(), &fi);
        if (res == FF::FRESULT::FR_OK && (fi.fattrib & AM_DIR))
        {
			BOOL ok = CreateDirectoryW(destinationFile.c_str(), NULL);
            if (!ok && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                if (failPrompts)
                {
                    auto decision = PromptForReplacement(m_hWnd, destinationFile, false);
                    if (decision == ReplaceDecision::Cancel)
                        break;
                    if (decision == ReplaceDecision::SkipAll)
                        failPrompts = false;
                }
            }
            else
            {
                FF::DIR dir;
                FF::FRESULT resx = FF::f_opendir(&dir, filePath.c_str());
                if (resx != FF::FRESULT::FR_OK)
                {
                    if (failPrompts)
                    {
                        auto decision = PromptForReplacement(m_hWnd, destinationFile, false);
                        if (decision == ReplaceDecision::Cancel)
                            break;
                        if (decision == ReplaceDecision::SkipAll)
                            failPrompts = false;
					}
                }

                while (FF::FR_OK == FF::f_readdir(&dir, &fi) && fi.fname[0] != '\0')
                {
                    if (strcmp(fi.fname, ".") == 0 || strcmp(fi.fname, "..") == 0)
                    {
                        continue;
                    }

                    std::string childCardPath = filePath + "/" + fi.fname;
                    std::wstring childDestinationPath = destinationFile;
                    actions.push_back(Action{ std::move(childCardPath), std::move(childDestinationPath) });
                }
				FF::f_closedir(&dir);
            }
        }
        else
        {
            FF::FRESULT resx = FsDownloadFile(filePath, destinationFile, !replacePrompts /*canReplace*/);
            if ((replacePrompts && resx == FF::FR_EXIST) || (failPrompts && resx != FF::FR_OK))
            {
                const wchar_t* whatHappened = nullptr;
                if (replacePrompts && res == FF::FR_EXIST)
                {
                    whatHappened = L"already exists";
                }
                else if (failPrompts && res != FF::FR_OK)
                {
                    whatHappened = L"failed to upload";
                }

                ReplaceDecision decision = PromptForReplacement(m_hWnd, destinationFile, whatHappened);
                if (ReplaceDecision::SkipAll == decision)
                    failPrompts = false;
                if (ReplaceDecision::ReplaceAll == decision)
                    replacePrompts = false;
                if (ReplaceDecision::Cancel == decision)
                    break;

                if (decision == ReplaceDecision::Replace || decision == ReplaceDecision::ReplaceAll)
                {
                    FF::FRESULT res = FsDownloadFile(filePath, destinationFile, true);
                    if (failPrompts && res != FF::FR_OK)
                    {
                        decision = PromptForReplacement(m_hWnd, destinationFile, L"failed to upload", false /*showReplace*/);
                        if (ReplaceDecision::Cancel == decision)
                            break;
                        if (ReplaceDecision::SkipAll == decision)
                            failPrompts = false;
                    }
                }
            }
        }
    }

    MessageBox(L"Download was finished", L"Download", MB_OK | MB_ICONINFORMATION);
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
    std::vector<ListItemData *> selectedItems = GetSelectedItemData();
    if (selectedItems.empty())
    {
        MessageBox(L"Select one or more entries to delete.", L"Delete", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string message;
    if (selectedItems.size() == 1)
    {
        message = "Delete '" + selectedItems[0]->Path + "'?";
    }
    else
    {
        message = "Delete " + std::to_string(selectedItems.size()) + " selected entries?";
    }

    if (MessageBoxA(NULL, message.c_str(), "Delete", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
    {
        return;
    }

    struct Action
    {
		std::string Path;
		bool Retried;
    };

    std::vector<Action> actions;

    for (ListItemData * selected : selectedItems)
    {
		actions.push_back(Action{ selected->Path, false });
    }

	bool skipPrompts = false;
    while (!actions.empty())
    {
        Action action = actions.back();
        actions.pop_back();

        FF::FRESULT result = FF::f_unlink(action.Path.c_str());
        if (result == FF::FR_OK)
        {
            continue;
        }

        if ((result == FF::FRESULT::FR_DENIED && action.Retried) || (result != FF::FRESULT::FR_DENIED))
        {
			auto decision = PromptForReplacement(m_hWnd, ToWide(action.Path), L"could not be deleted", false);
            if (decision != ReplaceDecision::Cancel)
            {
                break;
			}
            if (decision == ReplaceDecision::SkipAll)
            {
                skipPrompts = true;
            }
        }
		else // FR_DENIED on first try - dir?
        {
            actions.push_back(action);

            FF::DIR dir;
            FF::FILINFO fi;
            FF::FRESULT res = FF::f_opendir(&dir, action.Path.c_str());
            if (res == FF::FRESULT::FR_OK)
            {
                while (FF::FR_OK == FF::f_readdir(&dir, &fi) && fi.fname[0] != '\0')
                {
                    if (strcmp(fi.fname, ".") == 0 || strcmp(fi.fname, "..") == 0)
                    {
                        continue;
                    }
                    actions.push_back(Action{ action.Path + "/" + fi.fname, false });
                }
                FF::f_closedir(&dir);
            }
        }
    }

    RefreshCurrentDirectory();
}

static FF::FRESULT errnoToError()
{
    switch (errno)
    {
    case EACCES:
        return FF::FRESULT::FR_DENIED;
    case ENOENT:
        return FF::FRESULT::FR_NO_FILE;
    case EEXIST:
        return FF::FRESULT::FR_EXIST;
    default:
        return FF::FRESULT::FR_DISK_ERR;
    }
}

FF::FRESULT CSdcardFsUI::FsUploadFile(const std::wstring & hostFilePath, const std::string & fsDestinationPath, bool canReplace)
{
    FF::FIL fil{};
    FF::FRESULT result = FF::f_open(&fil, fsDestinationPath.c_str(), canReplace ? (FA_WRITE | FA_CREATE_ALWAYS) : (FA_WRITE | FA_CREATE_NEW));
    if (result != FF::FR_OK)
    {
        return result;
	}

	FILE* inputFile = _wfopen(hostFilePath.c_str(), L"rb");
    if (!inputFile)
    {
        FF::f_close(&fil);
        return errnoToError();
	}

	std::vector<char> buffer(65536);

	size_t bytesRead = 0;
    while ((bytesRead = fread(buffer.data(), 1, buffer.size(), inputFile)) > 0)
    {
        UINT bytesWritten = 0;
        result = FF::f_write(&fil, buffer.data(), (UINT)bytesRead, &bytesWritten);
        if (FF::FR_OK != result || bytesWritten != bytesRead)
        {
            if (result == FF::FR_OK)
            {
                result = FF::FRESULT::FR_DISK_ERR;
			}
            break;
        }
    }

	fclose(inputFile);
	FF::f_close(&fil);

    return result;
}

FF::FRESULT CSdcardFsUI::FsDownloadFile(const std::string & fsSourcePath, const std::wstring & hostDestinationPath, bool canReplace)
{
    FF::FIL fil{};
	FF::FRESULT result = FF::f_open(&fil, fsSourcePath.c_str(), FA_READ);
    if (FF::FRESULT::FR_OK != result)
    {
        return result;
    }

	FILE* outFile = _wfopen(hostDestinationPath.c_str(), canReplace ? L"wb" : L"wbx");
    if (!outFile)
    {
        FF::f_close(&fil);
		return errnoToError();
    }

    std::vector<char> buffer(65536);

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
            result = FF::FRESULT::FR_DISK_ERR;
            break;
		}
    }

    fclose(outFile);
    FF::f_close(&fil);

    return result;
}

std::string CSdcardFsUI::JoinFsPath(const std::string & parent, const std::string & name)
{
    if (parent.empty() || parent == "/")
    {
        return "/" + name;
    }
    return parent + "/" + name;
}
