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

	auto selection = m_ProfileManager.curSelection();
	auto selectedProfileIndex = m_ProfileManager.profileIndex(selection);
    m_ProfilePreset.SetCurSel(selectedProfileIndex == -1 ? (int)profiles.size() : selectedProfileIndex);

    ApplySelectionToUi(selection);
    UpdateUiState();

    return TRUE;
}

LRESULT CProfilesConfigUI::OnProfilePresetChanged(WORD, WORD, HWND, BOOL &)
{
    int selectedProfileIndex = GetSelectedProfileIndex();
    if (selectedProfileIndex != -1)
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

LRESULT CProfilesConfigUI::OnApply(WORD, WORD, HWND, BOOL &)
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
        return -1;
    }
    return selectedIndex;
}

const ProfileDefinition & CProfilesConfigUI::GetSelectedProfile() const
{
    int selectedIndex = GetSelectedProfileIndex();
    if (selectedIndex == -1)
    {
        selectedIndex = 0;
    }
    return m_ProfileManager.profiles()[selectedIndex];
}

void CProfilesConfigUI::SyncPresetSelectionFromUi(const ProfileSelection & selection)
{
    int matchedIndex = m_ProfileManager.profileIndex(selection);
	const auto& profiles = m_ProfileManager.profiles();
    int desiredIndex = matchedIndex == -1 ? (int)profiles.size() : matchedIndex;
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
        selection.Graphics.mode = ProfileSelection::GraphicsMode::Custom;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_FRAMEBUFFER) == BST_CHECKED)
    {
        selection.Graphics.mode = ProfileSelection::GraphicsMode::Framebuffer;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_FRAMEBUFFER_DEPTH) == BST_CHECKED)
    {
        selection.Graphics.mode = ProfileSelection::GraphicsMode::FramebufferDepth;
    }
    else if (IsDlgButtonChecked(IDC_PROFILE_GFX_LLE) == BST_CHECKED)
    {
        selection.Graphics.mode = ProfileSelection::GraphicsMode::LLE;
    }
    else
    {
        selection.Graphics.mode = ProfileSelection::GraphicsMode::Basic;
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

    selection.Graphics.ReduceInputDelay = IsDlgButtonChecked(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY) == BST_CHECKED;
    selection.Graphics.RemoveBlackBars = IsDlgButtonChecked(IDC_PROFILE_GFX_REMOVE_BLACK_BARS) == BST_CHECKED;
    selection.Graphics.EnableZeldaHacks = IsDlgButtonChecked(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS) == BST_CHECKED;

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
    switch (selection.Graphics.mode)
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

    CheckDlgButton(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY, selection.Graphics.ReduceInputDelay ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PROFILE_GFX_REMOVE_BLACK_BARS, selection.Graphics.RemoveBlackBars ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS, selection.Graphics.EnableZeldaHacks ? BST_CHECKED : BST_UNCHECKED);
}

void CProfilesConfigUI::UpdateUiState()
{
    const ProfileSelection current = GetUiSelection();

    UpdateGraphicsCheckboxState(current);
}

void CProfilesConfigUI::UpdateGraphicsCheckboxState(const ProfileSelection & selection)
{
    bool wantFBTweaks = ProfileSelection::GraphicsMode::Basic != selection.Graphics.mode;
    bool wantGLideN64Tweaks = selection.Graphics.mode == ProfileSelection::GraphicsMode::Framebuffer || selection.Graphics.mode == ProfileSelection::GraphicsMode::FramebufferDepth;

    GetDlgItem(IDC_PROFILE_GFX_REDUCE_INPUT_DELAY).EnableWindow(wantFBTweaks ? TRUE : FALSE);
    GetDlgItem(IDC_PROFILE_GFX_REMOVE_BLACK_BARS).EnableWindow(wantFBTweaks ? TRUE : FALSE);
    GetDlgItem(IDC_PROFILE_GFX_ENABLE_ZELDA_HACKS).EnableWindow(wantGLideN64Tweaks ? TRUE : FALSE);
}
