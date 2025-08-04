#include "stdafx.h"

#include "SettingsPage.h"
#include "SettingsPage-Game-Status.h"

CGameStatusPage::CGameStatusPage(HWND hParent, const RECT & rcDispay)
{
    if (!Create(hParent, rcDispay))
    {
        return;
    }
}

struct ColorDescription
{
    const wchar_t* Name;
    const char* RGB;
};

static const ColorDescription ColorNames[] = {
    { L"", "" },
    { L"Purple", "800080" },
    { L"Yellow", "827B00" },
    { L"Red", "990000" },
    { L"Blue", "000099" },
    { L"Green", "003300" },
    { L"Orange", "A66023" },
    { L"Brown", "603913" },
    { L"LightBlue", "00897C"}
};

static const char* ColorRGB[] = {
    "000000", "800080", "FFFF00", "FF0000", "0000FF", "008000", "FFA500", "A52A2A", "ADD8E6"
};

void CGameStatusPage::Init()
{
    if (m_Initialized)
        return;
    m_Initialized = true;

    stdstr Status = UISettingsLoadStringVal(Rdn_Color);

    CModifiedComboBoxTxt * ComboBox;
    ComboBox = AddModComboBoxTxt(GetDlgItem(IDC_STATUS_TYPE), (SettingID)Rdn_Color);
    if (ComboBox)
    {
        for (const auto& desc : ColorNames)
        {
            ComboBox->AddItem(desc.Name, desc.RGB);
        }
        ComboBox->SetTextField(GetDlgItem(IDC_STATUS_TEXT));
    }

    CModifiedEditBox * TxtBox;
    if (g_Settings->LoadBool(Setting_RdbEditor))
    {
        TxtBox = AddModTextBox(GetDlgItem(IDC_NOTES_CORE), (SettingID)Rdb_NotesCore, true);
        TxtBox->SetTextField(GetDlgItem(IDC_NOTES_CORE_TEXT));
        TxtBox = AddModTextBox(GetDlgItem(IDC_NOTES_PLUGIN), (SettingID)Rdb_NotesPlugin, true);
        TxtBox->SetTextField(GetDlgItem(IDC_NOTES_PLUGIN_TEXT));
    }
    else
    {
        TxtBox = AddModTextBox(GetDlgItem(IDC_NOTES_CORE), (SettingID)Rdn_NotesUser, true);
        TxtBox->SetTextField(GetDlgItem(IDC_NOTES_CORE_TEXT));
        SetWindowTextA(GetDlgItem(IDC_NOTES_CORE_TEXT), "Notes");
		::ShowWindow(GetDlgItem(IDC_NOTES_PLUGIN), SW_HIDE);
        ::ShowWindow(GetDlgItem(IDC_NOTES_PLUGIN_TEXT), SW_HIDE);
    }

    UpdatePageSettings();
}

void CGameStatusPage::ShowPage()
{
    Init();
    ShowWindow(SW_SHOW);
}

void CGameStatusPage::HidePage()
{
    ShowWindow(SW_HIDE);
}

void CGameStatusPage::ApplySettings(bool UpdateScreen)
{
    CSettingsPageImpl<CGameStatusPage>::ApplySettings(UpdateScreen);
}

bool CGameStatusPage::EnableReset(void)
{
    if (CSettingsPageImpl<CGameStatusPage>::EnableReset()) { return true; }
    return false;
}

void CGameStatusPage::ResetPage()
{
    CSettingsPageImpl<CGameStatusPage>::ResetPage();
}
