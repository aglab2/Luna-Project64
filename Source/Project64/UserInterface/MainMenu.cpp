#include "stdafx.h"
#include "RomInformation.h"
#include "Debugger/Breakpoints.h"
#include "Debugger/ScriptSystem.h"
#include "DiscordRPC.h"
#include <Project64-core/N64System/N64Disk.h>
#include <Project64-core/N64System/Summercart.h>
#include <Project64\UserInterface\About.h>
#include "SdCardMounter.h"
#include <comutil.h>
#include <windows.h>
#include <commdlg.h>

#ifdef RETROACHIEVEMENTS
#include <Project64-core/RetroAchievements.h>
#endif

CMainMenu::CMainMenu(CMainGui * hMainWindow) :
    CBaseMenu(),
    m_ResetAccelerators(true),
    m_Gui(hMainWindow)
{
    ResetMenu();

    hMainWindow->SetWindowMenu(this);

    m_ChangeSettingList.push_back(GameRunning_LimitFPS);
    m_ChangeUISettingList.push_back(UserInterface_InFullScreen);
    m_ChangeUISettingList.push_back(UserInterface_AlwaysOnTop);
    m_ChangeUISettingList.push_back(UserInterface_ShowingNagWindow);
    m_ChangeSettingList.push_back(UserInterface_ShowCPUPer);
    m_ChangeSettingList.push_back(Logging_GenerateLog);
    m_ChangeSettingList.push_back(Debugger_RecordExecutionTimes);
    m_ChangeSettingList.push_back(Debugger_ShowTLBMisses);
    m_ChangeSettingList.push_back(Debugger_ShowUnhandledMemory);
    m_ChangeSettingList.push_back(Debugger_ShowPifErrors);
    m_ChangeSettingList.push_back(Debugger_ShowDListAListCount);
    m_ChangeSettingList.push_back(Debugger_DebugLanguage);
    m_ChangeSettingList.push_back(Debugger_ShowRecompMemSize);
    m_ChangeSettingList.push_back(Debugger_ShowDivByZero);
    m_ChangeSettingList.push_back(Debugger_RecordRecompilerAsm);
    m_ChangeSettingList.push_back(Debugger_DisableGameFixes);
    m_ChangeSettingList.push_back(Debugger_TraceMD5);
    m_ChangeSettingList.push_back(Debugger_TraceSettings);
    m_ChangeSettingList.push_back(Debugger_TraceUnknown);
    m_ChangeSettingList.push_back(Debugger_TraceAppInit);
    m_ChangeSettingList.push_back(Debugger_TraceAppCleanup);
    m_ChangeSettingList.push_back(Debugger_TraceN64System);
    m_ChangeSettingList.push_back(Debugger_TracePlugins);
    m_ChangeSettingList.push_back(Debugger_TraceGFXPlugin);
    m_ChangeSettingList.push_back(Debugger_TraceAudioPlugin);
    m_ChangeSettingList.push_back(Debugger_TraceControllerPlugin);
    m_ChangeSettingList.push_back(Debugger_TraceRSPPlugin);
    m_ChangeSettingList.push_back(Debugger_TraceRSP);
    m_ChangeSettingList.push_back(Debugger_TraceAudio);
    m_ChangeSettingList.push_back(Debugger_TraceRegisterCache);
    m_ChangeSettingList.push_back(Debugger_TraceRecompiler);
    m_ChangeSettingList.push_back(Debugger_TraceTLB);
    m_ChangeSettingList.push_back(Debugger_TraceProtectedMEM);
    m_ChangeSettingList.push_back(Debugger_TraceUserInterface);
    m_ChangeSettingList.push_back(Debugger_AppLogFlush);
    m_ChangeSettingList.push_back(Game_CurrentSaveState);
    m_ChangeSettingList.push_back(Setting_CurrentLanguage);
    m_ChangeSettingList.push_back(GameRunning_InputDelay);

    for (UISettingList::const_iterator iter = m_ChangeUISettingList.begin(); iter != m_ChangeUISettingList.end(); iter++)
    {
        g_Settings->RegisterChangeCB((SettingID)*iter, this, (CSettings::SettingChangedFunc)SettingsChanged);
    }
    for (SettingList::const_iterator iter = m_ChangeSettingList.begin(); iter != m_ChangeSettingList.end(); iter++)
    {
        g_Settings->RegisterChangeCB(*iter, this, (CSettings::SettingChangedFunc)SettingsChanged);
    }

    g_Settings->RegisterChangeCB((SettingID)Info_ShortCutsChanged, this, (CSettings::SettingChangedFunc)stShortCutsChanged);
}

CMainMenu::~CMainMenu()
{
    g_Settings->UnregisterChangeCB((SettingID)Info_ShortCutsChanged, this, (CSettings::SettingChangedFunc)stShortCutsChanged);
    for (UISettingList::const_iterator iter = m_ChangeUISettingList.begin(); iter != m_ChangeUISettingList.end(); iter++)
    {
        g_Settings->UnregisterChangeCB((SettingID)*iter, this, (CSettings::SettingChangedFunc)SettingsChanged);
    }
    for (SettingList::const_iterator iter = m_ChangeSettingList.begin(); iter != m_ChangeSettingList.end(); iter++)
    {
        g_Settings->UnregisterChangeCB(*iter, this, (CSettings::SettingChangedFunc)SettingsChanged);
    }
}

void CALL CMainMenu::SettingsChanged(CMainMenu * _this)
{
    _this->ResetMenu();
}

int CMainMenu::ProcessAccelerator(HWND hWnd, void * lpMsg)
{
    if (m_ResetAccelerators)
    {
        m_ResetAccelerators = false;
        RebuildAccelerators();
    }
    if (!m_AccelTable) { return false; }
    return TranslateAccelerator((HWND)hWnd, (HACCEL)m_AccelTable, (LPMSG)lpMsg);
}

std::string CMainMenu::ChooseFileToOpen(HWND hParent)
{
    CPath FileName;
    const char * Filter = "N64 ROMs and disks (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin, *.ndd, *.d64)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal;*.ndd;*.d64\0All files (*.*)\0*.*\0";
    if (FileName.SelectFile(hParent, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
    {
        return FileName;
    }
    return "";
}

std::string CMainMenu::ChooseROMFileToOpen(HWND hParent)
{
    CPath FileName;
    const char * Filter = "N64 ROMs (*.zip, *.7z, *.?64, *.rom, *.usa, *.jap, *.pal, *.bin)\0*.?64;*.zip;*.7z;*.bin;*.rom;*.usa;*.jap;*.pal\0All files (*.*)\0*.*\0";
    if (FileName.SelectFile(hParent, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
    {
        return FileName;
    }
    return "";
}

std::string CMainMenu::ChooseDiskFileToOpen(HWND hParent)
{
    CPath FileName;
    const char * Filter = "N64DD disk images (*.ndd, *.d64)\0*.ndd;*.d64\0All files (*.*)\0*.*\0";
    if (FileName.SelectFile(hParent, g_Settings->LoadStringVal(RomList_GameDir).c_str(), Filter, true))
    {
        return FileName;
    }
    return "";
}

void CMainMenu::SetTraceModuleSetttings(SettingID Type)
{
    uint32_t value = g_Settings->LoadDword(Type) == TraceVerbose ? g_Settings->LoadDefaultDword(Type) : TraceVerbose;
    g_Settings->SaveDword(Type, value);
}

void CMainMenu::ShortCutsChanged(void)
{
    m_ShortCuts.Load();
    ResetMenu();
    m_ResetAccelerators = true;
}

void CMainMenu::OnOpenRom(HWND hWnd)
{
#ifdef RETROACHIEVEMENTS
    if (!RA_ConfirmLoadNewRom(false))
        return;
#endif

    std::string File = ChooseFileToOpen(hWnd);
    if (File.length() == 0)
    {
        return;
    }
    
    stdstr ext = CPath(File).GetExtension();
    if ((_stricmp(ext.c_str(), "ndd") != 0) && (_stricmp(ext.c_str(), "d64") != 0))
    {
        g_BaseSystem->RunFileImage(File.c_str());
        return;
    }
    else
    {
        g_BaseSystem->RunDiskImage(File.c_str());
    }
}

void CMainMenu::OnOpenCombo(HWND hWnd)
{
    std::string FileROM = ChooseROMFileToOpen(hWnd);
    if (FileROM.length() == 0)
    {
        return;
    }

    std::string FileDisk = ChooseDiskFileToOpen(hWnd);
    if (FileDisk.length() == 0)
    {
        return;
    }

    g_BaseSystem->RunDiskComboImage(FileROM.c_str(), FileDisk.c_str());
}

void CMainMenu::OnRomInfo(HWND hWnd)
{
    if (g_Disk)
    {
        RomInformation Info(g_Disk);
        Info.DisplayInformation(hWnd);
    }
    else if (g_Rom)
    {
        RomInformation Info(g_Rom);
        Info.DisplayInformation(hWnd);
    }
}

void CMainMenu::OnEndEmulation(void)
{
#ifdef RETROACHIEVEMENTS
    if (!RA_ConfirmLoadNewRom(false))
        return;
#endif

    WriteTrace(TraceUserInterface, TraceDebug, "ID_FILE_ENDEMULATION");
    if (g_BaseSystem)
    {
        g_BaseSystem->CloseCpu();
    }
    m_Gui->SaveWindowLoc();

	if (UISettingsLoadBool(Setting_EnableDiscordRPC))
	{
		CDiscord::Update(false);
	}
}

void CMainMenu::OnScreenShot(void)
{
    stdstr Dir(g_Settings->LoadStringVal(Directory_SnapShot));
    WriteTrace(TraceGFXPlugin, TraceDebug, "CaptureScreen(%s): Starting", Dir.c_str());
    g_Plugins->Gfx()->CaptureScreen(Dir.c_str());
    WriteTrace(TraceGFXPlugin, TraceDebug, "CaptureScreen: Done");
}

void CMainMenu::OnSaveAs(HWND hWnd)
{
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    char Directory[255], SaveFile[255];
    OPENFILENAMEA openfilename;

    memset(&SaveFile, 0, sizeof(SaveFile));
    memset(&openfilename, 0, sizeof(openfilename));

    UISettingsLoadStringVal(Directory_LastSave, Directory, sizeof(Directory));

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = (HWND)hWnd;
    openfilename.lpstrFilter = "Project64 saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
    openfilename.lpstrFile = SaveFile;
    openfilename.lpstrInitialDir = Directory;
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_HIDEREADONLY;

    g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_SaveGame);
    if (GetSaveFileNameA(&openfilename))
    {
        _splitpath(SaveFile, drive, dir, fname, ext);
        if (_stricmp(ext, ".pj") == 0 || _stricmp(ext, ".zip") == 0)
        {
            _makepath(SaveFile, drive, dir, fname, nullptr);
            _splitpath(SaveFile, drive, dir, fname, ext);
            if (_stricmp(ext, ".pj") == 0)
            {
                _makepath(SaveFile, drive, dir, fname, nullptr);
            }
        }
        g_Settings->SaveString(GameRunning_InstantSaveFile, SaveFile);

        char SaveDir[MAX_PATH];
        _makepath(SaveDir, drive, dir, nullptr, nullptr);
        UISettingsSaveString(Directory_LastSave, SaveDir);
        g_BaseSystem->ExternalEvent(SysEvent_SaveMachineState);
    }
    g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_SaveGame);
}

void CMainMenu::OnLodState(HWND hWnd)
{
#ifdef RETROACHIEVEMENTS
    if (!RA_WarnDisableHardcore("load a state"))
        return;
#endif

    g_BaseSystem->ExternalEvent(SysEvent_PauseCPU_LoadGame);

    char Directory[255];
    UISettingsLoadStringVal(Directory_LastSave, Directory, sizeof(Directory));

    CPath SaveFile;
    const char * Filter = "Project64 saves (*.zip, *.pj)\0*.pj?;*.pj;*.zip;";
    if (SaveFile.SelectFile(hWnd, Directory, Filter, false))
    {
        g_Settings->SaveString(GameRunning_InstantSaveFile, (const char *)SaveFile);
        if (!SaveFile.DirectoryExists())
        {
            SaveFile.DirectoryCreate();
        }
        UISettingsSaveString(Directory_LastSave, SaveFile.GetDriveDirectory());
        g_BaseSystem->ExternalEvent(SysEvent_LoadMachineState);
    }
    g_BaseSystem->ExternalEvent(SysEvent_ResumeCPU_LoadGame);
}

void CMainMenu::OnEnhancements(HWND /*hWnd*/)
{
#ifdef RETROACHIEVEMENTS
    if (!RA_WarnDisableHardcore("use enhancements"))
        return;
#endif

    m_Gui->DisplayEnhancements(false);
}

void CMainMenu::OnCheats(HWND /*hWnd*/)
{
#ifdef RETROACHIEVEMENTS
    if (!RA_WarnDisableHardcore("use cheats"))
        return;
#endif

    m_Gui->DisplayCheatsUI(false);
}

void CMainMenu::OnSettings(HWND hWnd)
{
    CSettingConfig().Display(hWnd);
}

void CMainMenu::OnSupportProject64(HWND hWnd)
{
    CSupportWindow(m_Gui->Support()).Show(hWnd, false);
}

static void invokeDefaultOpenAction(const TCHAR* path)
{    
    ShellExecute(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);
}

static void DebugOutput(const std::wstring& message) {
    OutputDebugString(message.c_str());
}

bool CMainMenu::ProcessMessage(HWND hWnd, DWORD /*FromAccelerator*/, DWORD MenuID)
{
    switch (MenuID)
    {
    case ID_FILE_OPEN_ROM: OnOpenRom(hWnd); break;
    case ID_FILE_OPEN_COMBO: OnOpenCombo(hWnd); break;
    case ID_FILE_MOUNT_SDCARD: 
    try
    {
        SdCardMounter::switchStates();
        auto state = SdCardMounter::getState();
        if (state == SdCardMounter::State::VHD)
        {
            invokeDefaultOpenAction(CSummerCart::VhdPath().c_str());
        }

        if (state == SdCardMounter::VHD)
        {

            MessageBox(hWnd, wGS(MSG_SDCARD_PREPARED).c_str(), wGS(MSG_SDCARD_TITLE).c_str(), MB_ICONEXCLAMATION);
            wchar_t* AppdataPathW = NULL;
            SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &AppdataPathW);
            PathAppend(AppdataPathW, L"Luna-Project64");
            ShellExecute(NULL, L"open", AppdataPathW, NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            MessageBox(hWnd, wGS(MSG_SDCARD_UNMOUNTED).c_str(), wGS(MSG_SDCARD_TITLE).c_str(), MB_OK);
        }
        {
            MENUITEMINFO MenuInfo = { 0 };
            wchar_t String[256];
            MenuInfo.cbSize = sizeof(MENUITEMINFO);
            MenuInfo.fMask = MIIM_TYPE;
            MenuInfo.fType = MFT_STRING;
            MenuInfo.fState = MFS_ENABLED;
            MenuInfo.dwTypeData = String;
            MenuInfo.cch = 256;

            GetMenuItemInfo(m_MenuHandle, ID_FILE_MOUNT_SDCARD, false /*lookup by identifier*/, &MenuInfo);
            wcscpy(String, state ? wGS(MENU_UNMOUNT_SDCARD).c_str() : wGS(MENU_MOUNT_SDCARD).c_str());
            SetMenuItemInfo(m_MenuHandle, ID_FILE_MOUNT_SDCARD, false /*lookup by identifier*/, &MenuInfo);
        }
    }
    catch (...)
    {
        auto state = SdCardMounter::getState();
        if (state == SdCardMounter::State::VHD)
		{
            MessageBox(hWnd, wGS(MSG_SDCARD_UNMOUNT_FAILED).c_str(), wGS(MSG_MSGBOX_ERROR_TITLE).c_str(), MB_ICONERROR);
		}
        else
        {
            MessageBox(hWnd, wGS(MSG_SDCARD_MOUNT_FAILED).c_str(), wGS(MSG_MSGBOX_ERROR_TITLE).c_str(), MB_ICONERROR);
        }
    }
    break;
    case ID_FILE_ROM_INFO: OnRomInfo(hWnd); break;
    case ID_FILE_STARTEMULATION:
        m_Gui->SaveWindowLoc();
        // Now we have created again, we can start up emulation
        if (g_BaseSystem)
        {
            if (g_Settings->LoadBool(Setting_AutoStart) == 0)
            {
                WriteTrace(TraceN64System, TraceDebug, "Manually starting ROM");
            }
            g_BaseSystem->StartEmulation(true);
        }
        else
        {
            g_Notify->BreakPoint(__FILE__, __LINE__);
        }
        break;
    case ID_FILE_ENDEMULATION: OnEndEmulation(); break;
    case ID_FILE_ROMDIRECTORY:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_FILE_ROMDIRECTORY 1");
        m_Gui->SelectRomDir();
        WriteTrace(TraceUserInterface, TraceDebug, "ID_FILE_ROMDIRECTORY 2");
        m_Gui->RefreshMenu();
        WriteTrace(TraceUserInterface, TraceDebug, "ID_FILE_ROMDIRECTORY 3");
        break;
    case ID_FILE_REFRESHROMLIST: m_Gui->RefreshRomList(); break;
    case ID_FILE_EXIT:
#ifdef RETROACHIEVEMENTS
        if (!RA_ConfirmLoadNewRom(true))
            break;
#endif
        DestroyWindow((HWND)hWnd);
        break;
    case ID_SYSTEM_RESET_SOFT:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_RESET_SOFT");
        g_BaseSystem->ExternalEvent(SysEvent_ResetCPU_Soft);
        break;
    case ID_SYSTEM_RESET_HARD:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_RESET_HARD");
        g_BaseSystem->ExternalEvent(SysEvent_ResetCPU_Hard);
        break;
    case ID_SYSTEM_PAUSE:
        m_Gui->SaveWindowLoc();
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_PAUSE");
#ifdef RETROACHIEVEMENTS
        RA_SetPaused(!g_Settings->LoadBool(GameRunning_CPU_Paused));
#endif
        g_BaseSystem->ExternalEvent(g_Settings->LoadBool(GameRunning_CPU_Paused) ? SysEvent_ResumeCPU_FromMenu : SysEvent_PauseCPU_FromMenu);
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_PAUSE 1");
        break;
    case ID_SYSTEM_BITMAP: OnScreenShot(); break;
        break;
    case ID_SYSTEM_LIMITFPS:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_LIMITFPS");
        g_Settings->SaveBool(GameRunning_LimitFPS, !g_Settings->LoadBool(GameRunning_LimitFPS));
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_LIMITFPS 1");
        break;
    case ID_SYSTEM_SWAPDISK:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_SWAPDISK");
        {
            // Open disk
            stdstr FileName = ChooseDiskFileToOpen(hWnd);
            if (FileName.length() != 0)
            {
                g_Disk->SaveDiskImage();
                g_Disk->SwapDiskImage(FileName.c_str());
            }
        }
        break;
    case ID_SYSTEM_SAVE:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_SAVE");
        g_BaseSystem->ExternalEvent(SysEvent_SaveMachineState);
        break;
    case ID_SYSTEM_SAVEAS: OnSaveAs(hWnd); break;
    case ID_SYSTEM_RESTORE:
#ifdef RETROACHIEVEMENTS
        if (!RA_WarnDisableHardcore("load a state"))
            break;
#endif
        WriteTrace(TraceUserInterface, TraceDebug, "ID_SYSTEM_RESTORE");
        g_BaseSystem->ExternalEvent(SysEvent_LoadMachineState);
        break;
    case ID_SYSTEM_LOAD: OnLodState(hWnd); break;
    case ID_SYSTEM_ENHANCEMENT: OnEnhancements(hWnd); break;
    case ID_SYSTEM_CHEAT: OnCheats(hWnd); break;
    case ID_SYSTEM_GSBUTTON:
#ifdef RETROACHIEVEMENTS
        if (!RA_WarnDisableHardcore("press the GS button"))
            break;
#endif
        g_BaseSystem->ExternalEvent(SysEvent_GSButtonPressed);
        break;
    case ID_OPTIONS_DISPLAY_FR:
        g_Settings->SaveBool(UserInterface_DisplayFrameRate, !g_Settings->LoadBool(UserInterface_DisplayFrameRate));
        break;
    case ID_OPTIONS_CHANGE_FR:
        switch (g_Settings->LoadDword(UserInterface_FrameDisplayType))
        {
        case FR_VIs:
            g_Settings->SaveDword(UserInterface_FrameDisplayType, FR_DLs);
            break;
        case FR_DLs:
            g_Settings->SaveDword(UserInterface_FrameDisplayType, FR_PERCENT);
            break;
        default:
            g_Settings->SaveDword(UserInterface_FrameDisplayType, FR_VIs);
        }
        break;
    case ID_OPTIONS_INCREASE_SPEED:
        g_BaseSystem->AlterSpeed(CSpeedLimiter::INCREASE_SPEED);
        break;
    case ID_OPTIONS_DECREASE_SPEED:
        g_BaseSystem->AlterSpeed(CSpeedLimiter::DECREASE_SPEED);
        break;
    case ID_OPTIONS_FULLSCREEN:
        g_BaseSystem->ExternalEvent(SysEvent_ChangingFullScreen);
        break;
    case ID_OPTIONS_FULLSCREEN2:
        if (UISettingsLoadBool(UserInterface_InFullScreen))
        {
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN a");
            m_Gui->MakeWindowOnTop(false);
            Notify().SetGfxPlugin(nullptr);
            WriteTrace(TraceGFXPlugin, TraceDebug, "ChangeWindow: Starting");
            g_Plugins->Gfx()->ChangeWindow();
            WriteTrace(TraceGFXPlugin, TraceDebug, "ChangeWindow: Done");
            ShowCursor(true);
            m_Gui->ShowStatusBar(true);
            m_Gui->MakeWindowOnTop(UISettingsLoadBool(UserInterface_AlwaysOnTop));
            UISettingsSaveBool(UserInterface_InFullScreen, (DWORD)false);
        }
        else
        {
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b");
            ShowCursor(false);
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 1");
            m_Gui->ShowStatusBar(false);
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 2");
            try
            {
                WriteTrace(TraceGFXPlugin, TraceDebug, "ChangeWindow: Starting");
                g_Plugins->Gfx()->ChangeWindow();
                WriteTrace(TraceGFXPlugin, TraceDebug, "ChangeWindow: Done");
            }
            catch (...)
            {
                WriteTrace(TraceError, TraceDebug, "Exception when going to full screen");
                char Message[600];
                sprintf(Message, "Exception caught\nFile: %s\nLine: %d", __FILE__, __LINE__);
                MessageBox(nullptr, stdstr(Message).ToUTF16().c_str(), L"Exception", MB_OK);
            }
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 4");
            m_Gui->MakeWindowOnTop(false);
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 5");
            Notify().SetGfxPlugin(g_Plugins->Gfx());
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 3");
            UISettingsSaveBool(UserInterface_InFullScreen, true);
            WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN b 6");
        }
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_FULLSCREEN 1");
        break;
    case ID_OPTIONS_ALWAYSONTOP:
        if (UISettingsLoadBool(UserInterface_AlwaysOnTop))
        {
            UISettingsSaveBool(UserInterface_AlwaysOnTop, false);
            m_Gui->MakeWindowOnTop(false);
        }
        else
        {
            UISettingsSaveBool(UserInterface_AlwaysOnTop, true);
            m_Gui->MakeWindowOnTop(g_Settings->LoadBool(GameRunning_CPU_Running));
        }
        break;
    case ID_OPTIONS_CONFIG_RSP:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_CONFIG_RSP");
        g_Plugins->ConfigPlugin(hWnd, PLUGIN_TYPE_RSP);
        break;
    case ID_OPTIONS_CONFIG_GFX:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_CONFIG_GFX");
        g_Plugins->ConfigPlugin(hWnd, PLUGIN_TYPE_GFX);
        break;
    case ID_OPTIONS_CONFIG_AUDIO:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_CONFIG_AUDIO");
        g_Plugins->ConfigPlugin(hWnd, PLUGIN_TYPE_AUDIO);
        break;
    case ID_OPTIONS_CONFIG_CONT:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_CONFIG_CONT");
        g_Plugins->ConfigPlugin(hWnd, PLUGIN_TYPE_CONTROLLER);
        break;
    case ID_OPTIONS_CPU_USAGE:
        WriteTrace(TraceUserInterface, TraceDebug, "ID_OPTIONS_CPU_USAGE");
        if (g_Settings->LoadBool(UserInterface_ShowCPUPer))
        {
            g_Settings->SaveBool(UserInterface_ShowCPUPer, false);
            g_Notify->DisplayMessage(0, EMPTY_STRING);
        }
        else
        {
            g_Settings->SaveBool(UserInterface_ShowCPUPer, true);
        }
        break;
    case ID_OPTIONS_SETTINGS: OnSettings(hWnd);  break;
    case ID_PROFILE_PROFILE:
        g_Settings->SaveBool(Debugger_RecordExecutionTimes, !g_Settings->LoadBool(Debugger_RecordExecutionTimes));
        g_BaseSystem->ExternalEvent(SysEvent_ResetFunctionTimes);
        break;
    case ID_PROFILE_RESETCOUNTER: g_BaseSystem->ExternalEvent(SysEvent_ResetFunctionTimes); break;
    case ID_PROFILE_GENERATELOG: g_BaseSystem->ExternalEvent(SysEvent_DumpFunctionTimes); break;
    case ID_DEBUG_SHOW_TLB_MISSES:
        g_Settings->SaveBool(Debugger_ShowTLBMisses, !g_Settings->LoadBool(Debugger_ShowTLBMisses));
        break;
    case ID_DEBUG_SHOW_UNHANDLED_MEM:
        g_Settings->SaveBool(Debugger_ShowUnhandledMemory, !g_Settings->LoadBool(Debugger_ShowUnhandledMemory));
        break;
    case ID_DEBUG_SHOW_PIF_ERRORS:
        g_Settings->SaveBool(Debugger_ShowPifErrors, !g_Settings->LoadBool(Debugger_ShowPifErrors));
        break;
    case ID_DEBUG_SHOW_DLIST_COUNT:
        g_Settings->SaveBool(Debugger_ShowDListAListCount, !g_Settings->LoadBool(Debugger_ShowDListAListCount));
        g_Notify->DisplayMessage(0, EMPTY_STRING);
        break;
    case ID_DEBUG_LANGUAGE:
        g_Settings->SaveBool(Debugger_DebugLanguage, !g_Settings->LoadBool(Debugger_DebugLanguage));
        g_Lang->LoadCurrentStrings();
        m_Gui->ResetRomBrowserColomuns();
        break;
    case ID_DEBUG_SHOW_RECOMP_MEM_SIZE:
        g_Settings->SaveBool(Debugger_ShowRecompMemSize, !g_Settings->LoadBool(Debugger_ShowRecompMemSize));
        g_Notify->DisplayMessage(0, EMPTY_STRING);
        break;
    case ID_DEBUG_SHOW_DIV_BY_ZERO:
        g_Settings->SaveBool(Debugger_ShowDivByZero, !g_Settings->LoadBool(Debugger_ShowDivByZero));
        break;
    case ID_DEBUG_RECORD_RECOMPILER_ASM:
        g_Settings->SaveBool(Debugger_RecordRecompilerAsm, !g_Settings->LoadBool(Debugger_RecordRecompilerAsm));
        break;
    case ID_DEBUG_DISABLE_GAMEFIX:
        g_Settings->SaveBool(Debugger_DisableGameFixes, !g_Settings->LoadBool(Debugger_DisableGameFixes));
        break;
    case ID_DEBUGGER_TRACE_MD5: SetTraceModuleSetttings(Debugger_TraceMD5); break;
    case ID_DEBUGGER_TRACE_SETTINGS: SetTraceModuleSetttings(Debugger_TraceSettings); break;
    case ID_DEBUGGER_TRACE_UNKNOWN: SetTraceModuleSetttings(Debugger_TraceUnknown); break;
    case ID_DEBUGGER_TRACE_APPINIT: SetTraceModuleSetttings(Debugger_TraceAppInit); break;
    case ID_DEBUGGER_TRACE_APPCLEANUP: SetTraceModuleSetttings(Debugger_TraceAppCleanup); break;
    case ID_DEBUGGER_TRACE_N64SYSTEM: SetTraceModuleSetttings(Debugger_TraceN64System); break;
    case ID_DEBUGGER_TRACE_PLUGINS: SetTraceModuleSetttings(Debugger_TracePlugins); break;
    case ID_DEBUGGER_TRACE_GFXPLUGIN: SetTraceModuleSetttings(Debugger_TraceGFXPlugin); break;
    case ID_DEBUGGER_TRACE_AUDIOPLUGIN: SetTraceModuleSetttings(Debugger_TraceAudioPlugin); break;
    case ID_DEBUGGER_TRACE_CONTROLLERPLUGIN: SetTraceModuleSetttings(Debugger_TraceControllerPlugin); break;
    case ID_DEBUGGER_TRACE_RSPPLUGIN: SetTraceModuleSetttings(Debugger_TraceRSPPlugin); break;
    case ID_DEBUGGER_TRACE_RSP: SetTraceModuleSetttings(Debugger_TraceRSP); break;
    case ID_DEBUGGER_TRACE_AUDIO: SetTraceModuleSetttings(Debugger_TraceAudio); break;
    case ID_DEBUGGER_TRACE_REGISTERCACHE: SetTraceModuleSetttings(Debugger_TraceRegisterCache); break;
    case ID_DEBUGGER_TRACE_RECOMPILER: SetTraceModuleSetttings(Debugger_TraceRecompiler); break;
    case ID_DEBUGGER_TRACE_TLB: SetTraceModuleSetttings(Debugger_TraceTLB); break;
    case ID_DEBUGGER_TRACE_PROTECTEDMEM: SetTraceModuleSetttings(Debugger_TraceProtectedMEM); break;
    case ID_DEBUGGER_TRACE_USERINTERFACE: SetTraceModuleSetttings(Debugger_TraceUserInterface); break;

    case ID_DEBUGGER_APPLOG_FLUSH:
        g_Settings->SaveBool(Debugger_AppLogFlush, !g_Settings->LoadBool(Debugger_AppLogFlush));
        break;
    case ID_DEBUGGER_LOGOPTIONS: m_Gui->EnterLogOptions(); break;
    case ID_DEBUGGER_GENERATELOG:
        g_Settings->SaveBool(Logging_GenerateLog, !g_Settings->LoadBool(Logging_GenerateLog));
        break;
    case ID_DEBUGGER_DUMPMEMORY: g_Debugger->OpenMemoryDump(); break;
    case ID_DEBUGGER_SEARCHMEMORY: g_Debugger->OpenMemorySearch(); break;
    case ID_DEBUGGER_MEMORY: g_Debugger->OpenMemoryWindow(); break;
    case ID_DEBUGGER_TLBENTRIES: g_Debugger->OpenTLBWindow(); break;
    case ID_DEBUGGER_INTERRUPT_SP: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_SP); break;
    case ID_DEBUGGER_INTERRUPT_SI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_SI); break;
    case ID_DEBUGGER_INTERRUPT_AI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_AI); break;
    case ID_DEBUGGER_INTERRUPT_VI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_VI); break;
    case ID_DEBUGGER_INTERRUPT_PI: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_PI); break;
    case ID_DEBUGGER_INTERRUPT_DP: g_BaseSystem->ExternalEvent(SysEvent_Interrupt_DP); break;
    case ID_DEBUGGER_BREAKPOINTS: g_Debugger->OpenCommandWindow(); break;
    case ID_DEBUGGER_SCRIPTS: g_Debugger->OpenScriptsWindow(); break;
    case ID_DEBUGGER_SYMBOLS: g_Debugger->OpenSymbolsWindow(); break;
    case ID_DEBUGGER_DMALOG: g_Debugger->OpenDMALogWindow(); break;
    case ID_DEBUGGER_CPULOG: g_Debugger->OpenCPULogWindow(); break;
    case ID_DEBUGGER_EXCBREAKPOINTS: g_Debugger->OpenExcBreakpointsWindow(); break;
    case ID_DEBUGGER_STACKTRACE: g_Debugger->OpenStackTraceWindow(); break;
    case ID_DEBUGGER_STACKVIEW: g_Debugger->OpenStackViewWindow(); break;
    case ID_CURRENT_SAVE_DEFAULT:
        g_Notify->DisplayMessage(3, stdstr_f(GS(MENU_SLOT_SAVE), GetSaveSlotString(MenuID - ID_CURRENT_SAVE_DEFAULT).c_str()).c_str());
        g_Settings->SaveDword(Game_CurrentSaveState, (DWORD)(MenuID - ID_CURRENT_SAVE_DEFAULT));
        break;
    case ID_CURRENT_SAVE_1:
    case ID_CURRENT_SAVE_2:
    case ID_CURRENT_SAVE_3:
    case ID_CURRENT_SAVE_4:
    case ID_CURRENT_SAVE_5:
    case ID_CURRENT_SAVE_6:
    case ID_CURRENT_SAVE_7:
    case ID_CURRENT_SAVE_8:
    case ID_CURRENT_SAVE_9:
    case ID_CURRENT_SAVE_10:
    case ID_CURRENT_SAVE_11:
    case ID_CURRENT_SAVE_12:
    case ID_CURRENT_SAVE_13:
    case ID_CURRENT_SAVE_14:
    case ID_CURRENT_SAVE_15:
    case ID_CURRENT_SAVE_16:
    case ID_CURRENT_SAVE_17:
    case ID_CURRENT_SAVE_18:
    case ID_CURRENT_SAVE_19:
    case ID_CURRENT_SAVE_20:
    case ID_CURRENT_SAVE_21:
    case ID_CURRENT_SAVE_22:
    case ID_CURRENT_SAVE_23:
    case ID_CURRENT_SAVE_24:
    case ID_CURRENT_SAVE_25:
    case ID_CURRENT_SAVE_26:
    case ID_CURRENT_SAVE_27:
    case ID_CURRENT_SAVE_28:
    case ID_CURRENT_SAVE_29:
    case ID_CURRENT_SAVE_30:
    case ID_CURRENT_SAVE_31:
    case ID_CURRENT_SAVE_32:
    case ID_CURRENT_SAVE_33:
    case ID_CURRENT_SAVE_34:
    case ID_CURRENT_SAVE_35:
    case ID_CURRENT_SAVE_36:
    case ID_CURRENT_SAVE_37:
    case ID_CURRENT_SAVE_38:
    case ID_CURRENT_SAVE_39:
    case ID_CURRENT_SAVE_40:
    case ID_CURRENT_SAVE_41:
    case ID_CURRENT_SAVE_42:
    case ID_CURRENT_SAVE_43:
    case ID_CURRENT_SAVE_44:
    case ID_CURRENT_SAVE_45:
    case ID_CURRENT_SAVE_46:
    case ID_CURRENT_SAVE_47:
    case ID_CURRENT_SAVE_48:
    case ID_CURRENT_SAVE_49:
    case ID_CURRENT_SAVE_50:
    case ID_CURRENT_SAVE_51:
    case ID_CURRENT_SAVE_52:
    case ID_CURRENT_SAVE_53:
    case ID_CURRENT_SAVE_54:
    case ID_CURRENT_SAVE_55:
    case ID_CURRENT_SAVE_56:
    case ID_CURRENT_SAVE_57:
    case ID_CURRENT_SAVE_58:
    case ID_CURRENT_SAVE_59:
    case ID_CURRENT_SAVE_60:
    case ID_CURRENT_SAVE_61:
    case ID_CURRENT_SAVE_62:
    case ID_CURRENT_SAVE_63:
    case ID_CURRENT_SAVE_64:
    case ID_CURRENT_SAVE_65:
    case ID_CURRENT_SAVE_66:
    case ID_CURRENT_SAVE_67:
    case ID_CURRENT_SAVE_68:
    case ID_CURRENT_SAVE_69:
    case ID_CURRENT_SAVE_70:
    case ID_CURRENT_SAVE_71:
    case ID_CURRENT_SAVE_72:
    case ID_CURRENT_SAVE_73:
    case ID_CURRENT_SAVE_74:
    case ID_CURRENT_SAVE_75:
    case ID_CURRENT_SAVE_76:
    case ID_CURRENT_SAVE_77:
    case ID_CURRENT_SAVE_78:
    case ID_CURRENT_SAVE_79:
    case ID_CURRENT_SAVE_80:
    case ID_CURRENT_SAVE_81:
    case ID_CURRENT_SAVE_82:
    case ID_CURRENT_SAVE_83:
    case ID_CURRENT_SAVE_84:
    case ID_CURRENT_SAVE_85:
    case ID_CURRENT_SAVE_86:
    case ID_CURRENT_SAVE_87:
    case ID_CURRENT_SAVE_88:
    case ID_CURRENT_SAVE_89:
    case ID_CURRENT_SAVE_90:
    case ID_CURRENT_SAVE_91:
    case ID_CURRENT_SAVE_92:
    case ID_CURRENT_SAVE_93:
    case ID_CURRENT_SAVE_94:
    case ID_CURRENT_SAVE_95:
    case ID_CURRENT_SAVE_96:
    case ID_CURRENT_SAVE_97:
    case ID_CURRENT_SAVE_98:
    case ID_CURRENT_SAVE_99:
        g_Notify->DisplayMessage(3, stdstr_f(GS(MENU_SLOT_SAVE), GetSaveSlotString((MenuID - ID_CURRENT_SAVE_1) + 1).c_str()).c_str());
        g_Settings->SaveDword(Game_CurrentSaveState, (DWORD)((MenuID - ID_CURRENT_SAVE_1) + 1));
        break;
    case ID_HELP_SUPPORT_PROJECT64: OnSupportProject64(hWnd); break;
    case ID_HELP_OPEN_APPDATA:
        {
            wchar_t* AppdataPathW = NULL;
            SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &AppdataPathW);
            PathAppend(AppdataPathW, L"Luna-Project64");
            ShellExecute(NULL, L"open", AppdataPathW, NULL, NULL, SW_SHOWNORMAL);
        }
        break;
    case ID_HELP_SUPPORT_LUNA: ShellExecute(nullptr, L"open", L"https://ko-fi.com/shin3", nullptr, nullptr, SW_SHOWMAXIMIZED); break;
    case ID_HELP_DISCORD: ShellExecute(nullptr, L"open", L"https://sites.google.com/view/shurislibrary/discord", nullptr, nullptr, SW_SHOWMAXIMIZED); break;
    case ID_HELP_WEBSITE: ShellExecute(nullptr, L"open", L"https://sites.google.com/view/shurislibrary", nullptr, nullptr, SW_SHOWMAXIMIZED); break;
    case ID_HELP_ABOUT: ShellExecute(nullptr, L"open", L"https://sites.google.com/view/shurislibrary/luna-project64", nullptr, nullptr, SW_SHOWMAXIMIZED); break;
    case ID_DEBUGGER_ENABLE:
        if (g_Notify->AskYesNoQuestion(g_Lang->GetString(MSG_ENABLE_DEBUGGER_MSG).c_str()))
        {
            g_Settings->SaveBool(Debugger_Enabled, 1);
            ResetMenu();
        }
        break;
    case ID_DEBUGGER_DISABLE:
        g_Settings->SaveBool(Debugger_Enabled, 0);
        ResetMenu();
        break;
    default:
        if (MenuID >= ID_RECENT_ROM_START && MenuID < ID_RECENT_ROM_END)
        {
#ifdef RETROACHIEVEMENTS
            if (!RA_ConfirmLoadNewRom(false))
                return false;
#endif
            stdstr FileName;
            if (UISettingsLoadStringIndex(File_RecentGameFileIndex, MenuID - ID_RECENT_ROM_START, FileName) &&
                FileName.length() > 0)
            {
                if ((CPath(FileName).GetExtension() != "ndd") && (CPath(FileName).GetExtension() != "d64"))
                    g_BaseSystem->RunFileImage(FileName.c_str());
                else
                    g_BaseSystem->RunDiskImage(FileName.c_str());
            }
        }
        if (MenuID >= ID_RECENT_DIR_START && MenuID < ID_RECENT_DIR_END)
        {
            int Offset = MenuID - ID_RECENT_DIR_START;
            stdstr Dir = UISettingsLoadStringIndex(Directory_RecentGameDirIndex, Offset);
            if (Dir.length() > 0)
            {
                g_Settings->SaveString(RomList_GameDir, Dir.c_str());
                Notify().AddRecentDir(Dir.c_str());
                m_Gui->RefreshMenu();
                if (m_Gui->RomBrowserVisible())
                {
                    m_Gui->RefreshRomList();
                }
            }
        }
        if (MenuID >= ID_LANG_START && MenuID < ID_LANG_END)
        {
            MENUITEMINFO menuinfo;
            wchar_t String[300];

            menuinfo.cbSize = sizeof(MENUITEMINFO);
            menuinfo.fMask = MIIM_TYPE;
            menuinfo.fType = MFT_STRING;
            menuinfo.dwTypeData = String;
            menuinfo.cch = sizeof(String);
            GetMenuItemInfo((HMENU)m_MenuHandle, MenuID, FALSE, &menuinfo);

            g_Lang->SetLanguage(stdstr().FromUTF16(String).c_str());
            m_Gui->ResetRomBrowserColomuns();
            break;
        }
        return false;
    }
    return true;
}

stdstr CMainMenu::GetFileLastMod(const CPath & FileName)
{
    HANDLE hFile = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return "";
    }
    FILETIME CreationTime, LastAccessTime, LastWriteTime;
    stdstr LastMod;
    if (GetFileTime(hFile, &CreationTime, &LastAccessTime, &LastWriteTime))
    {
        SYSTEMTIME stUTC, stLocal;

        // Convert the last-write time to local time
        FileTimeToSystemTime(&LastWriteTime, &stUTC);
        SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal);

        LastMod.Format(" [%d/%02d/%02d %02d:%02d]", stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute);
    }
    CloseHandle(hFile);

    return LastMod;
}

std::wstring CMainMenu::GetSaveSlotString(int Slot)
{
    stdstr SlotName;
    switch (Slot)
    {
    case 0: SlotName = GS(MENU_SLOT_DEFAULT); break;
    case 1: SlotName = GS(MENU_SLOT_1); break;
    case 2: SlotName = GS(MENU_SLOT_2); break;
    case 3: SlotName = GS(MENU_SLOT_3); break;
    case 4: SlotName = GS(MENU_SLOT_4); break;
    case 5: SlotName = GS(MENU_SLOT_5); break;
    case 6: SlotName = GS(MENU_SLOT_6); break;
    case 7: SlotName = GS(MENU_SLOT_7); break;
    case 8: SlotName = GS(MENU_SLOT_8); break;
    case 9: SlotName = GS(MENU_SLOT_9); break;
    case 10: SlotName = GS(MENU_SLOT_10); break;
    case 11: SlotName = GS(MENU_SLOT_11); break;
    case 12: SlotName = GS(MENU_SLOT_12); break;
    case 13: SlotName = GS(MENU_SLOT_13); break;
    case 14: SlotName = GS(MENU_SLOT_14); break;
    case 15: SlotName = GS(MENU_SLOT_15); break;
    case 16: SlotName = GS(MENU_SLOT_16); break;
    case 17: SlotName = GS(MENU_SLOT_17); break;
    case 18: SlotName = GS(MENU_SLOT_18); break;
    case 19: SlotName = GS(MENU_SLOT_19); break;
    case 20: SlotName = GS(MENU_SLOT_20); break;
    case 21: SlotName = GS(MENU_SLOT_21); break;
    case 22: SlotName = GS(MENU_SLOT_22); break;
    case 23: SlotName = GS(MENU_SLOT_23); break;
    case 24: SlotName = GS(MENU_SLOT_24); break;
    case 25: SlotName = GS(MENU_SLOT_25); break;
    case 26: SlotName = GS(MENU_SLOT_26); break;
    case 27: SlotName = GS(MENU_SLOT_27); break;
    case 28: SlotName = GS(MENU_SLOT_28); break;
    case 29: SlotName = GS(MENU_SLOT_29); break;
    case 30: SlotName = GS(MENU_SLOT_30); break;
    case 31: SlotName = GS(MENU_SLOT_31); break;
    case 32: SlotName = GS(MENU_SLOT_32); break;
    case 33: SlotName = GS(MENU_SLOT_33); break;
    case 34: SlotName = GS(MENU_SLOT_34); break;
    case 35: SlotName = GS(MENU_SLOT_35); break;
    case 36: SlotName = GS(MENU_SLOT_36); break;
    case 37: SlotName = GS(MENU_SLOT_37); break;
    case 38: SlotName = GS(MENU_SLOT_38); break;
    case 39: SlotName = GS(MENU_SLOT_39); break;
    case 40: SlotName = GS(MENU_SLOT_40); break;
    case 41: SlotName = GS(MENU_SLOT_41); break;
    case 42: SlotName = GS(MENU_SLOT_42); break;
    case 43: SlotName = GS(MENU_SLOT_43); break;
    case 44: SlotName = GS(MENU_SLOT_44); break;
    case 45: SlotName = GS(MENU_SLOT_45); break;
    case 46: SlotName = GS(MENU_SLOT_46); break;
    case 47: SlotName = GS(MENU_SLOT_47); break;
    case 48: SlotName = GS(MENU_SLOT_48); break;
    case 49: SlotName = GS(MENU_SLOT_49); break;
    case 50: SlotName = GS(MENU_SLOT_50); break;
    case 51: SlotName = GS(MENU_SLOT_51); break;
    case 52: SlotName = GS(MENU_SLOT_52); break;
    case 53: SlotName = GS(MENU_SLOT_53); break;
    case 54: SlotName = GS(MENU_SLOT_54); break;
    case 55: SlotName = GS(MENU_SLOT_55); break;
    case 56: SlotName = GS(MENU_SLOT_56); break;
    case 57: SlotName = GS(MENU_SLOT_57); break;
    case 58: SlotName = GS(MENU_SLOT_58); break;
    case 59: SlotName = GS(MENU_SLOT_59); break;
    case 60: SlotName = GS(MENU_SLOT_60); break;
    case 61: SlotName = GS(MENU_SLOT_61); break;
    case 62: SlotName = GS(MENU_SLOT_62); break;
    case 63: SlotName = GS(MENU_SLOT_63); break;
    case 64: SlotName = GS(MENU_SLOT_64); break;
    case 65: SlotName = GS(MENU_SLOT_65); break;
    case 66: SlotName = GS(MENU_SLOT_66); break;
    case 67: SlotName = GS(MENU_SLOT_67); break;
    case 68: SlotName = GS(MENU_SLOT_68); break;
    case 69: SlotName = GS(MENU_SLOT_69); break;
    case 70: SlotName = GS(MENU_SLOT_70); break;
    case 71: SlotName = GS(MENU_SLOT_71); break;
    case 72: SlotName = GS(MENU_SLOT_72); break;
    case 73: SlotName = GS(MENU_SLOT_73); break;
    case 74: SlotName = GS(MENU_SLOT_74); break;
    case 75: SlotName = GS(MENU_SLOT_75); break;
    case 76: SlotName = GS(MENU_SLOT_76); break;
    case 77: SlotName = GS(MENU_SLOT_77); break;
    case 78: SlotName = GS(MENU_SLOT_78); break;
    case 79: SlotName = GS(MENU_SLOT_79); break;
    case 80: SlotName = GS(MENU_SLOT_80); break;
    case 81: SlotName = GS(MENU_SLOT_81); break;
    case 82: SlotName = GS(MENU_SLOT_82); break;
    case 83: SlotName = GS(MENU_SLOT_83); break;
    case 84: SlotName = GS(MENU_SLOT_84); break;
    case 85: SlotName = GS(MENU_SLOT_85); break;
    case 86: SlotName = GS(MENU_SLOT_86); break;
    case 87: SlotName = GS(MENU_SLOT_87); break;
    case 88: SlotName = GS(MENU_SLOT_88); break;
    case 89: SlotName = GS(MENU_SLOT_89); break;
    case 90: SlotName = GS(MENU_SLOT_90); break;
    case 91: SlotName = GS(MENU_SLOT_91); break;
    case 92: SlotName = GS(MENU_SLOT_92); break;
    case 93: SlotName = GS(MENU_SLOT_93); break;
    case 94: SlotName = GS(MENU_SLOT_94); break;
    case 95: SlotName = GS(MENU_SLOT_95); break;
    case 96: SlotName = GS(MENU_SLOT_96); break;
    case 97: SlotName = GS(MENU_SLOT_97); break;
    case 98: SlotName = GS(MENU_SLOT_98); break;
    case 99: SlotName = GS(MENU_SLOT_99); break;
    }

    if (!g_Settings->LoadBool(GameRunning_CPU_Running)) { return SlotName.ToUTF16(); }

    stdstr LastSaveTime;

    // Check first save name
    CPath FileName(g_Settings->LoadStringVal(Directory_InstantSave).c_str(), "");
    if (g_Settings->LoadBool(Setting_UniqueSaveDir))
    {
        FileName.AppendDirectory(g_Settings->LoadStringVal(Game_UniqueSaveDir).c_str());
    }
    FileName.NormalizePath(CPath(CPath::MODULE_DIRECTORY));
    if (Slot != 0)
    {
        FileName.SetNameExtension(stdstr_f("%s.pj%d", g_Settings->LoadStringVal(Rdb_GoodName).c_str(), Slot).c_str());
    }
    else
    {
        FileName.SetNameExtension(stdstr_f("%s.pj", g_Settings->LoadStringVal(Rdb_GoodName).c_str()).c_str());
    }

    if (g_Settings->LoadDword(Setting_AutoZipInstantSave))
    {
        CPath ZipFileName(FileName.GetDriveDirectory(), stdstr_f("%s.zip", FileName.GetNameExtension().c_str()).c_str());
        LastSaveTime = GetFileLastMod(ZipFileName);
    }
    if (LastSaveTime.empty())
    {
        LastSaveTime = GetFileLastMod(FileName);
    }

    // Check old file name
    if (LastSaveTime.empty())
    {
        if (Slot > 0)
        {
            FileName.SetNameExtension(stdstr_f("%s.pj%d", g_Settings->LoadStringVal(Game_GameName).c_str(), Slot).c_str());
        }
        else
        {
            FileName.SetNameExtension(stdstr_f("%s.pj", g_Settings->LoadStringVal(Game_GameName).c_str()).c_str());
        }

        if (g_Settings->LoadBool(Setting_AutoZipInstantSave))
        {
            CPath ZipFileName(FileName.GetDriveDirectory(), stdstr_f("%s.zip", FileName.GetNameExtension().c_str()).c_str());
            LastSaveTime = GetFileLastMod(ZipFileName);
        }
        if (LastSaveTime.empty())
        {
            LastSaveTime = GetFileLastMod(FileName);
        }
    }
    SlotName += LastSaveTime;
    return SlotName.ToUTF16();
}

void CMainMenu::FillOutMenu(HMENU hMenu)
{
    CGuard Guard(m_CS);

    MENU_ITEM Item;

    // Get all flags
    bool inBasicMode = g_Settings->LoadBool(UserInterface_BasicMode);
    bool CPURunning = g_Settings->LoadBool(GameRunning_CPU_Running);
    bool RomLoading = g_Settings->LoadBool(GameRunning_LoadingInProgress);
    bool RomLoaded = g_Settings->LoadStringVal(Game_GameName).length() > 0;
    bool RomList = UISettingsLoadBool(RomBrowser_Enabled) && !CPURunning;
    bool Enhancement = g_Settings->LoadBool(Setting_Enhancement);

    CMenuShortCutKey::RUNNING_STATE RunningState = CMenuShortCutKey::RUNNING_STATE_NOT_RUNNING;
    if (g_Settings->LoadBool(GameRunning_CPU_Running))
    {
        RunningState = UISettingsLoadBool(UserInterface_InFullScreen) ? CMenuShortCutKey::RUNNING_STATE_FULLSCREEN : CMenuShortCutKey::RUNNING_STATE_WINDOWED;
    }

    // Get the system information to make the menu
    LanguageList LangList = g_Lang->GetLangList();

    MenuItemList LangMenu;
    int Offset = 0;
    for (LanguageList::iterator Language = LangList.begin(); Language != LangList.end(); Language++)
    {
        Item.Reset(ID_LANG_START + Offset++, EMPTY_STRING, EMPTY_STDSTR, nullptr, stdstr(Language->LanguageName).ToUTF16().c_str());
        if (g_Lang->IsCurrentLang(*Language))
        {
            Item.SetItemTicked(true);
        }
        LangMenu.push_back(Item);
    }

    // Go through the settings to create a list of recent ROMS
    MenuItemList RecentRomMenu;
    DWORD count, RomsToRemember = UISettingsLoadDword(File_RecentGameFileCount);

    for (count = 0; count < RomsToRemember; count++)
    {
        stdstr LastRom = UISettingsLoadStringIndex(File_RecentGameFileIndex, count);
        if (LastRom.empty())
        {
            break;
        }
        stdstr_f MenuString("&%d %s", (count + 1) % 10, LastRom.c_str());

        RecentRomMenu.push_back(MENU_ITEM(ID_RECENT_ROM_START + count, EMPTY_STRING, EMPTY_STDSTR, nullptr, MenuString.ToUTF16(CP_ACP).c_str()));
    }

    // Recent directory
    MenuItemList RecentDirMenu;
    DWORD DirsToRemember = UISettingsLoadDword(Directory_RecentGameDirCount);

    for (count = 0; count < DirsToRemember; count++)
    {
        stdstr LastDir = UISettingsLoadStringIndex(Directory_RecentGameDirIndex, count);
        if (LastDir.empty())
        {
            break;
        }

        stdstr_f MenuString("&%d %s", (count + 1) % 10, LastDir.c_str());

        RecentDirMenu.push_back(MENU_ITEM(ID_RECENT_DIR_START + count, EMPTY_STRING, EMPTY_STDSTR, nullptr, MenuString.ToUTF16(CP_ACP).c_str()));
    }

    // File menu
    MenuItemList FileMenu;
    Item.Reset(ID_FILE_OPEN_ROM, MENU_OPEN, m_ShortCuts.ShortCutString(ID_FILE_OPEN_ROM, RunningState));
    FileMenu.push_back(Item);
    Item.Reset(ID_FILE_OPEN_COMBO, MENU_OPEN_COMBO, m_ShortCuts.ShortCutString(ID_FILE_OPEN_COMBO, RunningState));
    FileMenu.push_back(Item);
    if (SdCardMounter::State::VHD == SdCardMounter::getState())
    {
        FileMenu.push_back(MENU_ITEM(ID_FILE_MOUNT_SDCARD, MENU_UNMOUNT_SDCARD, m_ShortCuts.ShortCutString(ID_FILE_MOUNT_SDCARD, RunningState)));
    }
    else
    {
        FileMenu.push_back(MENU_ITEM(ID_FILE_MOUNT_SDCARD, MENU_MOUNT_SDCARD, m_ShortCuts.ShortCutString(ID_FILE_MOUNT_SDCARD, RunningState)));
    }
    if (!inBasicMode)
    {
        Item.Reset(ID_FILE_ROM_INFO, MENU_ROM_INFO, m_ShortCuts.ShortCutString(ID_FILE_ROM_INFO, RunningState));
        Item.SetItemEnabled(RomLoaded);
        FileMenu.push_back(Item);
        FileMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(ID_FILE_STARTEMULATION, MENU_START, m_ShortCuts.ShortCutString(ID_FILE_STARTEMULATION, RunningState));
        Item.SetItemEnabled(RomLoaded && !CPURunning);
        FileMenu.push_back(Item);
    }
    Item.Reset(ID_FILE_ENDEMULATION, MENU_END, m_ShortCuts.ShortCutString(ID_FILE_ENDEMULATION, RunningState));
    Item.SetItemEnabled(CPURunning);
    FileMenu.push_back(Item);
    FileMenu.push_back(MENU_ITEM(SPLITER));
    Item.Reset(SUB_MENU, MENU_LANGUAGE, EMPTY_STDSTR, &LangMenu);
    FileMenu.push_back(Item);
    if (RomList)
    {
        FileMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(ID_FILE_ROMDIRECTORY, MENU_CHOOSE_ROM, m_ShortCuts.ShortCutString(ID_FILE_ROMDIRECTORY, RunningState));
        FileMenu.push_back(Item);
        Item.Reset(ID_FILE_REFRESHROMLIST, MENU_REFRESH, m_ShortCuts.ShortCutString(ID_FILE_REFRESHROMLIST, RunningState));
        FileMenu.push_back(Item);
    }

    if (!inBasicMode && RomList)
    {
        FileMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(SUB_MENU, MENU_RECENT_ROM, EMPTY_STDSTR, &RecentRomMenu);
        if (RecentRomMenu.size() == 0)
        {
            RecentRomMenu.push_back(MENU_ITEM(SPLITER));
            Item.SetItemEnabled(false);
        }
        FileMenu.push_back(Item);
        Item.Reset(SUB_MENU, MENU_RECENT_DIR, EMPTY_STDSTR, &RecentDirMenu);
        if (RecentDirMenu.size() == 0)
        {
            RecentDirMenu.push_back(MENU_ITEM(SPLITER));
            Item.SetItemEnabled(false);
        }
        FileMenu.push_back(Item);
    }
    else
    {
        if (RecentRomMenu.size() != 0)
        {
            FileMenu.push_back(MENU_ITEM(SPLITER));
            for (MenuItemList::iterator MenuItem = RecentRomMenu.begin(); MenuItem != RecentRomMenu.end(); MenuItem++)
            {
                FileMenu.push_back(*MenuItem);
            }
        }
    }
    FileMenu.push_back(MENU_ITEM(SPLITER));
    FileMenu.push_back(MENU_ITEM(ID_FILE_EXIT, MENU_EXIT, m_ShortCuts.ShortCutString(ID_FILE_EXIT, RunningState)));

    // Current save
    MenuItemList CurrentSaveMenu;
    DWORD _CurrentSaveState = g_Settings->LoadDword(Game_CurrentSaveState);
    Item.Reset(ID_CURRENT_SAVE_DEFAULT, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_DEFAULT, RunningState), nullptr, GetSaveSlotString(0));
    if (_CurrentSaveState == 0) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    CurrentSaveMenu.push_back(MENU_ITEM(SPLITER));
    Item.Reset(ID_CURRENT_SAVE_1, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_1, RunningState), nullptr, GetSaveSlotString(1));
    if (_CurrentSaveState == 1) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_2, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_2, RunningState), nullptr, GetSaveSlotString(2));
    if (_CurrentSaveState == 2) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_3, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_3, RunningState), nullptr, GetSaveSlotString(3));
    if (_CurrentSaveState == 3) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_4, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_4, RunningState), nullptr, GetSaveSlotString(4));
    if (_CurrentSaveState == 4) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_5, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_5, RunningState), nullptr, GetSaveSlotString(5));
    if (_CurrentSaveState == 5) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_6, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_6, RunningState), nullptr, GetSaveSlotString(6));
    if (_CurrentSaveState == 6) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_7, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_7, RunningState), nullptr, GetSaveSlotString(7));
    if (_CurrentSaveState == 7) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_8, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_8, RunningState), nullptr, GetSaveSlotString(8));
    if (_CurrentSaveState == 8) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_9, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_9, RunningState), nullptr, GetSaveSlotString(9));
    if (_CurrentSaveState == 9) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_10, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_10, RunningState), nullptr, GetSaveSlotString(10));
    if (_CurrentSaveState == 10) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_11, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_11, RunningState), nullptr, GetSaveSlotString(11));
    if (_CurrentSaveState == 11) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_12, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_12, RunningState), nullptr, GetSaveSlotString(12));
    if (_CurrentSaveState == 12) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_13, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_13, RunningState), nullptr, GetSaveSlotString(13));
    if (_CurrentSaveState == 13) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_14, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_14, RunningState), nullptr, GetSaveSlotString(14));
    if (_CurrentSaveState == 14) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_15, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_15, RunningState), nullptr, GetSaveSlotString(15));
    if (_CurrentSaveState == 15) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_16, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_16, RunningState), nullptr, GetSaveSlotString(16));
    if (_CurrentSaveState == 16) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_17, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_17, RunningState), nullptr, GetSaveSlotString(17));
    if (_CurrentSaveState == 17) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_18, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_18, RunningState), nullptr, GetSaveSlotString(18));
    if (_CurrentSaveState == 18) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_19, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_19, RunningState), nullptr, GetSaveSlotString(19));
    if (_CurrentSaveState == 19) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_20, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_20, RunningState), nullptr, GetSaveSlotString(20));
    if (_CurrentSaveState == 20) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_21, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_21, RunningState), nullptr, GetSaveSlotString(21));
    if (_CurrentSaveState == 21) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_22, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_22, RunningState), nullptr, GetSaveSlotString(22));
    if (_CurrentSaveState == 22) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_23, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_23, RunningState), nullptr, GetSaveSlotString(23));
    if (_CurrentSaveState == 23) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_24, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_24, RunningState), nullptr, GetSaveSlotString(24));
    if (_CurrentSaveState == 24) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_25, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_25, RunningState), nullptr, GetSaveSlotString(25));
    if (_CurrentSaveState == 25) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_26, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_26, RunningState), nullptr, GetSaveSlotString(26));
    if (_CurrentSaveState == 26) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_27, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_27, RunningState), nullptr, GetSaveSlotString(27));
    if (_CurrentSaveState == 27) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_28, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_28, RunningState), nullptr, GetSaveSlotString(28));
    if (_CurrentSaveState == 28) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_29, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_29, RunningState), nullptr, GetSaveSlotString(29));
    if (_CurrentSaveState == 29) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_30, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_30, RunningState), nullptr, GetSaveSlotString(30));
    if (_CurrentSaveState == 30) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_31, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_31, RunningState), nullptr, GetSaveSlotString(31));
    if (_CurrentSaveState == 31) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_32, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_32, RunningState), nullptr, GetSaveSlotString(32));
    if (_CurrentSaveState == 32) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_33, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_33, RunningState), nullptr, GetSaveSlotString(33));
    if (_CurrentSaveState == 33) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_34, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_34, RunningState), nullptr, GetSaveSlotString(34));
    if (_CurrentSaveState == 34) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_35, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_35, RunningState), nullptr, GetSaveSlotString(35));
    if (_CurrentSaveState == 35) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_36, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_36, RunningState), nullptr, GetSaveSlotString(36));
    if (_CurrentSaveState == 36) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_37, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_37, RunningState), nullptr, GetSaveSlotString(37));
    if (_CurrentSaveState == 37) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_38, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_38, RunningState), nullptr, GetSaveSlotString(38));
    if (_CurrentSaveState == 38) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_39, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_39, RunningState), nullptr, GetSaveSlotString(39));
    if (_CurrentSaveState == 39) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_40, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_40, RunningState), nullptr, GetSaveSlotString(40));
    if (_CurrentSaveState == 40) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_41, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_41, RunningState), nullptr, GetSaveSlotString(41));
    if (_CurrentSaveState == 41) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_42, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_42, RunningState), nullptr, GetSaveSlotString(42));
    if (_CurrentSaveState == 42) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_43, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_43, RunningState), nullptr, GetSaveSlotString(43));
    if (_CurrentSaveState == 43) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_44, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_44, RunningState), nullptr, GetSaveSlotString(44));
    if (_CurrentSaveState == 44) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_45, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_45, RunningState), nullptr, GetSaveSlotString(45));
    if (_CurrentSaveState == 45) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_46, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_46, RunningState), nullptr, GetSaveSlotString(46));
    if (_CurrentSaveState == 46) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_47, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_47, RunningState), nullptr, GetSaveSlotString(47));
    if (_CurrentSaveState == 47) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_48, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_48, RunningState), nullptr, GetSaveSlotString(48));
    if (_CurrentSaveState == 48) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_49, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_49, RunningState), nullptr, GetSaveSlotString(49));
    if (_CurrentSaveState == 49) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_50, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_50, RunningState), nullptr, GetSaveSlotString(50));
    if (_CurrentSaveState == 50) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_51, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_51, RunningState), nullptr, GetSaveSlotString(51));
    if (_CurrentSaveState == 51) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_52, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_52, RunningState), nullptr, GetSaveSlotString(52));
    if (_CurrentSaveState == 52) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_53, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_53, RunningState), nullptr, GetSaveSlotString(53));
    if (_CurrentSaveState == 53) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_54, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_54, RunningState), nullptr, GetSaveSlotString(54));
    if (_CurrentSaveState == 54) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_55, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_55, RunningState), nullptr, GetSaveSlotString(55));
    if (_CurrentSaveState == 55) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_56, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_56, RunningState), nullptr, GetSaveSlotString(56));
    if (_CurrentSaveState == 56) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_57, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_57, RunningState), nullptr, GetSaveSlotString(57));
    if (_CurrentSaveState == 57) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_58, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_58, RunningState), nullptr, GetSaveSlotString(58));
    if (_CurrentSaveState == 58) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_59, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_59, RunningState), nullptr, GetSaveSlotString(59));
    if (_CurrentSaveState == 59) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_60, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_60, RunningState), nullptr, GetSaveSlotString(60));
    if (_CurrentSaveState == 60) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_61, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_61, RunningState), nullptr, GetSaveSlotString(61));
    if (_CurrentSaveState == 61) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_62, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_62, RunningState), nullptr, GetSaveSlotString(62));
    if (_CurrentSaveState == 62) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_63, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_63, RunningState), nullptr, GetSaveSlotString(63));
    if (_CurrentSaveState == 63) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_64, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_64, RunningState), nullptr, GetSaveSlotString(64));
    if (_CurrentSaveState == 64) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_65, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_65, RunningState), nullptr, GetSaveSlotString(65));
    if (_CurrentSaveState == 65) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_66, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_66, RunningState), nullptr, GetSaveSlotString(66));
    if (_CurrentSaveState == 66) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_67, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_67, RunningState), nullptr, GetSaveSlotString(67));
    if (_CurrentSaveState == 67) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_68, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_68, RunningState), nullptr, GetSaveSlotString(68));
    if (_CurrentSaveState == 68) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_69, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_69, RunningState), nullptr, GetSaveSlotString(69));
    if (_CurrentSaveState == 69) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_70, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_70, RunningState), nullptr, GetSaveSlotString(70));
    if (_CurrentSaveState == 70) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_71, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_71, RunningState), nullptr, GetSaveSlotString(71));
    if (_CurrentSaveState == 71) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_72, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_72, RunningState), nullptr, GetSaveSlotString(72));
    if (_CurrentSaveState == 72) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_73, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_73, RunningState), nullptr, GetSaveSlotString(73));
    if (_CurrentSaveState == 73) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_74, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_74, RunningState), nullptr, GetSaveSlotString(74));
    if (_CurrentSaveState == 74) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_75, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_75, RunningState), nullptr, GetSaveSlotString(75));
    if (_CurrentSaveState == 75) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_76, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_76, RunningState), nullptr, GetSaveSlotString(76));
    if (_CurrentSaveState == 76) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_77, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_77, RunningState), nullptr, GetSaveSlotString(77));
    if (_CurrentSaveState == 77) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_78, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_78, RunningState), nullptr, GetSaveSlotString(78));
    if (_CurrentSaveState == 78) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_79, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_79, RunningState), nullptr, GetSaveSlotString(79));
    if (_CurrentSaveState == 79) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_80, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_80, RunningState), nullptr, GetSaveSlotString(80));
    if (_CurrentSaveState == 80) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_81, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_81, RunningState), nullptr, GetSaveSlotString(81));
    if (_CurrentSaveState == 81) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_82, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_82, RunningState), nullptr, GetSaveSlotString(82));
    if (_CurrentSaveState == 82) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_83, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_83, RunningState), nullptr, GetSaveSlotString(83));
    if (_CurrentSaveState == 83) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_84, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_84, RunningState), nullptr, GetSaveSlotString(84));
    if (_CurrentSaveState == 84) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_85, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_85, RunningState), nullptr, GetSaveSlotString(85));
    if (_CurrentSaveState == 85) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_86, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_86, RunningState), nullptr, GetSaveSlotString(86));
    if (_CurrentSaveState == 86) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_87, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_87, RunningState), nullptr, GetSaveSlotString(87));
    if (_CurrentSaveState == 87) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_88, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_88, RunningState), nullptr, GetSaveSlotString(88));
    if (_CurrentSaveState == 88) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_89, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_89, RunningState), nullptr, GetSaveSlotString(89));
    if (_CurrentSaveState == 89) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_90, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_90, RunningState), nullptr, GetSaveSlotString(90));
    if (_CurrentSaveState == 90) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_91, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_91, RunningState), nullptr, GetSaveSlotString(91));
    if (_CurrentSaveState == 91) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_92, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_92, RunningState), nullptr, GetSaveSlotString(92));
    if (_CurrentSaveState == 92) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_93, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_93, RunningState), nullptr, GetSaveSlotString(93));
    if (_CurrentSaveState == 93) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_94, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_94, RunningState), nullptr, GetSaveSlotString(94));
    if (_CurrentSaveState == 94) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_95, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_95, RunningState), nullptr, GetSaveSlotString(95));
    if (_CurrentSaveState == 95) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_96, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_96, RunningState), nullptr, GetSaveSlotString(96));
    if (_CurrentSaveState == 96) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_97, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_97, RunningState), nullptr, GetSaveSlotString(97));
    if (_CurrentSaveState == 97) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_98, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_98, RunningState), nullptr, GetSaveSlotString(98));
    if (_CurrentSaveState == 98) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);
    Item.Reset(ID_CURRENT_SAVE_99, EMPTY_STRING, m_ShortCuts.ShortCutString(ID_CURRENT_SAVE_99, RunningState), nullptr, GetSaveSlotString(99));
    if (_CurrentSaveState == 99) { Item.SetItemTicked(true); }
    CurrentSaveMenu.push_back(Item);

    // System menu
    MenuItemList SystemMenu;
    MenuItemList ResetMenu;
    if (inBasicMode)
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_SOFT, RunningState)));
    }
    else
    {
        ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_SOFT, MENU_RESET_SOFT, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_SOFT, RunningState)));
        ResetMenu.push_back(MENU_ITEM(ID_SYSTEM_RESET_HARD, MENU_RESET_HARD, m_ShortCuts.ShortCutString(ID_SYSTEM_RESET_HARD, RunningState)));
        SystemMenu.push_back(MENU_ITEM(SUB_MENU, MENU_RESET, EMPTY_STDSTR, &ResetMenu));
    }
    if (g_Settings->LoadBool(GameRunning_CPU_Paused))
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_RESUME, m_ShortCuts.ShortCutString(ID_SYSTEM_PAUSE, RunningState)));
    }
    else
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_PAUSE, MENU_PAUSE, m_ShortCuts.ShortCutString(ID_SYSTEM_PAUSE, RunningState)));
    }
    SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_BITMAP, MENU_BITMAP, m_ShortCuts.ShortCutString(ID_SYSTEM_BITMAP, RunningState)));
    SystemMenu.push_back(MENU_ITEM(SPLITER));
    if (!inBasicMode)
    {
        Item.Reset(ID_SYSTEM_LIMITFPS, MENU_LIMIT_FPS, m_ShortCuts.ShortCutString(ID_SYSTEM_LIMITFPS, RunningState));
        if (g_Settings->LoadBool(GameRunning_LimitFPS)) { Item.SetItemTicked(true); }
        SystemMenu.push_back(Item);
        SystemMenu.push_back(MENU_ITEM(SPLITER));
    }
    Item.Reset(ID_SYSTEM_SWAPDISK, MENU_SWAPDISK, m_ShortCuts.ShortCutString(ID_SYSTEM_SWAPDISK, RunningState));
    if (g_Disk == nullptr) { Item.SetItemEnabled(false); }
    SystemMenu.push_back(Item);
    SystemMenu.push_back(MENU_ITEM(SPLITER));
    SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVE, MENU_SAVE, m_ShortCuts.ShortCutString(ID_SYSTEM_SAVE, RunningState)));
    if (!inBasicMode)
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_SAVEAS, MENU_SAVE_AS, m_ShortCuts.ShortCutString(ID_SYSTEM_SAVEAS, RunningState)));
    }
    SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_RESTORE, MENU_RESTORE, m_ShortCuts.ShortCutString(ID_SYSTEM_RESTORE, RunningState)));
    if (!inBasicMode)
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_LOAD, MENU_LOAD, m_ShortCuts.ShortCutString(ID_SYSTEM_LOAD, RunningState)));
    }
    SystemMenu.push_back(MENU_ITEM(SPLITER));
    SystemMenu.push_back(MENU_ITEM(SUB_MENU, MENU_CURRENT_SAVE, EMPTY_STDSTR, &CurrentSaveMenu));
    SystemMenu.push_back(MENU_ITEM(SPLITER));
    if (Enhancement)
    {
        SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_ENHANCEMENT, MENU_ENHANCEMENT, m_ShortCuts.ShortCutString(ID_SYSTEM_ENHANCEMENT, RunningState)));
    }
    SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_CHEAT, MENU_CHEAT, m_ShortCuts.ShortCutString(ID_SYSTEM_CHEAT, RunningState)));
    SystemMenu.push_back(MENU_ITEM(ID_SYSTEM_GSBUTTON, MENU_GS_BUTTON, m_ShortCuts.ShortCutString(ID_SYSTEM_GSBUTTON, RunningState)));

    // Option menu
    MenuItemList OptionMenu;
    Item.Reset(ID_OPTIONS_FULLSCREEN, MENU_FULL_SCREEN, m_ShortCuts.ShortCutString(ID_OPTIONS_FULLSCREEN, RunningState));
    Item.SetItemEnabled(CPURunning);
    if (g_Plugins && g_Plugins->Gfx() && g_Plugins->Gfx()->ChangeWindow == nullptr)
    {
        Item.SetItemEnabled(false);
    }
    OptionMenu.push_back(Item);
    if (!inBasicMode)
    {
        Item.Reset(ID_OPTIONS_ALWAYSONTOP, MENU_ON_TOP, m_ShortCuts.ShortCutString(ID_OPTIONS_ALWAYSONTOP, RunningState));
        if (UISettingsLoadDword(UserInterface_AlwaysOnTop)) { Item.SetItemTicked(true); }
        Item.SetItemEnabled(CPURunning);
        OptionMenu.push_back(Item);
    }
    OptionMenu.push_back(MENU_ITEM(SPLITER));

    Item.Reset(ID_OPTIONS_CONFIG_GFX, MENU_CONFG_GFX, m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_GFX, RunningState));
    if (g_Plugins && g_Plugins->Gfx() == nullptr || g_Plugins->Gfx()->DllConfig == nullptr)
    {
        Item.SetItemEnabled(false);
    }
    OptionMenu.push_back(Item);
    Item.Reset(ID_OPTIONS_CONFIG_AUDIO, MENU_CONFG_AUDIO, m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_AUDIO, RunningState));
    if (g_Plugins->Audio() == nullptr || g_Plugins->Audio()->DllConfig == nullptr)
    {
        Item.SetItemEnabled(false);
    }
    OptionMenu.push_back(Item);
    if (!inBasicMode)
    {
        Item.Reset(ID_OPTIONS_CONFIG_RSP, MENU_CONFG_RSP, m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_RSP, RunningState));
        if (g_Plugins->RSP() == nullptr || g_Plugins->RSP()->DllConfig == nullptr)
        {
            Item.SetItemEnabled(false);
        }
        OptionMenu.push_back(Item);
    }
    Item.Reset(ID_OPTIONS_CONFIG_CONT, MENU_CONFG_CTRL, m_ShortCuts.ShortCutString(ID_OPTIONS_CONFIG_CONT, RunningState));
    if (g_Plugins && g_Plugins->Control() == nullptr || g_Plugins->Control()->DllConfig == nullptr)
    {
        Item.SetItemEnabled(false);
    }
    OptionMenu.push_back(Item);

    OptionMenu.push_back(MENU_ITEM(SPLITER));
    if (!inBasicMode)
    {
        Item.Reset(ID_OPTIONS_CPU_USAGE, MENU_SHOW_CPU, m_ShortCuts.ShortCutString(ID_OPTIONS_CPU_USAGE, RunningState));
        if (g_Settings->LoadDword(UserInterface_ShowCPUPer)) { Item.SetItemTicked(true); }
        OptionMenu.push_back(Item);
    }
    OptionMenu.push_back(MENU_ITEM(ID_OPTIONS_SETTINGS, MENU_SETTINGS, m_ShortCuts.ShortCutString(ID_OPTIONS_SETTINGS, RunningState)));

    // Profile menu
    MenuItemList DebugProfileMenu;
    if (HaveDebugger())
    {
        Item.Reset(ID_PROFILE_PROFILE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Record Execution Times");
        if (g_Settings->LoadBool(Debugger_RecordExecutionTimes)) { Item.SetItemTicked(true); }
        DebugProfileMenu.push_back(Item);
        Item.Reset(ID_PROFILE_RESETCOUNTER, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Reset Counters");
        if (!CPURunning) { Item.SetItemEnabled(false); }
        DebugProfileMenu.push_back(Item);
        Item.Reset(ID_PROFILE_GENERATELOG, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Generate Log File");
        if (!CPURunning) { Item.SetItemEnabled(false); }
        DebugProfileMenu.push_back(Item);
    }

    // Debugger menu
    MenuItemList DebugMenu;
    MenuItemList DebugLoggingMenu;
    MenuItemList DebugAppLoggingMenu;
    MenuItemList DebugR4300Menu;
    MenuItemList DebugMemoryMenu;
    MenuItemList DebugInterrupt;
    MenuItemList DebugNotificationMenu;
    if (HaveDebugger())
    {
        // Debug - interrupt
        Item.Reset(ID_DEBUGGER_INTERRUPT_SP, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"SP interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);
        Item.Reset(ID_DEBUGGER_INTERRUPT_SI, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"SI interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);
        Item.Reset(ID_DEBUGGER_INTERRUPT_AI, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"AI interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);
        Item.Reset(ID_DEBUGGER_INTERRUPT_VI, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"VI interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);
        Item.Reset(ID_DEBUGGER_INTERRUPT_PI, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"PI interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);
        Item.Reset(ID_DEBUGGER_INTERRUPT_DP, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"DP interrupt");
        Item.SetItemEnabled(CPURunning);
        DebugInterrupt.push_back(Item);

        // Debug - R4300i

        // ID_DEBUGGER_LOGOPTIONS
        Item.Reset(ID_DEBUGGER_BREAKPOINTS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"&Commands...");
        DebugR4300Menu.push_back(Item);
        Item.Reset(ID_DEBUGGER_CPULOG, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Command log...");
        DebugR4300Menu.push_back(Item);
        Item.Reset(ID_DEBUGGER_EXCBREAKPOINTS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Exceptions...");
        DebugR4300Menu.push_back(Item);
        Item.Reset(ID_DEBUGGER_STACKVIEW, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Stack...");
        DebugR4300Menu.push_back(Item);
        Item.Reset(ID_DEBUGGER_STACKTRACE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Stack lrace...");
        DebugR4300Menu.push_back(Item);

        Item.Reset(ID_DEBUG_DISABLE_GAMEFIX, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Disable game fixes");
        if (g_Settings->LoadBool(Debugger_DisableGameFixes))
        {
            Item.SetItemTicked(true);
        }
        DebugR4300Menu.push_back(Item);
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugInterrupt, L"&Generate interrupt");
        DebugR4300Menu.push_back(Item);

        // Debug - memory
        Item.Reset(ID_DEBUGGER_MEMORY, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"View...");
        DebugMemoryMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_SEARCHMEMORY, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Search...");
        DebugMemoryMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_SYMBOLS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Symbols...");
        DebugMemoryMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_DUMPMEMORY, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Dump...");
        DebugMemoryMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_TLBENTRIES, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"TLB entries...");
        DebugMemoryMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_DMALOG, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"DMA log...");
        DebugMemoryMenu.push_back(Item);

        // Debug - app logging
        Item.Reset(ID_DEBUGGER_TRACE_MD5, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"MD5");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceMD5) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_SETTINGS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Settings");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceSettings) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_UNKNOWN, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Unknown");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceUnknown) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_APPINIT, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"App initialization");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceAppInit) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_APPCLEANUP, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"App cleanup");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceAppCleanup) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_N64SYSTEM, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"N64 system");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceN64System) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_PLUGINS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Plugins");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TracePlugins) == TraceVerbose);;
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_GFXPLUGIN, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"GFX plugin");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceGFXPlugin) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_AUDIOPLUGIN, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Audio plugin");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceAudioPlugin) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_CONTROLLERPLUGIN, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Controller plugin");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceControllerPlugin) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_RSPPLUGIN, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"RSP plugin");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceRSPPlugin) == TraceVerbose);;
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_RSP, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"RSP");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceRSP) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_AUDIO, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Audio");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceAudio) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_REGISTERCACHE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Register cache");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceRegisterCache) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_RECOMPILER, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Recompiler");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceRecompiler) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_TLB, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"TLB");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceTLB) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_PROTECTEDMEM, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Protected MEM");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceProtectedMEM) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_TRACE_USERINTERFACE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"User interface");
        Item.SetItemTicked(g_Settings->LoadDword(Debugger_TraceUserInterface) == TraceVerbose);
        DebugAppLoggingMenu.push_back(Item);

        DebugAppLoggingMenu.push_back(MENU_ITEM(SPLITER));

        Item.Reset(ID_DEBUGGER_APPLOG_FLUSH, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Auto flush file");
        if (g_Settings->LoadBool(Debugger_AppLogFlush)) { Item.SetItemTicked(true); }
        DebugAppLoggingMenu.push_back(Item);

        // Debug - logging
        Item.Reset(ID_DEBUGGER_LOGOPTIONS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Log options...");
        DebugLoggingMenu.push_back(Item);

        Item.Reset(ID_DEBUGGER_GENERATELOG, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Generate log");
        if (g_Settings->LoadBool(Logging_GenerateLog)) { Item.SetItemTicked(true); }
        DebugLoggingMenu.push_back(Item);

        // Debugger main menu
        Item.Reset(ID_DEBUGGER_BREAKPOINTS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Commands...");
        DebugMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_MEMORY, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"View memory...");
        DebugMenu.push_back(Item);
        Item.Reset(ID_DEBUGGER_SCRIPTS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Scripts...");
        DebugMenu.push_back(Item);

        DebugMenu.push_back(MENU_ITEM(SPLITER));

        // Debug - memory
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugMemoryMenu, L"Memory");
        DebugMenu.push_back(Item);

        // Debug - R4300i
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugR4300Menu, L"&R4300i");
        DebugMenu.push_back(Item);

        // Debug - RSP
        if (g_Plugins && g_Plugins->RSP() != nullptr && IsMenu((HMENU)g_Plugins->RSP()->GetDebugMenu()))
        {
            Item.Reset(ID_PLUGIN_MENU, EMPTY_STRING, EMPTY_STDSTR, g_Plugins->RSP()->GetDebugMenu(), L"&RSP");
            DebugMenu.push_back(Item);
        }

        // Debug - RDP
        if (g_Plugins && g_Plugins->Gfx() != nullptr && IsMenu((HMENU)g_Plugins->Gfx()->GetDebugMenu()))
        {
            Item.Reset(ID_PLUGIN_MENU, EMPTY_STRING, EMPTY_STDSTR, g_Plugins->Gfx()->GetDebugMenu(), L"&RDP");
            DebugMenu.push_back(Item);
        }

        // Notification menu
        Item.Reset(ID_DEBUG_SHOW_UNHANDLED_MEM, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"On unhandled memory actions");
        if (g_Settings->LoadBool(Debugger_ShowUnhandledMemory))
        {
            Item.SetItemTicked(true);
        }
        DebugNotificationMenu.push_back(Item);
        Item.Reset(ID_DEBUG_SHOW_PIF_ERRORS, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"On PIF errors");
        if (g_Settings->LoadBool(Debugger_ShowPifErrors))
        {
            Item.SetItemTicked(true);
        }
        DebugNotificationMenu.push_back(Item);
        Item.Reset(ID_DEBUG_SHOW_DIV_BY_ZERO, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"On division by zero errors");
        if (g_Settings->LoadBool(Debugger_ShowDivByZero))
        {
            Item.SetItemTicked(true);
        }
        DebugNotificationMenu.push_back(Item);

        DebugMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugProfileMenu, L"Profile");
        DebugMenu.push_back(Item);
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugAppLoggingMenu, L"App logging");
        DebugMenu.push_back(Item);
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugLoggingMenu, L"Logging");
        DebugMenu.push_back(Item);
        Item.Reset(SUB_MENU, EMPTY_STRING, EMPTY_STDSTR, &DebugNotificationMenu, L"Notification");
        DebugMenu.push_back(Item);
        DebugMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(ID_DEBUG_SHOW_TLB_MISSES, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Show TLB misses");
        if (g_Settings->LoadBool(Debugger_ShowTLBMisses))
        {
            Item.SetItemTicked(true);
        }
        DebugMenu.push_back(Item);
        Item.Reset(ID_DEBUG_SHOW_DLIST_COUNT, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Display Alist/Dlist count");
        if (g_Settings->LoadBool(Debugger_ShowDListAListCount))
        {
            Item.SetItemTicked(true);
        }
        DebugMenu.push_back(Item);
        Item.Reset(ID_DEBUG_LANGUAGE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Debug language");
        if (g_Settings->LoadBool(Debugger_DebugLanguage))
        {
            Item.SetItemTicked(true);
        }
        DebugMenu.push_back(Item);
        Item.Reset(ID_DEBUG_SHOW_RECOMP_MEM_SIZE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Display recompiler code buffer size");
        if (g_Settings->LoadBool(Debugger_ShowRecompMemSize))
        {
            Item.SetItemTicked(true);
        }
        DebugMenu.push_back(Item);
        DebugMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(ID_DEBUG_RECORD_RECOMPILER_ASM, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Record recompiler ASM");
        if (g_Settings->LoadBool(Debugger_RecordRecompilerAsm))
        {
            Item.SetItemTicked(true);
        }
        DebugMenu.push_back(Item);
        DebugMenu.push_back(MENU_ITEM(SPLITER));
        Item.Reset(ID_DEBUGGER_DISABLE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Disable Debugger");
        DebugMenu.push_back(Item);

    }
    else {
        Item.Reset(ID_DEBUGGER_ENABLE, EMPTY_STRING, EMPTY_STDSTR, nullptr, L"Enable Debugger");
        DebugMenu.push_back(Item);
    }

    // Help menu
    MenuItemList HelpMenu;
    HelpMenu.push_back(MENU_ITEM(ID_HELP_OPEN_APPDATA, MENU_OPEN_APPDATA));
    HelpMenu.push_back(MENU_ITEM(ID_HELP_SUPPORT_PROJECT64, MENU_SUPPORT_PROJECT64));
    HelpMenu.push_back(MENU_ITEM(ID_HELP_SUPPORT_LUNA, MENU_SUPPORT_LUNA));
    HelpMenu.push_back(MENU_ITEM(ID_HELP_DISCORD, MENU_DISCORD));
    HelpMenu.push_back(MENU_ITEM(ID_HELP_WEBSITE, MENU_WEBSITE));
    HelpMenu.push_back(MENU_ITEM(SPLITER));
    HelpMenu.push_back(MENU_ITEM(ID_HELP_ABOUT, MENU_ABOUT_PJ64));

    // Main title bar Menu
    MenuItemList MainTitleMenu;
    Item.Reset(SUB_MENU, MENU_FILE, EMPTY_STDSTR, &FileMenu);
    if (RomLoading) { Item.SetItemEnabled(false); }
    MainTitleMenu.push_back(Item);
    if (CPURunning)
    {
        Item.Reset(SUB_MENU, MENU_SYSTEM, EMPTY_STDSTR, &SystemMenu);
        if (RomLoading) { Item.SetItemEnabled(false); }
        MainTitleMenu.push_back(Item);
    }
    Item.Reset(SUB_MENU, MENU_OPTIONS, EMPTY_STDSTR, &OptionMenu);
    if (RomLoading) { Item.SetItemEnabled(false); }
    MainTitleMenu.push_back(Item);
    if (!inBasicMode)
    {
        Item.Reset(SUB_MENU, MENU_DEBUGGER, EMPTY_STDSTR, &DebugMenu);
        if (RomLoading) { Item.SetItemEnabled(false); }
        MainTitleMenu.push_back(Item);
    }
    Item.Reset(SUB_MENU, MENU_HELP, EMPTY_STDSTR, &HelpMenu);
    if (RomLoading) { Item.SetItemEnabled(false); }
    MainTitleMenu.push_back(Item);

    if (UISettingsLoadBool(UserInterface_ShowingNagWindow))
    {
        for (MenuItemList::iterator MenuItem = MainTitleMenu.begin(); MenuItem != MainTitleMenu.end(); MenuItem++)
        {
            MenuItem->SetItemEnabled(false);
        }
    }
    AddMenu(hMenu, MainTitleMenu);
}

void CMainMenu::RebuildAccelerators(void)
{
    CGuard Guard(m_CS);

    // Delete the old accel list
    WriteTrace(TraceUserInterface, TraceDebug, "Start");

    HACCEL m_OldAccelTable = (HACCEL)m_AccelTable;
    m_AccelTable = m_ShortCuts.GetAcceleratorTable();
    if (m_OldAccelTable)
    {
        DestroyAcceleratorTable(m_OldAccelTable);
    }
    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}

void CMainMenu::ResetMenu(void)
{
    WriteTrace(TraceUserInterface, TraceDebug, "Start");
    if (!UISettingsLoadBool(UserInterface_InFullScreen))
    {
        // Create a new window with all the items
        WriteTrace(TraceUserInterface, TraceDebug, "Create menu");
        HMENU hMenu = CreateMenu();
        FillOutMenu(hMenu);
        WriteTrace(TraceUserInterface, TraceDebug, "Creating menu done");

        // Save old menu to destroy latter
        HMENU OldMenuHandle;
        {
            CGuard Guard(m_CS);
            OldMenuHandle = m_MenuHandle;

            // Save handle and re-attach to a window
            WriteTrace(TraceUserInterface, TraceDebug, "Attach menu");
            m_MenuHandle = hMenu;
        }
        m_Gui->SetWindowMenu(this);

        WriteTrace(TraceUserInterface, TraceDebug, "Remove plugin menu");
        if (g_Plugins->Gfx() != nullptr && IsMenu((HMENU)g_Plugins->Gfx()->GetDebugMenu()))
        {
            RemoveMenu((HMENU)OldMenuHandle, (DWORD)g_Plugins->Gfx()->GetDebugMenu(), MF_BYCOMMAND);
        }
        if (g_Plugins->RSP() != nullptr && IsMenu((HMENU)g_Plugins->RSP()->GetDebugMenu()))
        {
            RemoveMenu((HMENU)OldMenuHandle, (DWORD)g_Plugins->RSP()->GetDebugMenu(), MF_BYCOMMAND);
        }
        WriteTrace(TraceUserInterface, TraceDebug, "Destroy old menu");

#ifdef RETROACHIEVEMENTS
        RA_RebuildMenu();
#endif

        // Destroy the old menu
        DestroyMenu((HMENU)OldMenuHandle);
    }

    ResetAccelerators();

    WriteTrace(TraceUserInterface, TraceDebug, "Done");
}
