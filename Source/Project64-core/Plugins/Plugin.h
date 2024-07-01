#pragma once
#include <list>
#include <Project64-core/Settings.h>
#include <Project64-core/Settings/DebugSettings.h>

#if defined(_WIN32)
#define CALL        __cdecl
#else
#define CALL
#endif

#ifndef PLUGIN_INFO_STRUCT
#define PLUGIN_INFO_STRUCT

typedef struct
{
    uint16_t Version;        // Should be set to 1
    uint16_t Type;           // Set to PLUGIN_TYPE_GFX
    char Name[100];      // Name of the DLL

    // If DLL supports these memory options then set them to TRUE or FALSE if it does not support it
    int32_t NormalMemory;   // A normal BYTE array
    int32_t MemoryBswaped;  // A normal BYTE array where the memory has been pre-bswap'd on a DWORD (32-bit) boundary
} PLUGIN_INFO;

#endif

// Enums
enum SETTING_DATA_TYPE
{
    Data_DWORD_General = 0, // A uint32_t setting used anywhere
    Data_String_General = 1, // A string setting used anywhere
    Data_DWORD_Game = 2, // A uint32_t associated with the current game
    Data_String_Game = 3, // A string associated with the current game
    Data_DWORD_RDB = 4, // A uint32_t associated with the current game in the ROM database
    Data_String_RDB = 5, // A string associated with the current game in the ROM database
    Data_DWORD_RDB_Setting = 6, // A uint32_t read from the ROM database, with config file
    Data_String_RDB_Setting = 7, // A string read from the ROM database, with config file
};

typedef struct
{
    uint32_t  dwSize;
    int32_t    DefaultStartRange;
    int32_t    SettingStartRange;
    int32_t    MaximumSettings;
    int32_t    NoDefault;
    int32_t    DefaultLocation;
    void * handle;
    uint32_t(CALL *GetSetting)      (void * handle, int32_t ID);
    const char * (CALL *GetSettingSz)    (void * handle, int32_t ID, char * Buffer, int32_t BufferLen);
    void(CALL *SetSetting)      (void * handle, int32_t ID, uint32_t Value);
    void(CALL *SetSettingSz)    (void * handle, int32_t ID, const char * Value);
    void(CALL *RegisterSetting) (void * handle, int32_t ID, int32_t DefaultID, SettingDataType Type,
        SettingType Location, const char * Category, const char * DefaultStr, uint32_t Value);
    void(CALL *UseUnregisteredSetting) (int32_t ID);
} PLUGIN_SETTINGS;

typedef struct
{
    uint32_t(CALL *FindSystemSettingId) (void * handle, const char * Name);
} PLUGIN_SETTINGS2;

typedef struct
{
    void(CALL *FlushSettings) (void * handle);
} PLUGIN_SETTINGS3;

typedef struct
{
    typedef void(CALL *SettingChangedFunc)(void *);

    void(CALL *RegisterChangeCB)(void * handle, int ID, void * Data, SettingChangedFunc Func);
    void(CALL *UnregisterChangeCB)(void * handle, int ID, void * Data, SettingChangedFunc Func);
} PLUGIN_SETTINGS_NOTIFICATION;

typedef struct
{
    void(CALL *DisplayError)(const char * Message);
    void(CALL *FatalError)(const char * Message);
    void(CALL *DisplayMessage)(int DisplayTime, const char * Message);
    void(CALL *DisplayMessage2)(const char * Message);
    void(CALL *BreakPoint)(const char * FileName, int32_t LineNumber);
} PLUGIN_NOTIFICATION;

enum PLUGIN_TYPE
{
    PLUGIN_TYPE_NONE = 0,
    PLUGIN_TYPE_RSP = 1,
    PLUGIN_TYPE_GFX = 2,
    PLUGIN_TYPE_AUDIO = 3,
    PLUGIN_TYPE_CONTROLLER = 4,
};

class CSettings;
class CGfxPlugin; class CAudioPlugin; class CRSP_Plugin; class CControl_Plugin;
class CN64System;
class CPlugins;

#if defined(_WIN32)
#include <objbase.h>
#else
#define __interface     struct
#endif

__interface RenderWindow
{
#ifdef _WIN32
    virtual bool ResetPluginsInUiThread(CPlugins * plugins, CN64System * System) = 0;
    virtual void * GetWindowHandle(void) const = 0;
    virtual void * GetStatusBar(void) const = 0;
    virtual void * GetModuleInstance(void) const = 0;
#else
    virtual void GfxThreadInit() = 0;
    virtual void GfxThreadDone() = 0;
    virtual void SwapWindow() = 0;
#endif
};

class COREAPI CPlugins :
    private CDebugSettings
{
public:
    // Functions
    CPlugins(SettingID PluginDirSetting, bool SyncPlugins);
    ~CPlugins();

    bool Initiate(CN64System * System);
    void RomOpened(void);
    void RomClosed(void);
    void SetRenderWindows(RenderWindow * MainWindow, RenderWindow * SyncWindow);
    void ConfigPlugin(void * hParent, PLUGIN_TYPE Type);
    bool CopyPlugins(const stdstr & DstDir) const;
    void CreatePlugins(void);
    bool Reset(CN64System * System);
    bool ResetInUiThread(CN64System * System);
    void GameReset(void);

    inline CGfxPlugin * Gfx(void) const { return m_Gfx; }
    inline CAudioPlugin * Audio(void) const { return m_Audio; }
    inline CRSP_Plugin * RSP(void) const { return m_RSP; }
    inline CControl_Plugin * Control(void) const { return m_Control; }

    inline RenderWindow * MainWindow(void) const { return m_MainWindow; }
    inline RenderWindow * SyncWindow(void) const { return m_SyncWindow; }

    inline bool initilized(void) const { return m_initilized; }

private:
    CPlugins(void);
    CPlugins(const CPlugins&);
    CPlugins& operator=(const CPlugins&);

    void DestroyGfxPlugin(void);
    void DestroyAudioPlugin(void);
    void DestroyRspPlugin(void);
    void DestroyControlPlugin(void);

    static void PluginChanged(CPlugins * _this);

    RenderWindow * m_MainWindow;
    RenderWindow * m_SyncWindow;

    SettingID m_PluginDirSetting;
    stdstr m_PluginDir;

    // Plugins
    CGfxPlugin      * m_Gfx;
    CAudioPlugin    * m_Audio;
    CRSP_Plugin     * m_RSP;
    CControl_Plugin * m_Control;

    stdstr m_GfxFile;
    stdstr m_AudioFile;
    stdstr m_RSPFile;
    stdstr m_ControlFile;
    bool m_initilized;
    bool m_SyncPlugins;
};

// Dummy Functions
void CALL DummyCheckInterrupts(void);
void CALL DummyFunction(void);
