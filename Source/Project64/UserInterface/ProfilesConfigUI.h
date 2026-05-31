#pragma once

#include <string>

#include "resource.h"
#include "ProfileManager.h"

class CProfilesConfigUI :
    public CDialogImpl<CProfilesConfigUI>
{
public:
    BEGIN_MSG_MAP_EX(CProfilesConfigUI)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_PROFILE_PRESET_COMBO, CBN_SELCHANGE, OnProfilePresetChanged)
        COMMAND_RANGE_HANDLER(IDC_PROFILE_CPU_BASIC, IDC_PROFILE_MEMORY_CUSTOM, OnSelectionChanged)
        COMMAND_ID_HANDLER(IDOK, OnApply)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseDialog)
    END_MSG_MAP()

    enum { IDD = IDD_PROFILES_CONFIG };

    CProfilesConfigUI(ProfileManager& profileManager);
    void Display(void * ParentWindow);

private:
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnProfilePresetChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnSelectionChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
    LRESULT OnCloseDialog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);

    int GetSelectedProfileIndex() const;
    const ProfileDefinition & GetSelectedProfile() const;
    void SyncPresetSelectionFromUi(const ProfileSelection & selection);
    ProfileSelection GetUiSelection() const;
    void ApplySelectionToUi(const ProfileSelection & selection);
    void UpdateUiState();
    void UpdateGraphicsCheckboxState(const ProfileSelection & selection);

    bool IsGraphicsHleFb(ProfileSelection::GraphicsMode mode) const;

    CComboBox m_ProfilePreset;
	ProfileManager& m_ProfileManager;
};
