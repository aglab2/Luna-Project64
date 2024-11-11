#include "stdafx.h"

#include "SettingsPage.h"

CGeneralOptionsPage::CGeneralOptionsPage(CSettingConfig * SettingsConfig, HWND hParent, const RECT & rcDispay) :
m_SettingsConfig(SettingsConfig)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }

    // Set the text for all GUI items
    SetDlgItemText(IDC_DARKTHEME, wGS(OPTION_DARKTHEME).c_str());
    SetDlgItemText(IDC_AUTOSLEEP, wGS(OPTION_AUTO_SLEEP).c_str());
    SetDlgItemText(IDC_MINIMIZEDSLEEP, wGS(OPTION_MINIMIZED_SLEEP).c_str());
    SetDlgItemText(IDC_LOAD_FULLSCREEN, wGS(OPTION_AUTO_FULLSCREEN).c_str());
    SetDlgItemText(IDC_SCREEN_SAVER, wGS(OPTION_DISABLE_SS).c_str());
    SetDlgItemText(IDC_DISCORD_RPC, wGS(OPTION_DISCORD_RPC).c_str());
    SetDlgItemText(IDC_GLOBAL_CHEATS, wGS(OPTION_GLOBAL_CHEATS).c_str());
    SetDlgItemText(IDC_BASIC_MODE, wGS(OPTION_BASIC_MODE).c_str());
    SetDlgItemText(IDC_MAXROMS_TXT, wGS(RB_MAX_ROMS).c_str());
    SetDlgItemText(IDC_ROMSEL_TEXT2, wGS(RB_ROMS).c_str());
    SetDlgItemText(IDC_MAXROMDIR_TXT, wGS(RB_MAX_DIRS).c_str());
    SetDlgItemText(IDC_ROMSEL_TEXT4, wGS(RB_DIRS).c_str());
    SetDlgItemText(IDC_IPLDIR_TXT, wGS(OPTION_IPL_ROM_PATH).c_str());
    SetDlgItemText(IDC_INPUTDELAY_TXT, wGS(OPTION_INPUTDELAY).c_str());
    SetDlgItemText(IDC_RETROACHIEVEMENTS, wGS(OPTION_RETROACHIEVEMENTS).c_str());

    AddModCheckBox(GetDlgItem(IDC_DARKTHEME), (SettingID)Setting_DarkTheme);
    AddModCheckBox(GetDlgItem(IDC_AUTOSLEEP), (SettingID)Setting_AutoSleep);
    AddModCheckBox(GetDlgItem(IDC_MINIMIZEDSLEEP), (SettingID)Setting_MinimizedSleep);
    AddModCheckBox(GetDlgItem(IDC_LOAD_FULLSCREEN), (SettingID)Setting_AutoFullscreen);
    AddModCheckBox(GetDlgItem(IDC_SCREEN_SAVER), (SettingID)Setting_DisableScrSaver);
	AddModCheckBox(GetDlgItem(IDC_DISCORD_RPC), (SettingID)Setting_EnableDiscordRPC);
    AddModCheckBox(GetDlgItem(IDC_GLOBAL_CHEATS), (SettingID)Setting_GlobalCheats);
    AddModCheckBox(GetDlgItem(IDC_BASIC_MODE), UserInterface_BasicMode);
    AddModCheckBox(GetDlgItem(IDC_RETROACHIEVEMENTS), (SettingID)Setting_RetroAchievements);

    CModifiedEditBox * TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBER), (SettingID)File_RecentGameFileCount, false);
    TxtBox->SetTextField(GetDlgItem(IDC_MAXROMS_TXT));

    TxtBox = AddModTextBox(GetDlgItem(IDC_REMEMBERDIR), (SettingID)Directory_RecentGameDirCount, false);
    TxtBox->SetTextField(GetDlgItem(IDC_MAXROMDIR_TXT));

    TxtBox = AddModTextBox(GetDlgItem(IDC_INPUTDELAY), GameRunning_InputDelay, false);
    TxtBox->SetTextField(GetDlgItem(IDC_INPUTDELAY));

    UpdatePageSettings();
    
    if (g_Settings->LoadBool(UserInterface_BasicMode)) {
        g_Settings->SaveBool(UserInterface_BasicMode, 0);
        MessageBox(wGS(MSG_BASICMODE_UNSUPPORTED).c_str(), wGS(MSG_DEPRECATED_SETTING_TITLE).c_str(), MB_OK);
    }
}

void CGeneralOptionsPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGeneralOptionsPage::ShowPage()
{
    ShowWindow(SW_SHOW);
}

void CGeneralOptionsPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGeneralOptionsPage>::ApplySettings(UpdateScreen);
}

bool CGeneralOptionsPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGeneralOptionsPage>::EnableReset()) { return true; }
    return false;
}

void CGeneralOptionsPage::ResetPage()
{
    CSettingsPageImpl<CGeneralOptionsPage>::ResetPage();
    m_SettingsConfig->UpdateAdvanced((int)::SendMessage(GetDlgItem(IDC_BASIC_MODE), BM_GETCHECK, 0, 0) == 0);
}

void CGeneralOptionsPage::OnBasicMode(UINT Code, int id, HWND ctl)
{
    CheckBoxChanged(Code, id, ctl);
    m_SettingsConfig->UpdateAdvanced((int)::SendMessage(ctl, BM_GETCHECK, 0, 0) == 0);
}
