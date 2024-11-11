#include "stdafx.h"

int  CGuiSettings::m_RefCount = 0;
bool CGuiSettings::m_bCPURunning;
bool CGuiSettings::m_bAutoSleep;
bool CGuiSettings::m_bMinimizedSleep;
bool CGuiSettings::m_bDarkTheme;
bool CGuiSettings::m_bGlobalCheats;
bool CGuiSettings::m_bRetroAchievements;

CGuiSettings::CGuiSettings()
{
    m_RefCount += 1;
    if (m_RefCount == 1)
    {
        g_Settings->RegisterChangeCB(GameRunning_CPU_Running,nullptr,RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_AutoSleep,nullptr,RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_MinimizedSleep, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_DarkTheme, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_GlobalCheats, nullptr, RefreshSettings);
        g_Settings->RegisterChangeCB((SettingID)Setting_RetroAchievements, nullptr, RefreshSettings);
        RefreshSettings(nullptr);
    }
}

CGuiSettings::~CGuiSettings()
{
    m_RefCount -= 1;
    if (m_RefCount == 0)
    {
        g_Settings->UnregisterChangeCB(GameRunning_CPU_Running,nullptr,RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_AutoSleep,nullptr,RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_MinimizedSleep, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_DarkTheme, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_GlobalCheats, nullptr, RefreshSettings);
        g_Settings->UnregisterChangeCB((SettingID)Setting_RetroAchievements, nullptr, RefreshSettings);
    }
}

void CALL CGuiSettings::RefreshSettings(void *)
{
    m_bCPURunning = g_Settings->LoadBool(GameRunning_CPU_Running);
    m_bAutoSleep = g_Settings->LoadBool((SettingID)Setting_AutoSleep);
    m_bMinimizedSleep = g_Settings->LoadBool((SettingID)Setting_MinimizedSleep);
    m_bDarkTheme = g_Settings->LoadBool((SettingID)Setting_DarkTheme);
    m_bGlobalCheats = g_Settings->LoadBool((SettingID)Setting_GlobalCheats);
    m_bRetroAchievements = g_Settings->LoadBool((SettingID)Setting_RetroAchievements);
}
