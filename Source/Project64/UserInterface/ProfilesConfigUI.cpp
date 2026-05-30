#include <stdafx.h>

#include "ProfilesConfigUI.h"

static const wchar_t* g_CustomProfileName = L"Custom";

CProfilesConfigUI::CProfilesConfigUI(ProfileManager& profileManager)
    : m_ProfileManager(profileManager)
{
}

void CProfilesConfigUI::Display(void * ParentWindow)
{
    DoModal((HWND)ParentWindow);
}

LRESULT CProfilesConfigUI::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &)
{
    SetWindowText(L"Profiles");

    m_ProfilePreset.Attach(GetDlgItem(IDC_PROFILE_PRESET_COMBO));

	const auto& profiles = m_ProfileManager.profiles();
	const auto& names = m_ProfileManager.profileNames();
    for (size_t i = 0; i < profiles.size(); i++)
    {
        m_ProfilePreset.AddString(names[i].c_str());
    }
    m_ProfilePreset.AddString(g_CustomProfileName);
    m_ProfilePreset.SetCurSel(0);

    ApplySelectionToUi(GetSelectedProfile().Config);
    UpdateUiState();

    return TRUE;
}

LRESULT CProfilesConfigUI::OnProfilePresetChanged(WORD, WORD, HWND, BOOL &)
{
    int selectedProfileIndex = GetSelectedProfileIndex();
    if (selectedProfileIndex != CustomProfileIndex)
    {
        ApplySelectionToUi(GetSelectedProfile().Config);
    }
    UpdateUiState();
    return 0;
}

LRESULT CProfilesConfigUI::OnSelectionChanged(WORD, WORD, HWND, BOOL &)
{
    SyncPresetSelectionFromUi(GetUiSelection());
    UpdateUiState();
    return 0;
}

LRESULT CProfilesConfigUI::OnApplyStub(WORD, WORD, HWND, BOOL &)
{
    const int selectedProfileIndex = GetSelectedProfileIndex();
	m_ProfileManager.activate(GetUiSelection());
    EndDialog(IDOK);
    return 0;
}

LRESULT CProfilesConfigUI::OnCloseDialog(WORD, WORD, HWND, BOOL &)
{
    EndDialog(IDCANCEL);
    return 0;
}

int CProfilesConfigUI::GetSelectedProfileIndex() const
{
    int selectedIndex = m_ProfilePreset.GetCurSel();
    if (selectedIndex < 0)
    {
        return 0;
    }
    const auto& profiles = m_ProfileManager.profiles();
    if (selectedIndex >= (int)profiles.size())
    {
        return CustomProfileIndex;
    }
    return selectedIndex;
}

const ProfileDefinition & CProfilesConfigUI::GetSelectedProfile() const
{
    int selectedIndex = GetSelectedProfileIndex();
    if (selectedIndex == CustomProfileIndex)
    {
        selectedIndex = 0;
    }
    return m_ProfileManager.profiles()[selectedIndex];
}

int CProfilesConfigUI::FindMatchingProfileIndex(const ProfileSelection & selection) const
{
	const auto& profiles = m_ProfileManager.profiles();
    for (size_t i = 0; i < profiles.size(); i++)
    {
        const ProfileSelection & recommended = profiles[i].Config;
        if (selection.Cpu != recommended.Cpu ||
            selection.Graphics != recommended.Graphics ||
            selection.Memory != recommended.Memory ||
            selection.ReduceInputDelay != recommended.ReduceInputDelay ||
            selection.RemoveBlackBars != recommended.RemoveBlackBars ||
            selection.EnableZeldaHacks != recommended.EnableZeldaHacks)
        {
            continue;
        }
        return (int)i;
    }
    return CustomProfileIndex;
}

void CProfilesConfigUI::SyncPresetSelectionFromUi(const ProfileSelection & selection)
{
    int matchedIndex = FindMatchingProfileIndex(selection);
	const auto& profiles = m_ProfileManager.profiles();
    int desiredIndex = matchedIndex == CustomProfileIndex ? (int)profiles.size() : matchedIndex;
    if (m_ProfilePreset.GetCurSel() != desiredIndex)
    {
        m_ProfilePreset.SetCurSel(desiredIndex);
    }
}

ProfileSelection CProfilesConfigUI::GetUiSelection() const
{
    ProfileSelection selection = {};

    if (IsDlgButtonChecked(IDC_PROFILE_CPU_CUSTOM) == BST_CHECKED)
    {
        selection.Cpu = ProfileSelection::CpuMode::Custom;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_CPU_HIGH_ACCURACY) == BST_CHECKED)
    {
        selection.Cpu = ProfileSelection::CpuMode::HighAccuracy;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_CPU_HIGH_NO_RECOMPILER) == BST_CHECKED)
    {
        selection.Cpu = ProfileSelection::CpuMode::HighAccuracyNoRecompiler;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_CPU_INTERPRETER) == BST_CHECKED)
    {
        selection.Cpu = ProfileSelection::CpuMode::Interpreter;
    }
    else
    {
        selection.Cpu = ProfileSelection::CpuMode::Basic;
    }

    if (IsDlgButtonChecked(IDC_PROFILE_GFX_CUSTOM) == BST_CHECKED)
    {
        selection.Graphics = ProfileSelection::GraphicsMode::Custom;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_FRAMEBUFFER) == BST_CHECKED)
    {
        selection.Graphics = ProfileSelection::GraphicsMode::Framebuffer;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_FRAMEBUFFER_DEPTH) == BST_CHECKED)
    {
        selection.Graphics = ProfileSelection::GraphicsMode::FramebufferDepth;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_LLE) == BST_CHECKED)
    {
        selection.Graphics = ProfileSelection::GraphicsMode::LLE;
    }
    else
    {
        selection.Graphics = ProfileSelection::GraphicsMode::Basic;
    }

    if (IsDlgButtonChecked(IDC_PROFILE_MEMORY_CUSTOM) == BST_CHECKED)
    {
        selection.Memory = ProfileSelection::MemoryMode::Custom;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_MEMORY_UNCHECKED) == BST_CHECKED)
    {
        selection.Memory = ProfileSelection::MemoryMode::Unchecked;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_MEMORY_PROTECTED) == BST_CHECKED)
    {
        selection.Memory = ProfileSelection::MemoryMode::Protected;
    }
    else
    {
        selection.Memory = ProfileSelection::MemoryMode::Basic;
    }

    selection.ReduceInputDelay = IsDlgButtonChecked(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY) == BST_CHECKED;
    selection.RemoveBlackBars = IsDlgButtonChecked(IDC_PROFILE_GFX_REMOVE_BLACK_BARS) == BST_CHECKED;
    selection.EnableZeldaHacks = IsDlgButtonChecked(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS) == BST_CHECKED;

    return selection;
}

void CProfilesConfigUI::ApplySelectionToUi(const ProfileSelection & selection)
{
    int cpuRadioId = IDC_PROFILE_CPU_BASIC;
    switch (selection.Cpu)
    {
    case ProfileSelection::CpuMode::HighAccuracy: cpuRadioId = IDC_PROFILE_CPU_HIGH_ACCURACY; break;
    case ProfileSelection::CpuMode::HighAccuracyNoRecompiler: cpuRadioId = IDC_PROFILE_CPU_HIGH_NO_RECOMPILER; break;
    case ProfileSelection::CpuMode::Interpreter: cpuRadioId = IDC_PROFILE_CPU_INTERPRETER; break;
    case ProfileSelection::CpuMode::Custom: cpuRadioId = IDC_PROFILE_CPU_CUSTOM; break;
    case ProfileSelection::CpuMode::Basic:
    default: cpuRadioId = IDC_PROFILE_CPU_BASIC; break;
    }
    CheckRadioButton(IDC_PROFILE_CPU_BASIC, IDC_PROFILE_CPU_CUSTOM, cpuRadioId);

    int gfxRadioId = IDC_PROFILE_GFX_BASIC;
    switch (selection.Graphics)
    {
    case ProfileSelection::GraphicsMode::Framebuffer: gfxRadioId = IDC_PROFILE_GFX_FRAMEBUFFER; break;
    case ProfileSelection::GraphicsMode::FramebufferDepth: gfxRadioId = IDC_PROFILE_GFX_FRAMEBUFFER_DEPTH; break;
    case ProfileSelection::GraphicsMode::LLE: gfxRadioId = IDC_PROFILE_GFX_LLE; break;
    case ProfileSelection::GraphicsMode::Custom: gfxRadioId = IDC_PROFILE_GFX_CUSTOM; break;
    case ProfileSelection::GraphicsMode::Basic:
    default: gfxRadioId = IDC_PROFILE_GFX_BASIC; break;
    }
    CheckRadioButton(IDC_PROFILE_GFX_BASIC, IDC_PROFILE_GFX_CUSTOM, gfxRadioId);

    int memoryRadioId = IDC_PROFILE_MEMORY_BASIC;
    switch (selection.Memory)
    {
    case ProfileSelection::MemoryMode::Unchecked: memoryRadioId = IDC_PROFILE_MEMORY_UNCHECKED; break;
    case ProfileSelection::MemoryMode::Protected: memoryRadioId = IDC_PROFILE_MEMORY_PROTECTED; break;
    case ProfileSelection::MemoryMode::Custom: memoryRadioId = IDC_PROFILE_MEMORY_CUSTOM; break;
    case ProfileSelection::MemoryMode::Basic:
    default: memoryRadioId = IDC_PROFILE_MEMORY_BASIC; break;
    }
    CheckRadioButton(IDC_PROFILE_MEMORY_BASIC, IDC_PROFILE_MEMORY_CUSTOM, memoryRadioId);

    CheckDlgButton(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY, selection.ReduceInputDelay ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PROFILE_GFX_REMOVE_BLACK_BARS, selection.RemoveBlackBars ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS, selection.EnableZeldaHacks ? BST_CHECKED : BST_UNCHECKED);
}

void CProfilesConfigUI::UpdateUiState()
{
    const ProfileSelection current = GetUiSelection();

    UpdateGraphicsCheckboxState(current);
}

void CProfilesConfigUI::UpdateGraphicsCheckboxState(const ProfileSelection & selection)
{
    const bool enable = IsGraphicsHleFb(selection.Graphics);

    GetDlgItem(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY).EnableWindow(enable ? TRUE : FALSE);
    GetDlgItem(IDC_PROFILE_GFX_REMOVE_BLACK_BARS).EnableWindow(enable ? TRUE : FALSE);
    GetDlgItem(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS).EnableWindow(enable ? TRUE : FALSE);
}

bool CProfilesConfigUI::IsGraphicsHleFb(ProfileSelection::GraphicsMode mode) const
{
    return mode == ProfileSelection::GraphicsMode::Framebuffer || mode == ProfileSelection::GraphicsMode::FramebufferDepth;
}
