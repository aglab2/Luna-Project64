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
                    profile.Config.Graphics = toGraphicsMode(value);
                }
                if (key == "memory")
                {
                    profile.Config.Memory = toMemoryMode(value);
                }
                if (key == "graphics_reduce_input_delay")
                {
                    profile.Config.ReduceInputDelay = value == "true";
                }
                if (key == "graphics_remove_black_bars")
                {
                    profile.Config.RemoveBlackBars = value == "true";
                }
                if (key == "graphics_zelda_hack")
                {
                    profile.Config.EnableZeldaHacks = value == "true";
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

void ProfileManager::activate(ProfileSelection selection)
{

}
