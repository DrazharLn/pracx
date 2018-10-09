# PlotinusRedux's GUI improvements and renderer for SMACX

PRACX was written by PlotinusRedux and is maintained by DrazharLn, both of [alphacentauri2.info](http://alphacentauri2.info) ([forum thread](http://alphacentauri2.info/index.php?topic=14308.0)). PRACX is permissively licensed free software.

PRACX implements these improvements to the UI:

 - Configurable full screen resolution
 - Working windowed mode
 	- Toggle with <kbd>alt</kbd>+<kbd>enter</kbd>
 - Custom menu for PRACX options
 - Mouse over support for view mode <kbd>V</kbd>
 - Scrolling
	 - Pixel level scrolling
	 - Right click and drag scrolling
	 - Configurable edge scroll zone size and speed
	 - Mouse wheel scrolls in menus
 - Zooming
	 - Pixel-level re-centering: no more screen zigzag
	 - Right-click and drag scrolling
	 - Mouse wheel zooms in and out
	 - Automatically sets reasonable min, max, and increment values for the current resolution
	 - Configurable # of increments between min and max zoom
	 - Details (units, cities, improvements, etc.) are now shown even when fully zoomed out
 - Overlays
	 - Resource overlay with <kbd>ALT</kbd>+<kbd>R</kbd>: normal/current yield of tile/potential yield
	 	- Potential yield is the nutrient output with a farm + mineral output with a mine + energy output with solar panels
		- Yields are displayed as though for a faction with no max resource limits to make it easier to plan where to place your bases
	 - Terrain overlay with <kbd>ALT</kbd>+<kbd>T</kbd>: normal/faction ownership/elevation/rainfall/rockiness
	 - City mode: unworked tiles show potential yield in a translucent outline (configurable)
	 - Existing terrain survey mode <kbd>T</kbd> has an extra mode: where only fungus and forests are hidden

PRACX is a patch for the Windows version of the game, but it runs fine under Wine (better than under windows 10 for some people!). The [unofficially patched](http://alphacentauri2.info/wiki/Installation#Improving_your_game) Windows version running under Wine is a better experience than the old GNU/Linux port, in my opinion, anyway. Similarly for other OSs.


## Install

You must install this over a version of the .exe that has had the DRM removed. Any of the [unofficial patches](http://alphacentauri2.info/wiki/Installation#Improving_your_game) will do.

Download the latest installer from the [releases page](https://github.com/DrazharLn/pracx/releases). You'll want `PRACX.v1.11.exe` or something similar. Running the installer puts our `.dll`s in your SMAC folder, patches SMAC's binary to use them and re-enables <kdb>alt</kbd>+<kbd>tab</kbd> if it has been disabled (as it is by the GOG installer).

The installer makes backups of files it replaces in `_backup_v{version number}`.


## Uninstall

The entire patch may be temporarily disabled by setting `Disabled=1` in the `[PRACX]` section of `Alpha Centauri.Ini` in the application's directory.  It may be permanently disabled by deleting `prac.dll` and `prax.dll` in the application's directory or by running the uninstaller.


## Configuration

PRACX will create it's own settings section in `Alpha Centauri.Ini` if it doesn't exist. Settings are generally configurable from the in-game menu too and will be reflected in this file.

Here's an example with some settings changed:

```ini
[PRACX]
Disabled=<DEFAULT>
WindowWidth=1920
WindowHeight=1080
ZoomLevels=<DEFAULT>
ScrollMin=<DEFAULT>
ScrollMax=<DEFAULT>
ScrollArea=<DEFAULT>
MouseOverTileInfo=<DEFAULT>
ShowUnworkedCityResources=<DEFAULT>
ListScrollLines=<DEFAULT>
ZoomedOutShowDetails=<DEFAULT>
MoviePlayerCommand=test.bat
ScreenWidth=<DEFAULT>
ScreenHeight=<DEFAULT>
```

## Troubleshooting

SMAC and perhaps especially PRACX may work badly on Windows 10 Creators Update. Make sure you [enable DirectPlay](https://windowsforum.com/threads/turn-on-direct-play-to-use-older-games-windows-8-8-1-1-and-10.205952/).


## Development

You don't need to read this if you just want to use the patch ;)

### Requirements

You need Microsoft's compiler (`cl.exe`) and "XP Tools".

1. Install VS 2017 Community
2. Install XP Tools (I installed these with the VS IDE by setting the platform toolset for a "solution" to v141_xp)
3. Optional: A POSIX environment with make (I use msys2).

This needs to be built with Microsoft's compiler `cl.exe`. It cannot be compiled with gcc because it uses non-standard inline ASM.


You need to ensure that the environment variables `PATH`, `INCLUDE` and `LIB` are set correctly if you're using `make`. I use a script to get the correct values from `vsdevcmd.bat`: `bash generate_vsenv`.

### Building

When everything is working correctly its just `make` and collect the binaries from `bin/`.

If you don't like make then you can set something else up using the Makefile as a build.

I've tried to make the build system simple, but building stuff on windows is a bit tricky so you might need to fiddle with MSVC.
As a first port of call, check that `vsenv` exists and that you can call `cl.exe`.

#### Packaging (installer)

PRACX is packaged with NSIS. The makefile knows how to do this and will emit an
installer in `./bin`, if `makensis` exists on your `PATH`.

If you don't like make, just point NSIS at the script in InstallScript.


### Code overview

From shared/pracx.cpp:

This library is intended to be imported by the SMAC binary and to then cause
SMAC to call functions in this library, sometimes instead of certain of its
traditional components, sometimes purely in addition.

PRACX inserts extra code into SMAC's native map rendering and drawing code, as
well as some aspects of base management screens. PRACX also registers to
receive user interaction events from the OS's window manager and can control
scrolling and zooming behaviour, amongst other things.

The combined effect is that PRACX replaces the main UI of the game, allowing
new features and overlays to be implemented in C++, rather than hacked into
terran(x).exe in assembly.

Function names beginning with "PRACX" belong to the PRACX functions, these are
called from the SMAC binary (terran(x).exe). SMAC is hacked to call these
functions by the HOOK function at the bottom of this file.

Functions called from m_pAC are functions in the SMAC binary (SMAC functions).

Other functions are typically unexported helper functions for the PRACX
functions.

"Thunk"s are bits of assembly code that are used mostly to insert function
calls into the SMAC code where previously the code did not call any functions.

The m_ST structure contains the value of configuration options as read from
Alpha Centauri.ini

Most comments (including this header) were written by Draz. If they have
question marks, they're guesses. If they don't, they're confident guesses :)

I've commented on the purpose of most functions and other chunks of code, and
in a bit more detail about some aspects, but the commentary is still lacking,
especially for some of the more complicated functions.

Of course, this code is tightly coupled with the SMACX code, so a complete
understanding of what's going on isn't possible without looking at the
dissassembled code for that, but maintenance of this code and some extensions
might now be possible.
