#include "stdafx.h"
#include "WelcomeScreen.h"
#include "resource.h"

WelcomeScreen::WelcomeScreen()
{
}


static bool isWine(void)
{
    HMODULE ntdll = GetModuleHandle(L"ntdll.dll");
    if (!ntdll)
        return false;

    return NULL != GetProcAddress(ntdll, "wine_get_version");
}

void WelcomeScreen::SelectGameDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    wchar_t Buffer[MAX_PATH], Directory[MAX_PATH];
    LPITEMIDLIST pidl;
    BROWSEINFO bi;

    stdstr InitialDir = g_Settings->LoadStringVal(RomList_GameDir);
    std::wstring wTitle = L"Select Game Directory";
    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = nullptr;
    bi.pszDisplayName = Buffer;
    bi.lpszTitle = wTitle.c_str();
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
    bi.lParam = (DWORD)InitialDir.c_str();
    if ((pidl = SHBrowseForFolder(&bi)) != nullptr)
    {
        if (SHGetPathFromIDList(pidl, Directory))
        {
            stdstr path;
            CPath SelectedDir(path.FromUTF16(Directory), "");
            if (SelectedDir.DirectoryExists())
            {
                GetDlgItem(IDC_GAME_DIR).SetWindowText(Directory);
            }
        }
    }
}

void WelcomeScreen::SelectPrevDir(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    wchar_t Buffer[MAX_PATH], Directory[MAX_PATH];
    LPITEMIDLIST pidl;
    BROWSEINFO bi;

    stdstr InitialDir = g_Settings->LoadStringVal(RomList_GameDir);
    std::wstring wTitle = L"Select your previous Project64 folder";
    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = nullptr;
    bi.pszDisplayName = Buffer;
    bi.lpszTitle = wTitle.c_str();
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
    bi.lParam = (DWORD)InitialDir.c_str();
    if ((pidl = SHBrowseForFolder(&bi)) != nullptr)
    {
        if (SHGetPathFromIDList(pidl, Directory))
        {
            stdstr path;
            CPath SelectedDir(path.FromUTF16(Directory), "");
            if (SelectedDir.DirectoryExists())
            {
                GetDlgItem(IDC_PREV_DIR).SetWindowText(Directory);
            }
        }
    }
}

LRESULT WelcomeScreen::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_Logo.SubclassWindow(GetDlgItem(IDC_BMP_LOGO));
    m_Logo.SetBitmap(MAKEINTRESOURCE(IDB_ABOUT_LOGO));
 
    LanguageList LangList = g_Lang->GetLangList();
    CComboBox LangCB(GetDlgItem(IDC_LANG_SEL));
    for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
    {
        int Index = LangCB.AddString(stdstr(Language->LanguageName).ToUTF16().c_str());
        if (_stricmp(Language->LanguageName.c_str(), "English") == 0)
        {
            LangCB.SetCurSel(Index);
        }
    }
    if (LangCB.GetCurSel() < 0)
    {
        LangCB.SetCurSel(0);
    }

    hDlgItem = GetDlgItem(IDC_INPUT_PLUGIN);
    LvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    LvColumn.cx = 0x70;
    LvColumn.pszText = "Plugin";
    SendMessageA(hDlgItem, LVM_INSERTCOLUMNA, 0, (LPARAM)&LvColumn);
    LvColumn.cx = 0x120;
    LvColumn.pszText = "Description";
    SendMessageA(hDlgItem, LVM_INSERTCOLUMNA, 1, (LPARAM)&LvColumn);
    LvItem.mask = LVIF_TEXT;
    LvItem.cchTextMax = 256;
    LvItem.pszText = "Init";
    LvItem.iSubItem = 0;
    for (i = 0; i < 6; i++) {
        LvItem.iItem = i;
        SendMessageA(hDlgItem, LVM_INSERTITEMA, 0, (LPARAM)&LvItem);
    }
    SendMessageA(hDlgItem, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    LvItem.iItem = 0;
    LvItem.iSubItem = 0;
    LvItem.pszText = "NRage";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);
    LvItem.iSubItem = 1;
    LvItem.pszText = "For DirectInput controllers";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);

    LvItem.iItem = 1;
    LvItem.iSubItem = 0;
    LvItem.pszText = "Luna DInput";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);
    LvItem.iSubItem = 1;
    LvItem.pszText = "For keyboard";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);

    LvItem.iItem = 2;
    LvItem.iSubItem = 0;
    LvItem.pszText = "LINK's Mapper";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);
    LvItem.iSubItem = 1;
    LvItem.pszText = "For XInput controllers, no GUI";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);

    LvItem.iItem = 3;
    LvItem.iSubItem = 0;
    LvItem.pszText = "pj64-wiiu-gcn";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);
    LvItem.iSubItem = 1;
    LvItem.pszText = "For GCN controller with Wii U/Switch adapter";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);

    LvItem.iItem = 4;
    LvItem.iSubItem = 0;
    LvItem.pszText = "RaphnetRaw";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);
    LvItem.iSubItem = 1;
    LvItem.pszText = "For OEM with RaphNet adapter";
    SendMessageA(hDlgItem, LVM_SETITEMA, 0, (LPARAM)&LvItem);

    return TRUE;
}

LRESULT WelcomeScreen::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0));
    SetBkMode(hdcStatic, TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

BOOL WelcomeScreen::OnEraseBackground(CDCHandle dc)
{
    static HPEN outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
    static HBRUSH fill = CreateSolidBrush(0x00FFFFFF);
    dc.SelectPen(outline);
    dc.SelectBrush(fill);

    RECT rect;
    GetClientRect(&rect);
    dc.Rectangle(&rect);
    return TRUE;
}

HBRUSH WelcomeScreen::OnCtlColorStatic(CDCHandle dc, CStatic /*wndStatic*/)
{
    dc.SetBkColor(RGB(255, 255, 255));
    dc.SetDCBrushColor(RGB(255, 255, 255));
    return (HBRUSH)GetStockObject(DC_BRUSH);
}

LRESULT WelcomeScreen::OnOkCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/)
{
    CComboBox LangCB(GetDlgItem(IDC_LANG_SEL));
    int Index = LangCB.GetCurSel();
    if (Index >= 0)
    {
        wchar_t String[255];
        LangCB.GetLBText(Index, String);
        g_Lang->SetLanguage(stdstr().FromUTF16(String).c_str());
    }

    CPath GameDir(GetCWindowText(GetDlgItem(IDC_GAME_DIR)).c_str(), "");
    if (GameDir.DirectoryExists())
    {
        g_Settings->SaveString(RomList_GameDir, GameDir.GetDriveDirectory().c_str());
        Notify().AddRecentDir(GameDir);
    }

    CPath PrevDir(GetCWindowText(GetDlgItem(IDC_PREV_DIR)).c_str(), "");
    std::string DriveDirectoryStr = PrevDir.GetDriveDirectory();
    if (DriveDirectoryStr != "\\") {
        PrevDir.AppendDirectory("Save");
        if (PrevDir.DirectoryExists())
        {
            std::string DriveDirectoryStr = PrevDir.GetDriveDirectory();
            char* PrevDirChar = const_cast<char*>(DriveDirectoryStr.c_str());

            wchar_t* AppdataPathW = NULL;
            SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &AppdataPathW);
            char* AppdataPath = new char[wcslen(AppdataPathW) * sizeof(AppdataPathW[0]) + 20];
            wcstombs(AppdataPath, AppdataPathW, wcslen(AppdataPathW) * sizeof(AppdataPathW[0]) + 20);
            strcat(AppdataPath, "\\Luna-Project64\\Save\\");
            strcat(PrevDirChar, "\\");
            std::error_code ec;

            try {
                std::filesystem::copy(PrevDirChar, AppdataPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
            }
            catch (...) {
                MessageBox(L"Failed to copy folder.", L"Error", MB_OK);
                return FALSE;
            }

            if (strstr(PrevDirChar, "Program Files") != NULL)
            {
                wchar_t* LocalPathW = NULL;
                SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &LocalPathW);
                char* LocalPath = new char[wcslen(LocalPathW) * sizeof(LocalPathW[0]) + 20];
                wcstombs(LocalPath, LocalPathW, wcslen(LocalPathW) * sizeof(LocalPathW[0]) + 20);
                strcat(LocalPath, "\\VirtualStore\\");
                char* ptr = strstr(PrevDirChar, "Project64");
                if (ptr != NULL) {
                    strcat(LocalPath, strstr(PrevDirChar, "Project64"));
                    try {
                        std::filesystem::copy(LocalPath, AppdataPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
                    }
                    catch (...) {
                        MessageBox(L"Failed to copy VirtualStore folder.", L"Error", MB_OK);
                        return FALSE;
                    }
                }
                try {
                    for (auto const& file : std::filesystem::directory_iterator{ AppdataPath }) {
                        const std::string oldSaveStateName = ".pj0";
                        const std::string newSaveStateName = ".pj";
                        const size_t pos = file.path().string().find(oldSaveStateName);

                        if (pos != std::string::npos) {
                            const std::string newFile = file.path().string().replace(pos, oldSaveStateName.length(), newSaveStateName);
                            std::rename(file.path().string().c_str(), newFile.c_str());
                        }
                    }
                }
                catch (...) {
                    MessageBox(L"Failed to convert savestate file extensions from PJ64 1.6.", L"Error", MB_OK);
                    return FALSE;
                }
                delete[] LocalPath;
            }
            delete[] AppdataPath;
        }
        else
        {
            MessageBox(L"The chosen folder isn't a valid Project64 installation directory.", L"Invalid directory", MB_OK);
            return FALSE;
        }
    }
    string Project64VideoPluginPath = g_Settings->LoadStringVal(Plugin_GFX_Default);
    if (Project64VideoPluginPath.find("Project64-Video") == string::npos) {
        Project64VideoPluginPath = "GFX\\Project64-Video.dll";
    }

    switch (selectedIndexInput)
    {
    case 0:
        g_Settings->SaveString(Plugin_CONT_Default, "Input\\NRage_Input_V2.4.dll"); //NRage
        break;
    case 1:
        g_Settings->SaveString(Plugin_CONT_Default, "Input\\Luna's DirectInput8.dll"); //Luna
        break;
    case 2:
        g_Settings->SaveString(Plugin_CONT_Default, "Input\\LMapper.dll"); //LMapper
        break;
    case 3:
        g_Settings->SaveString(Plugin_CONT_Default, "Input\\pj64-wiiu-gcn.dll"); //pj64-wiiu-gcn
        break;
    case 4:
        g_Settings->SaveString(Plugin_CONT_Default, "Input\\pj64raphnetraw_1player.dll"); //RaphnetRaw
        break;
    }
    g_Settings->SaveString(Plugin_CONT_Current, g_Settings->LoadStringVal(Plugin_CONT_Default));

    auto i = g_Settings->LoadStringVal(Plugin_CONT_Default);

    EndDialog(0);
    return TRUE;
}

LRESULT WelcomeScreen::OnPluginHelp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (isWine())
    {
        ShellExecuteA(0, 0, "http://sites.google.com/view/shurislibrary/plugin-guide/linux/linux-301n", 0, 0, SW_HIDE);
    }
    else
    {
        ShellExecuteA(0, 0, "http://sites.google.com/view/shurislibrary/plugin-guide/windows/windows-301n", 0, 0, SW_HIDE);
    }
    return TRUE;
}

LRESULT WelcomeScreen::OnListNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (lParam != NULL) {
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_CLICK:
            hDlgItem = GetDlgItem(IDC_INPUT_PLUGIN);
            selectedIndexInput = SendMessage(hDlgItem, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
            return TRUE;
            break;
        }
    }
    return FALSE;
}

int CALLBACK WelcomeScreen::SelectDirCallBack(HWND hwnd, DWORD uMsg, DWORD /*lp*/, DWORD lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // WParam is TRUE since you are passing a path
        // It would be FALSE if you were passing a PIDL
        if (lpData)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        break;
    }
    return 0;
}
