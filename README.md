# PlotinusRedux's GUI improvements and renderer for SMACX

PRACX was written by PlotinusRedux and is maintained by DrazharLn, both of [alphacentauri2.info](http://alphacentauri2.info) ([forum thread](http://alphacentauri2.info/index.php?topic=14308.0)). PRACX is permissively licensed free software.

PRACX implements these improvements to the UI:

 - Configurable full screen resolution
 - Working windowed mode
 - Custom menu for PRACX options
 - Mouse over support for view mode (V)
 - Scrolling
	 - Pixel level scrolling
	 - Right click and drag scrolling
	 - Configurable edge scroll zone size and speed
 - Zooming
	 - Pixel-level re-centering: no more screen zigzag
	 - Automatically sets reasonable min, max, and increment values for the current resolution
	 - Mouse wheel zooms in and out
	 - Configurable # of increments between min and max zoom
	 - Details (units, cities, improvements, etc.) are now shown even when fully zoomed out
 - Overlays
	 - Resource overlay with ALT+R: normal/current yield of tile/potential yield
	 - Terrain overlay with ALT+T: normal/faction ownership/elevation/rainfall/rockiness
	 - City mode: unworked tiles show potential yield in grey


## Building

This needs to be built with Visual Studio, or some compiler with equivalent
inline ASM semantics (not gcc). Because Microsoft don't have a stable
commandline API to their compilers, you will likely have to fiddle to get this
to work.

I install msys2 and use a unix-like terminal to do development. When things are
working correctly, all you need to do is `make`, and the binaries and installer
will be generated in `./bin` for you.

Unfortunately, the build system is fragile and probably won't work out of the
box.

### Compiling

The easiest way to build is to download the latest version of visual studio and
install the C++ and XP tools, import the project and make sure the platform
toolset for each "solution" is set to some XP toolset. For v141_xp, for
example. Other faffery might be required.

After that you can either build from VS or open the "Developer Command Prompt"
and run `msbuild path/to/project`.

I take the path from the developer command prompt and use it in
msbuild.workaround, which is used by the Makefile.

### Packaging (installer)

PRACX is packaged with NSIS. The makefile knows how to do this and will emit an
installer in ./bin, if makensis exists on your PATH.

If you don't like make, just point NSIS at the script in InstallScript.


## Code overview

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
