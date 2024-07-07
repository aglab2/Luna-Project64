This is a build of Project64 3.0.1 designed to:
- Contain all of the good plugins for SM64 romhacks with good settings.
- Work on Linux (via Wine).
- Remove the 30 second nagware donation screen.
- Add extra settings.

I would strongly encourage you to read the information at https://sites.google.com/view/shurislibrary/luna-project64
if you haven't already.

I hope you have fun playing!


CHANGELOG:

-- v3.5.0 --
	- Added dark theme.
	- Added SD card emulation support.
	- Added share cheat codes between games setting.
	- Added counter factor 0.
	- Added input delayer.
	- Applied lots of small optimizations to emulator core.
	- Removed Audio HLE.
	- Disabled debugger by default.
	- Added enable/disable debugger option to the debugger tab.
	- Removed CIC Checksum error in old SM64 romhacks.
	- Switched from zlib to zlib-ng.
	- Moved JS scripts folder to %AppData%.
	- Updated ANGLE GLideN64 (v4.3.8 --> v4.3.9.1).
	- Updated ParaLLEl RSP (v0.1 --> v0.5).

-- v3.4.1 --
	- Fixed register caching crashing on certain decomp hacks (Thanks to aglab2).
	- Register caching is now enabled by default, improving performance.
	- Added "1.6 labels" language, for people who hate having correct text in their settings.
	- Removed Octomino from welcome screen.
	- Real N64 Range is now properly disabled by default in NRage.
	- Lock mouse keybind is now unmapped by default in NRage.
	- Added libhidapi-0.dll, required for raphnetraw to work.
	- Updated ParaLLEl RSP (Unknown --> aglab2 v0.1).

-- v3.4.0 --
	- Added auto updater (Thanks to MarvJungs and aglab2).
	- Moved all config and save data files to AppData\Roaming\Luna-Project64 to allow auto updater to work.
	- Changed the welcome screen to let people select a default input plugin and import their save data from a previous PJ64 version.
	- Changed the way default settings are managed to make them no longer jank.
	- Added default keybinds for savestate slots above 10.
	- Register caching is now disabled by default.
	- Changed Discord rich presence to show Luna's Project64 title and icon.
	- Added "Pause emulation while window is minimized" setting.
	- Fixed a bug (found by Bliny) where the memory dump window wouldn't properly write the file directory.
	- Changed Project64.rdb to set VI refresh rate to 1500 in hacks that need it, and VI refresh rate and Counter Factor in vanilla SM64 and OoT.
	- Added LINK's ParaLLel v1.0, based on a different and significantly better ParaLLEl build than what was used in older emu releases.
	- Added raphnetraw (1 player version).
	- Added ParaLLel RSP, which significantly improves the performance of ParaLLEl related plugins.
	- Updated ANGLE GLideN64 (v4.3.5 --> v4.3.8), with lots of changes.
	- Updated LINK's AziAudio (v0.70 WIP 10 [2023] --> Sep 2023), which improves compatibility with different Windows versions.
	- Updated LINK's Mapper (v1.1.1 --> v1.1.2), which stores config in %AppData%.
	- Updated NRage (NRage's Direct-Input8 V2 1.83 --> NRage Input V2 2.4 (ShiN3 fork)), which has wider controller compatibility,
		disables Real N64 range by default, and stores config in %AppData%.
	- Updated pj64-wiiu-gcn (v1.0.0 --> v1.1.0), which adds a couple features such as rumble pak support.
	- Removed GLideN64 Public Release v4.0 (Obsolete due to ANGLE GLideN64 v4.3.7.1 and newer).
	- Removed ParaLLel RDP (Obsolete due to LINK's ParaLLEl).
	- Removed dgVoodoo files for Jabo, as they're only useful for hardware that Jabo shouldn't be used in in the first place.

-- v3.3.2 (N-v3b) --
	- Updated Luna's DirectInput8 (v1.0.2 --> v1.0.3).
	- Updated pj64-wiiu-gcn (v0.2.4 --> v1.0.0).
	- Bundled dgVoodoo files into a zip to still include them but have them disabled by default due to unstability.

-- v3.3.1 (N-v3a) --
	- Added Luna's DirectInput8 v1.0.2.
	- Updated ANGLE GLideN64 (v4.3.4 --> v4.3.5).
	- Updated Octomino's SDL Input wermi (2021.12.15 --> 2023.03.11).
	- Updated LINK's AziAudio (v0.70 WIP 10 --> v0.70 WIP 10 [2023]).
	- Added dgVoodoo files for Jabo DirectX11 patch, which fixes shadows and allows it to run in your dedicated GPU.
	- Removed LINK's DirectInput7 1.6.1 (Obsolete due to Luna's DirectInput8).

-- v3.3.0 (N-v3) --
	- Plugins no longer re-initialize on reset, which makes savestates faster and fixes the Discord screenshare crashes that happen when using GLideN64 or its variations.

-- v3.2.3 (N-v2c) --
	- Updated ANGLE GLideN64 (v4.3.3 --> v4.3.4).

-- v3.2.2 (N-v2b) --
	- Fixed lag issues caused by Visual Studio cringe.

-- v3.2.1 (N-v2a) --
	- Fixed vanilla savestate slots (Default and 1-10) being broken in languages that didn't use the Latin alphabet.
	- Updated the window title to display the correct version name instead of PJ64's default generated one.
	- Removed LibEGL.dll and LibGLESv2.dll (dead remains of ANGLE GLideN64 v4.2).

-- v3.2.0 (N-v2) --
	- Updated ANGLE GLideN64 (v4.2 --> v4.3.3).
	- Removed LINK's GLideN64 (Obsolete due to ANGLE GLideN64 update).
	- Added extra savestate slots, for a total of 100. You can access them through the System menu, and set keybinds for them in the Hotkeys tab in settings.
	  Thanks to aglab2 and HeralayanSalty for helping me implement this.
	- The buttons in the Help menu have been repurposed. "Discord", "Website" and "About Project64" now link to Shuri's Library. "Support Project64" still links
	  to the support page of vanilla Project64.

-- v3.1.0 (N-v1) --
	(Changes from vanilla Project64 3.0.1)
	- Added plugins:
		GFX:
		- LINK's GLideN64 v4.2 (Default)
		- ANGLE GLideN64 v4.2
		- GLideN64 Public Release v4.0
		- Jabo's Direct3D8 1.6.1
		- ParaLLel
		- Angrylion's RDP Plus 1.6
		Audio:
		- LINK's AziAudio (Default)
		Input:
		- NRage's Direct-Input8 V2 1.83 (Default)
		- LINK's Mapper 1.1.1
		- Octomino's SDL Input (wermi's fork)
		- LINK's DirectInput7 1.6.1
		- pj64-wiiu-gcn
	- Changed certain default settings:
		- Disabled: Pause emulation while window is not active
		- Disabled: Hide advanced settings
		- Enabled: Enable debugger
		- Disabled: Enable enhancements
		- Disabled: Store game saves in separate folders
		- Enabled: Speed display (Percentage of full speed)
		- Counter factor --> 1
		- VI refresh rate --> 2200
		- Enabled: 32-bit engine
		- Enabled: Unaligned DMA
		- Hotkey for Increase Game Speed --> Ctrl + +
		- Hotkey for Decrease Game Speed --> Ctrl + -
		Game specific settings:
		- Serene fusion: Disabled Register caching
		- Star Revenge 11 Star Adventure: Enabled Interpreter core
	- Fixed EEPROM "bug" that causes some hacks to give you 182 stars when you first play them and glitches out in-game sound settings.
	- Disabled nagware / 30 second donation window.