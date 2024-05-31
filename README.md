# Luna's Project64 - Nintendo 64 Emulator

This is a fork of Project64 3.0.1 that I made to have a Linux compatible alternative to 2.4 (Yes, it works on Windows too).
It is not focused on accuracy or compatibility, but rather on functionality that improves the general gameplay experience on a select set of games.
As such, it removes the nagware screen, and it contains the plugins that are necessary to play these games without issues.

## Changes from vanilla:

- The emu will prompt you to update when a new version is ready.
- There are now extra savestate slots, for a total of 100. You can change them in the hotkeys tab from the settings window, or just access them through the system menu.
- Loading savestates is now faster, and doing that a lot while screensharing on Discord will no longer crash the game.
- The keyboard shortcuts to increase and decrease the game's speed have been changed to Ctrl + and Ctrl - respectively, since the original shortcuts kept getting in the way and it wasn't good.
- The bug that caused the EEPROM to show corrupted data (182 stars in every file, etc) the first time you open certain hacks has been patched.
- The Project64 icons are now purple. Just a little silly thing. ^-^
- And a couple more!

### Stable Builds

Download the latest build in the Releases tab of this GitHub repo.
The emulator will prompt you to update when a new version is available.

## Support

[**Join the Lunatic Realm Discord server**](https://discord.gg/w9aCXwsKH9) if you need any help regarding the emu.

### Compiling

```
Visual Studio
```

Load .sln project file and compile


## Contributing

Message @lunaticshin3 on Discord if you would like to contribute. Any help is highly appreciated.

## Versioning

The emu currently uses a variation of semantic versioning, starting with "v3." to differentiate it from official Project64 versions (ie. Project64 v1.6 vs Luna's Project64 v3.6).
This works because official Project64 jumped from v3.0.1 to v4.0.0, so version numbers 3.X.X are unused.
Older versions used a number+letter naming convention that can be translated to the newer versioning system as such:
N-v1  --> v3.1.0
N-v2  --> v3.2.0
N-v2a --> v3.2.1
N-v2b --> v3.2.2
N-v2c --> v3.2.3
N-v3  --> v3.3.0
N-v3a --> v3.3.1
N-v3b --> v3.3.2

## Author / Contributors

* Original Project64 by [Zilmar](https://github.com/project64), **Jabo** and other [contributors](https://github.com/project64/project64/contributors).
* Luna's Project64 by [ShiN3](https://github.com/LunaticShiN3).
* Auto updater by [MarvJungs](https://github.com/MarvJungs) and [aglab2](https://github.com/aglab2).
* See [plugin guide](https://sites.google.com/view/shurislibrary/plugin-guide) for plugin credits.
* Tested by **Winter**, **Hyena Chan**, **HeralayanSalty**, **gnavidude**, **Dackage**, **katze789** and **AndrewSM64**.
* Nagware removal based on a fork by [Kelvinnkat](https://github.com/kelvinnkat).
* Help building by [Kelvinnkat](https://github.com/kelvinnkat) and [Derpyhsi](https://github.com/derpyhsi).
* Help with plugins by [aglab2](https://github.com/aglab2) and [wermi](https://github.com/wermipls).
* Help with new features by [aglab2](https://github.com/aglab2) and **HeralayanSalty**.
* Thanks to [Prakxo](https://github.com/Prakxo) for convincing me to make this, and to [aglab2](https://github.com/aglab2) for annoying me enough to get v3.4.0 out.

## License

This project is licensed under the GPLv2 License - see the [LICENSE.md](https://github.com/project64/project64/blob/develop/license.md) file for details
