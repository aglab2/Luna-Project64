#pragma once

#include <string>
#include <vector>

struct ProfileSelection
{
    enum class CpuMode
    {
        Basic = 0,
        HighAccuracy,
        HighAccuracyNoRecompiler,
        Interpreter,
        Custom,
    };

    enum class GraphicsMode
    {
        Basic = 0,
        Framebuffer,
        FramebufferDepth,
        LLE,
        Custom,
    };

    enum class MemoryMode
    {
        Basic = 0,
        Unchecked,
        Protected,
        Custom,
    };

    struct GraphicsCfg
    {
        GraphicsMode mode;
        bool ReduceInputDelay;
        bool RemoveBlackBars;
        bool EnableZeldaHacks;

        bool operator==(const GraphicsCfg& other) const;
        bool operator!=(const GraphicsCfg& other) const;
    };

    CpuMode Cpu;
    MemoryMode Memory;
    GraphicsCfg Graphics;
};

struct ProfileDefinition
{
    std::string Name;
    ProfileSelection Config;
};

class ProfileManager
{
public:
    ProfileManager(const char* cfgPath);

	const auto& profiles() const { return m_Profiles; }
	const auto& profileNames() const { return m_ProfileNames; }
    void activate(ProfileSelection);

    ProfileSelection curSelection() const;
	int curProfileIndex() const;
    int profileIndex(const ProfileSelection& selection) const;

private:

    std::vector<ProfileDefinition> m_Profiles;
	std::vector<std::wstring> m_ProfileNames;
};
