#pragma once

// Various tools to escape from the emulator window.
// On Windows this is available natively but using Wine causes major pain because wine ui horrible.
// This escape kit tries to smooth out the experience.
namespace WinEscape
{
    // Opens the native window for path
    void OpenNativeFor(const wchar_t* name);

    void ChooseFile(const wchar_t*);
}
