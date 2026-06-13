#pragma once
#include <Project64/Settings/UISettings.h>
#include <Project64/ProfileManager.h>
#include <Project64/MenuId.h>

class CMainMenu :
    public CBaseMenu,
    private CDebugSettings
{
public:
    CMainMenu(CMainGui* hMainWindow, ProfileManager& profileManager);
    ~CMainMenu();

    int ProcessAccelerator(HWND hWnd, void* lpMsg);
    bool ProcessMessage(HWND hWnd, DWORD wNotifyCode, DWORD wID, DWORD lParam);
    void ResetMenu(void);
    void ResetAccelerators(void) { m_ResetAccelerators = true; }

private:
    CMainMenu();
    CMainMenu(const CMainMenu&);
    CMainMenu& operator=(const CMainMenu&);

    void OnOpenRom(HWND hWnd);
    void OnOpenCombo(HWND hWnd);
    void OnRomInfo(HWND hWnd);
    void OnEndEmulation(void);
    void OnScreenShot(void);
    void OnSaveAs(HWND hWnd);
    void OnLodState(HWND hWnd);
    void OnEnhancements(HWND hWnd);
    void OnCheats(HWND hWnd);
    void OnSettings(HWND hWnd);
    void OnSupportProject64(HWND hWnd);

    void FillOutMenu(HMENU hMenu);
    std::wstring GetSaveSlotString(int Slot);
    stdstr GetFileLastMod(const CPath& FileName);
    void RebuildAccelerators(void);
    std::string ChooseFileToOpen(HWND hParent);
    std::string ChooseROMFileToOpen(HWND hParent);
    std::string ChooseDiskFileToOpen(HWND hParent);
    void SetTraceModuleSetttings(SettingID Type);
    void ShortCutsChanged(void);

    static void CALL SettingsChanged(CMainMenu* _this);
    static void CALL stShortCutsChanged(CMainMenu* _this) { return _this->ShortCutsChanged(); }

    typedef std::list<SettingID> SettingList;
    typedef std::list<UISettingID> UISettingList;

    CMainGui* m_Gui;

    void* m_AccelTable;
    bool m_ResetAccelerators;
    CShortCuts m_ShortCuts;
    SettingList m_ChangeSettingList;
    UISettingList m_ChangeUISettingList;
    CriticalSection m_CS;
    ProfileManager& m_ProfileManager;
};
