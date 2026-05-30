#pragma once

#include <commctrl.h>
#include <string>
#include <vector>

#include "ff.h"
#include "diskio.h"

class CSdcardFsUI :
    public CDialogImpl<CSdcardFsUI>
{
public:
    BEGIN_MSG_MAP_EX(CSdcardFsUI)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MSG_WM_DROPFILES(OnDropFiles)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseDialog)
        COMMAND_ID_HANDLER(ID_SDCARD_BACK, OnBackSelected)
        COMMAND_ID_HANDLER(ID_SDCARD_UPLOAD, OnUploadCommand)
        COMMAND_ID_HANDLER(ID_SDCARD_DOWNLOAD, OnDownloadCommand)
        COMMAND_ID_HANDLER(ID_SDCARD_CREATE_DIRECTORY, OnCreateDirectoryCommand)
        COMMAND_ID_HANDLER(ID_SDCARD_DELETE, OnDeleteCommand)
        NOTIFY_HANDLER_EX(IDC_SDCARD_LIST, LVN_ITEMACTIVATE, OnListItemActivate)
        NOTIFY_HANDLER_EX(IDC_SDCARD_LIST, LVN_DELETEITEM, OnListItemDeleted)
    END_MSG_MAP()

    enum { IDD = IDD_SDCARD_FS_UI };

    CSdcardFsUI();
    ~CSdcardFsUI();

    void Display(void * ParentWindow);

private:
    struct FsEntry
    {
        std::string Name;
        bool IsDirectory;
    };

    struct FsDirHandle;

    struct ListItemData
    {
        std::string Path;
        bool IsDirectory;
    };

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseDialog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBackSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnUploadCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDownloadCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCreateDirectoryCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDeleteCommand(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDropFiles(HDROP);
    LRESULT OnListItemActivate(NMHDR * phdr);
    LRESULT OnListItemDeleted(NMHDR * phdr);

    void ResizeControlsToClient();
    void RefreshCurrentDirectory(const std::string & selectPath = std::string());
    void InsertFsEntry(const std::string & parentPath, const FsEntry & entry);
    int ItemFromScreenPoint(POINT pt) const;
    ListItemData * GetItemData(int index) const;
    std::vector<ListItemData *> GetSelectedItemData() const;
    void NavigateToPath(const std::string & path);
    void NavigateBack();
    void UpdatePathLabel();
    static std::string FormatDisplayPath(const std::string & path);

    void OnUploadSelected();
    void OnDownloadSelected();
    void OnCreateDirectorySelected();
    void OnDeleteSelected();

    void UploadSelected(const std::vector<std::wstring>&);

    FF::FRESULT FsUploadFile(const std::wstring & hostFilePath, const std::string & fsDestinationPath, bool canReplace);
    FF::FRESULT FsDownloadFile(const std::string & fsSourcePath, const std::wstring & hostDestinationPath, bool canReplace);
    FF::FRESULT FsCreateDirectory(const std::string & fsPath);

    static std::string JoinFsPath(const std::string & parent, const std::string & name);

    CListViewCtrl m_FileList;
    CStatic m_PathLabel;
    std::string m_CurrentPath;
    FF::FATFS m_Fs{};
};
