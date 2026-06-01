#include "stdafx.h"
#include "ProfileManager.h"

#include "Common/StrKit.h"

static ProfileSelection::CpuMode toCpuMode(const std::string& name)
{
    if (name == "accurate")
    {
        return ProfileSelection::CpuMode::HighAccuracy;
    }

    return ProfileSelection::CpuMode::Basic;
}

static ProfileSelection::GraphicsMode toGraphicsMode(const std::string& name)
{
    if (name == "framebuffer")
    {
        return ProfileSelection::GraphicsMode::Framebuffer;
    }
    if (name == "framebuffer_depth")
    {
        return ProfileSelection::GraphicsMode::FramebufferDepth;
    }
    if (name == "lle")
    {
        return ProfileSelection::GraphicsMode::LLE;
    }

    return ProfileSelection::GraphicsMode::Basic;
}

static ProfileSelection::MemoryMode toMemoryMode(const std::string& name)
{
    if (name == "unchecked")
    {
        return ProfileSelection::MemoryMode::Unchecked;
    }
    if (name == "protected")
    {
        return ProfileSelection::MemoryMode::Protected;
    }

    return ProfileSelection::MemoryMode::Basic;
}

ProfileManager::ProfileManager(const char* cfgPath)
{
    try
    {
        CIniFile m_ProfilesIni(cfgPath);
        auto names = m_ProfilesIni.GetVectorOfSectionsSorted();

        for (const auto& name : names)
        {
            CIniFileBase::KeyValueData kvs;
            m_ProfilesIni.GetKeyValueData(name.c_str(), kvs);

            ProfileDefinition profile{};
            for (const auto& [key, value] : kvs)
            {
                if (key == "cpu")
                {
                    profile.Config.Cpu = toCpuMode(value);
                }
                if (key == "graphics")
                {
                    profile.Config.Graphics.mode = toGraphicsMode(value);
                }
                if (key == "memory")
                {
                    profile.Config.Memory = toMemoryMode(value);
                }
                if (key == "graphics_reduce_input_delay")
                {
                    profile.Config.Graphics.ReduceInputDelay = value == "true";
                }
                if (key == "graphics_remove_black_bars")
                {
                    profile.Config.Graphics.RemoveBlackBars = value == "true";
                }
                if (key == "graphics_zelda_hack")
                {
                    profile.Config.Graphics.EnableZeldaHacks = value == "true";
                }
                if (key == "name")
                {
                    profile.Name = value;
                }
            }

            m_Profiles.push_back(profile);
        }
    }
    catch (...)
    {
		m_Profiles.clear();
		return;
    }

    if (m_Profiles.empty())
    {
        m_Profiles.push_back(ProfileDefinition { "Default" });
    }

    for (const auto& profile : m_Profiles)
    {
		m_ProfileNames.push_back(ToWide(profile.Name));
    }
}

struct CpuConfig
{
    bool ForceInterpreterCPU;
    bool DebuggerEnabled;
    bool Game32Bit;
    bool SlowRecompiler;
};

struct PinnedConfig
{
	SettingID Id;
    bool isBool;
	uint32_t value;
};

static const PinnedConfig sCpuPinnedSettings[] = {
    { Default_UseTlb                 , true, true },
    { Default_AiCountPerBytes        , false, 0 },
    { Default_RandomizeSIPIInterrupts, true, true },
    { Default_UnalignedDMA           , true, true },
	{ Default_DiskSeekTiming         , false, DiskSeek_Turbo },
    { Default_SyncViaAudio           , true, true },
    { Default_FixedAudio             , true, true },
};

static void applyCpuPinnedSettings()
{
    for (const auto& setting : sCpuPinnedSettings)
    {
        if (setting.isBool)
        {
            g_Settings->SaveBool(setting.Id, setting.value);
        }
        else
        {
            g_Settings->SaveDword(setting.Id, setting.value);
        }
    }
}

static bool isCpuPinnedSettingsCorrect()
{
    for (const auto& setting : sCpuPinnedSettings)
    {
        if (setting.isBool)
        {
            if (g_Settings->LoadBool(setting.Id) != setting.value)
            {
                return false;
            }
        }
        else
        {
            if (g_Settings->LoadDword(setting.Id) != setting.value)
            {
                return false;
            }
        }
    }
    return true;
}

static void apply(const CpuConfig& config)
{
    g_Settings->SaveBool(Setting_ForceInterpreterCPU, config.ForceInterpreterCPU);
    g_Settings->SaveBool(Debugger_Enabled, config.DebuggerEnabled);
    g_Settings->SaveBool(Default_32Bit, config.Game32Bit);
    g_Settings->SaveBool(Default_RegCache, !config.SlowRecompiler);
    g_Settings->SaveBool(Default_BlockLinking, !config.SlowRecompiler);
    g_Settings->SaveBool(Default_FastSP, !config.SlowRecompiler);

	applyCpuPinnedSettings();
    // g_Settings->SaveDword(Game_LoadRomToMemory, false); // double check
    // Game_ViRefreshRate 1500?
    // Game_OverClockModifier 1?
}

static ProfileSelection::CpuMode curCpuMode()
{
    if (!isCpuPinnedSettingsCorrect())
    {
        return ProfileSelection::CpuMode::Custom;
	}

	bool regCache = g_Settings->LoadBool(Default_RegCache);
	bool blockLinking = g_Settings->LoadBool(Default_BlockLinking);
	bool fastSP = g_Settings->LoadBool(Default_FastSP);

    bool slowRecompiler;
    if (regCache && blockLinking && fastSP)
    {
        slowRecompiler = false;
    }
    else if (!regCache && !blockLinking && !fastSP)
    {
        slowRecompiler = true;
	}
    else
    {
		return ProfileSelection::CpuMode::Custom;
    }

    bool forceInterpreterCPU = g_Settings->LoadBool(Setting_ForceInterpreterCPU);
    bool debuggerEnabled = g_Settings->LoadBool(Debugger_Enabled);
    bool game32Bit = g_Settings->LoadBool(Default_32Bit);
    if (!forceInterpreterCPU && !debuggerEnabled && game32Bit && !slowRecompiler)
    {
        return ProfileSelection::CpuMode::Basic;
    }
    if (!forceInterpreterCPU && !debuggerEnabled && !game32Bit && !slowRecompiler)
    {
        return ProfileSelection::CpuMode::HighAccuracy;
    }
    if (!forceInterpreterCPU && !debuggerEnabled && !game32Bit && slowRecompiler)
    {
        return ProfileSelection::CpuMode::HighAccuracyNoRecompiler;
    }
    if (forceInterpreterCPU && !debuggerEnabled)
    {
        return ProfileSelection::CpuMode::Interpreter;
    }

	return ProfileSelection::CpuMode::Custom;
}

static void apply(ProfileSelection::CpuMode mode)
{
    CpuConfig cfg{};
    switch (mode)
    {
        case ProfileSelection::CpuMode::Basic:
            cfg.Game32Bit = true;
            break;
        case ProfileSelection::CpuMode::HighAccuracy:
            break;
        case ProfileSelection::CpuMode::HighAccuracyNoRecompiler:
			cfg.SlowRecompiler = true;
            break;
        case ProfileSelection::CpuMode::Interpreter:
            cfg.ForceInterpreterCPU = true;
            break;
        case ProfileSelection::CpuMode::Custom:
            return;
    }

    apply(cfg);
}

struct MemoryConfig
{
    bool Cache;
    bool Protect;
	bool ValidFunc;
    bool PIDMA;
	bool TLB;
};

static void apply(const MemoryConfig& config)
{
    g_Settings->SaveBool(Default_SMM_Cache, config.Cache);
    g_Settings->SaveBool(Default_SMM_PIDMA, config.PIDMA);
    g_Settings->SaveBool(Default_SMM_ValidFunc, config.ValidFunc);
    g_Settings->SaveBool(Default_SMM_TLB, config.TLB);
    g_Settings->SaveBool(Default_SMM_Protect_Memory, config.Protect);
}

static void apply(ProfileSelection::MemoryMode mode)
{
    MemoryConfig cfg;
	cfg.Cache = true;
	cfg.Protect = false;
	cfg.ValidFunc = true;
	cfg.PIDMA = true;
	cfg.TLB = true;

    switch (mode)
    {
        case ProfileSelection::MemoryMode::Basic:
            break;
        case ProfileSelection::MemoryMode::Unchecked:
            cfg = { 0 };
            break;
        case ProfileSelection::MemoryMode::Protected:
            cfg = { 0 };
			cfg.Protect = true;
            break;
        case ProfileSelection::MemoryMode::Custom:
            return;
    }

    apply(cfg);
}

static ProfileSelection::MemoryMode curMemoryMode()
{
    bool cache = g_Settings->LoadBool(Default_SMM_Cache);
    bool protect = g_Settings->LoadBool(Default_SMM_Protect_Memory);
    bool validFunc = g_Settings->LoadBool(Default_SMM_ValidFunc);
    bool pidma = g_Settings->LoadBool(Default_SMM_PIDMA);
    bool tlb = g_Settings->LoadBool(Default_SMM_TLB);
    if (cache && !protect && validFunc && pidma && tlb)
    {
        return ProfileSelection::MemoryMode::Basic;
    }
    if (!cache && !protect && !validFunc && !pidma && !tlb)
    {
        return ProfileSelection::MemoryMode::Unchecked;
    }
    if (!cache && protect && !validFunc && !pidma && !tlb)
    {
        return ProfileSelection::MemoryMode::Protected;
    }
    return ProfileSelection::MemoryMode::Custom;
}

struct GLideN64Config
{
    bool Fb;
    bool FbDepthCompare;
    bool FbDefault;
    bool EmuDefault;

    bool ReduceInputDelay;
    bool RemoveBlackBars;
    bool EnableZeldaHacks;
};

static ProfileSelection::GraphicsCfg curGraphicsMode()
{
    ProfileSelection::GraphicsCfg cfg;

    bool useHleGfx = g_Settings->LoadBool(Plugin_UseHleGfx);
    std::string gfxPlugin = g_Settings->LoadStringVal(Plugin_GFX_Current);
    if (useHleGfx)
    {
        if (gfxPlugin == "GFX\\GLideN64.dll")
        {
			GLideN64Config pluginCfg{};
			g_Plugins->Gfx()->LunaLoadConfig(nullptr, &pluginCfg);

            if (!pluginCfg.EmuDefault || !pluginCfg.FbDefault)
            {
				cfg.mode = ProfileSelection::GraphicsMode::Custom;
                return cfg;
			}

            if (!pluginCfg.Fb)
            {
				cfg.mode = ProfileSelection::GraphicsMode::Basic;
                return cfg;
			}

			cfg.EnableZeldaHacks = pluginCfg.EnableZeldaHacks;
			cfg.ReduceInputDelay = pluginCfg.ReduceInputDelay;
			cfg.RemoveBlackBars = pluginCfg.RemoveBlackBars;

            if (pluginCfg.FbDepthCompare)
            {
				cfg.mode = ProfileSelection::GraphicsMode::FramebufferDepth;
            }
            else
            {
				cfg.mode = ProfileSelection::GraphicsMode::Framebuffer;
            }
            return cfg;
        }
    }
    else
    {
        if (gfxPlugin == "GFX\\pj64-parallel-rdp.dll")
        {
            cfg.mode = ProfileSelection::GraphicsMode::LLE;
            return cfg;
        }
    }

    cfg.mode = ProfileSelection::GraphicsMode::Custom;
    return cfg;
}

static void apply(ProfileSelection::GraphicsCfg cfg)
{
    auto mode = cfg.mode;
    switch (mode)
    {
    case ProfileSelection::GraphicsMode::Basic:
    case ProfileSelection::GraphicsMode::Framebuffer:
    case ProfileSelection::GraphicsMode::FramebufferDepth:
        g_Settings->SaveBool(Plugin_UseHleGfx, true);
        g_Settings->SaveString(Plugin_GFX_Current, "GFX\\GLideN64.dll");
        break;
    case ProfileSelection::GraphicsMode::LLE:
        g_Settings->SaveBool(Plugin_UseHleGfx, false);
        g_Settings->SaveString(Plugin_GFX_Current, "GFX\\pj64-parallel-rdp.dll");
        break;
    case ProfileSelection::GraphicsMode::Custom:
        return;
    }

    g_Plugins->PluginChanged(g_Plugins);

    GLideN64Config pluginCfg{};

    bool wantFbSettings = false;
    if (ProfileSelection::GraphicsMode::Framebuffer == mode)
    {
        pluginCfg.Fb = true;
        pluginCfg.FbDepthCompare = false;
    }
    else if (ProfileSelection::GraphicsMode::FramebufferDepth == mode)
    {
        pluginCfg.Fb = true;
        pluginCfg.FbDepthCompare = true;
    }

    pluginCfg.EmuDefault = true;
    pluginCfg.FbDefault = true;

    if (pluginCfg.Fb)
    {
        pluginCfg.ReduceInputDelay = cfg.ReduceInputDelay;
        pluginCfg.RemoveBlackBars = cfg.RemoveBlackBars;
        pluginCfg.EnableZeldaHacks = cfg.EnableZeldaHacks;
    }

    g_Plugins->Gfx()->LunaSaveConfig(nullptr, &pluginCfg);
}

void ProfileManager::activate(ProfileSelection selection)
{
    apply(selection.Cpu);
	apply(selection.Memory);
    apply(selection.Graphics);
}

ProfileSelection ProfileManager::curSelection() const
{
    ProfileSelection selection;
	selection.Cpu = curCpuMode();
	selection.Graphics = curGraphicsMode();
    selection.Memory = curMemoryMode();
	return selection;
}

int ProfileManager::curProfileIndex() const
{
	return profileIndex(curSelection());
}

int ProfileManager::profileIndex(const ProfileSelection& selection) const
{
    for (size_t i = 0; i < m_Profiles.size(); i++)
    {
        const ProfileSelection& recommended = m_Profiles[i].Config;
        if (selection.Cpu != recommended.Cpu ||
            selection.Memory != recommended.Memory ||
            selection.Graphics != recommended.Graphics)
        {
            continue;
        }
        return (int)i;
    }

    return -1;
}

bool ProfileSelection::GraphicsCfg::operator==(const GraphicsCfg& other) const
{
    if (mode != other.mode)
    {
        return false;
	}

    if (mode == ProfileSelection::GraphicsMode::Basic || mode == ProfileSelection::GraphicsMode::LLE)
    {
        return true;
	}

    return ReduceInputDelay == other.ReduceInputDelay
        && RemoveBlackBars == other.RemoveBlackBars
        && EnableZeldaHacks == other.EnableZeldaHacks;
}

bool ProfileSelection::GraphicsCfg::operator!=(const GraphicsCfg& other) const
{
	return !(*this == other);
}
