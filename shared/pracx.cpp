/*
 * PlotinusRedux's GUI improvements and renderer for SMACX
 *
 * This library is intended to be imported by the SMAC binary and to then cause
 * SMAC to call functions in this library, sometimes instead of certain of its
 * traditional components, sometimes purely in addition.
 *
 * PRACX inserts extra code into SMAC's native map rendering and drawing code, as
 * well as some aspects of base management screens. PRACX also registers to
 * receive user interaction events from the OS's window manager and can control
 * scrolling and zooming behaviour, amongst other things.
 *
 * The combined effect is that PRACX replaces the main UI of the game, allowing
 * new features and overlays to be implemented in C++, rather than hacked into
 * terran(x).exe in assembly.
 *
 * PRACX implements these improvements to the UI:
 * 	Configurable full screen resolution
 * 	Working windowed mode
 * 	Custom menu for PRACX options
 * 	Mouse over support for view mode (V)
 * 	Scrolling
 * 		Pixel level scrolling
 * 		Right click and drag scrolling
 * 		Configurable edge scroll zone size and speed
 * 	Zooming
 * 		Pixel-level re-centering: no more screen zigzag
 * 		Automatically sets reasonable min, max, and increment values for the current resolution
 * 		Mouse wheel zooms in and out
 * 		Configurable # of increments between min and max zoom
 * 		Details (units, cities, improvements, etc.) are now shown even when fully zoomed out
 * 	Overlays
 * 		Resource overlay with ALT+R: normal/current yield of tile/potential yield
 * 		Terrain overlay with ALT+T: normal/faction ownership/elevation/rainfall/rockiness
 * 		City mode: unworked tiles show potential yield in grey
 *
 * Function names beginning with "PRACX" belong to the PRACX functions, these
 * are called from the SMAC binary (terran(x).exe). SMAC is hacked to call
 * these functions by the HOOK function at the bottom of this file.
 *
 * Functions called from m_pAC are functions in the SMAC binary (SMAC
 * functions).
 *
 * Other functions are typically unexported helper functions for the PRACX
 * functions.
 *
 * "Thunk"s are bits of assembly code that are used mostly to insert function
 * calls into the SMAC code where previously the code did not call any
 * functions.
 *
 * The m_ST structure contains the value of configuration options as read from
 * Alpha Centauri.ini
 *
 * Most comments (including this header) were written by Draz. If they have
 * question marks, they're guesses. If they don't, they're confident guesses :)
 *
 * I've commented on the purpose of most functions and other chunks of code,
 * and in a bit more detail about some aspects, but the commentary is still
 * lacking, especially for some of the more complicated functions.
 *
 * Of course, this code is tightly coupled with the SMACX code, so a complete
 * understanding of what's going on isn't possible without looking at the
 * dissassembled code for that, but maintenance of this code and some
 * extensions might now be possible.
 *
 * --Draz
 *
 */

/*
 * Note:
 *
 * Variable and struct member names are in Systems Hungarian Notation.
 * Apparently this is a common windows convention, as barmy as it seems to many
 * of us.
 *
 * https://en.wikipedia.org/wiki/Hungarian_notation
 *
 * iFoo = integer Foo
 * pfncFoo = function pointer Foo (maybe constant function pointer?)
 * m_Foo = module variable Foo, global within this file.
 * szFoo = null terminated string Foo
 * etc.
 *
 */

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <process.h>
#include <commdlg.h>
#include <string>
#include "terran.h"
#include "PRACXSettings.h"
#include "wm2str.cpp"

// _cx macro used to express information particular to smaC or smaX. If _SMAC
// is defined, the tuple evaluated to the first argument, if _SMAC is not
// defined, the tuple evaluates to the second argument.
#ifdef _SMAC
#define _cx(a,b) a
#else
#define _cx(a,b) b
#endif

// PRACX doesn't compile in mingw-g++ yet, because msvc and gcc use
// semantically different __asm extensions, so don't get too excited about a
// GNUC ifdef. This is just to get gnu to spit out the fewest errors.
//
// Ref on asm difference: http://stackoverflow.com/a/35959859
// In short: won't fix.
//
// Needed for GNUC because gcc suppresses these macro definitions in windows.h
// (which is kind of sensible, seeing as they override std::max/min if you're
// using namespace std).
#ifdef __GNUC__
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

// Unused?
#define MAPFLAGS_FLATWORD 1

#define WM_MOVIEOVER			WM_USER + 6
// PRACX doesn't respond to this signal...
#define WM_COMMON_DIALOG_DONE	WM_USER + 7

#define THISCALL_THUNK(a,b) __declspec(naked) void b(void){_asm POP EAX _asm PUSH ECX _asm PUSH EAX	_asm JMP a}

// A reference of SMAC functions Plotinus was interested in.
//
/*
#define _a(a) (a)
#define __ac(ssss,aaaa,bbbb,cccc,dddd) int (ssss*aaaa)bbbb = (int (ssss *)bbbb) _cx(cccc,dddd)
#define __acstd(aaaa,bbbb,cccc,dddd) __ac(__stdcall, aaaa, bbbb, cccc, dddd)
#define __acthis(aaaa,bbbb,cccc,dddd) __ac(__thiscall, aaaa, bbbb, cccc, dddd)
#define __accdecl(aaaa,bbbb,cccc,dddd) __ac(__cdecl, aaaa, bbbb, cccc, dddd)

	__acstd(
_WinMain, (HINSTANCE, HINSTANCE, LPSTR, int), 
	0x0046C1A0, 0x0045F950);__acthis(
CCanvas_Create, (CCanvas*), 
	0x00000000, 0x00000000);__acthis(
_WndProc, (HWND, int, WPARAM, LPARAM), 
	0x00000000, 0x00000000);__acstd(
_ProcZoomKey, (int iZoomType, int iZero), 
	0x00000000, 0x00000000);__acthis(
CMain_TileToPt, (CMain* This, int iTileX, int iTileY, long* piX, long* piY), 
	0x00000000, 0x00000000);__acthis(
CMain_MoveMap, (CMain* This, int iXPos, int iYPos, int a4), 
	0x00000000, 0x00000000);__acthis(
CMain_RedrawMap, (CMain* This, int a2), 
	0x00000000, 0x00000000);__acthis(
CMain_DrawMap, (CMain* This, int a2), 
	0x00000000, 0x00000000);__acthis(
CMain_PtToTile, (CMain* This, POINT p, long* piTileX, long* piTileY), 
	0x00000000, 0x00000000);__acthis(
CInfoWin_DrawTileInfo, (CInfoWin* This), 
	0x00000000, 0x00000000);__accdecl(
_PaintHandler, (RECT *prRect, int a2), 
	0x00000000, 0x00000000);__accdecl(
_PaintMain, (RECT *pRect), 
	0x00000000, 0x00000000);__acthis(
CSpriteFromCanvasRectTrans, (CSprite *This, CCanvas *poCanvas, int iTransparentIndex, int iLeft, int iTop, int iWidth, int iHeight, int a8), 
	0x00000000, 0x00000000);__acthis(
CCanvas_Destroy4, (CCanvas *This), 	
	0x00000000, 0x00000000);__acthis(
CSprite_StretchCopyToCanvas, (CSprite *This, CCanvas *poCanvasDest, int cTransparentIndex, int iLeft, int iTop), 
	0x00000000, 0x00000000);__acthis(
CSprite_StretchCopyToCanvas2, (CSprite *This, CCanvas *poCanvasDest, int cTransparentIndex, int iLeft, int iTop, int iDestScale, int iSourceScale), 
	0x00000000, 0x00000000);__acthis(
CSprite_StretchDrawToCanvas2, (CSprite *This, CCanvas *poCanvas, int a1, int a2, int a3, int a4, int a7, int a8), 
	0x00000000, 0x00000000);__acthis(
CWinBase_IsInvisible, (CWinBase *This), 
	0x00000000, 0x00000000);__acthis(
CTimer_StartTimer, (CTimer *This, int a2, int a3, int iDelay, int iElapse, int uResolution), 
	0x00000000, 0x00000000);__acthis(
CTimer_StopTimer, (CTimer *This), 
	0x00000000, 0x00000000);__acthis(
CWinBase_DrawCityMap, (CWinBase *This, int a2), 
	0x00000000, 0x00000000);__accdecl(
_GetFoodCount, (int iForFaction, int a2, int iTileX, int iTileY, bool fWithFarm), 
	0x00000000, 0x00000000);__accdecl(
_GetProdCount, (int iForFaction, int a2, int iTileX, int iTileY, bool fWithMine), 
	0x00000000, 0x00000000);
__accdecl(_GetEnergyCount, (int iForFaction, int a2, int iTileX, int iTileY, bool fWithSolar), 
	0x00000000, 0x00000000);
__acthis(CImage_FromCanvas, (CImage *This, CCanvas *poCanvasSource, int iLeft, int iTop, int iWidth, int iHeight, int a7), 
	0x00000000, 0x00000000);
__accdecl(_GetElevation, (int iTileX, int iTileY), 
	0x00000000, 0x00000000);
__acthis(CImage_CopyToCanvas2, (CImage *This, CCanvas *poCanvasDest, int x, int y, int a5, int a6, int a7), 
	0x00000000, 0x00000000);
__acthis(CMainMenu_AddSubMenu, (CMainMenu* This, int iMenuID, int iMenuItemID, char *lpString), 
	0x00000000, 0x00000000);
__acthis(CMainMenu_AddBaseMenu, (CMainMenu *This, int iMenuItemID, const char *pszCaption, int a4), 
	0x00000000, 0x00000000);
__acthis(CMainMenu_AddSeparator, (CMainMenu *This, int iMenuItemID, int iSubMenuItemID), 
	0x00000000, 0x00000000);
__acthis(CMainMenu_UpdateVisible, (CMainMenu *This, int a2), 
	0x00000000, 0x00000000);
*/

// Signatures of functions in terran(x).exe

typedef int(__stdcall *START_F)(HINSTANCE, HINSTANCE, LPSTR, int);
typedef int(__thiscall *CCANVAS_CREATE_F)(CCanvas* pthis);
typedef int(__stdcall *WNDPROC_F)(HWND, int, WPARAM, LPARAM);
typedef int(__thiscall *CMAIN_ZOOMPROCESSING_F)(CMain* pthis);
typedef int(__stdcall *PROC_ZOOM_KEY_F)(int iZoomType, int iZero);
typedef int(__thiscall *CMAIN_TILETOPT_F)(CMain* pthis, int iTileX, int iTileY, long* piX, long* piY);
typedef int(__thiscall *CMAIN_MOVEMAP_F)(CMain *pthis, int iXPos, int iYPos, int a4);
typedef int(__thiscall *CMAIN_REDRAWMAP_F)(CMain *pthis, int a2);
typedef int(__thiscall *CMAIN_DRAWMAP_F)(CMain* This, int iOwner, int fUnitsOnly);
typedef int(__thiscall *CMAIN_PTTOTILE_F)(CMain* This, POINT p, long* piTileX, long* piTileY);
typedef int(__thiscall *CINFOWIN_DRAWTILEINFO_F)(CInfoWin* This);
typedef int(__cdecl *PAINTHANDLER_F)(RECT *prRect, int a2);
typedef int(__cdecl *PAINTMAIN_F)(RECT *pRect);
typedef int(__thiscall *CSPRITE_FROMCANVASRECTTRANS_F)(CSprite *This, CCanvas *poCanvas,
	int iTransparentIndex, int iLeft, int iTop, int iWidth, int iHeight, int a8);
typedef int(__thiscall *CCANVAS_DESTROY4_F)(CCanvas *This);
typedef int (__thiscall *CSPRITE_STRETCHCOPYTOCANVAS_F)
	(CSprite *This, CCanvas *poCanvasDest, int cTransparentIndex, int iLeft, int iTop);
typedef int(__thiscall *CSPRITE_STRETCHCOPYTOCANVAS1_F)
	(CSprite *This, CCanvas *poCanvasDest, int cTransparentIndex, int iLeft, int iTop, 
		int iDestScale, int iSourceScale);
typedef int (__thiscall *CSPRITE_STRETCHDRAWTOCANVAS2_F)
	(CSprite *This, CCanvas *poCanvas, int a1, int a2, int a3, int a4, int a7, int a8);
typedef int(__thiscall *CWINBASE_ISVISIBLE_F)(CWinBase *This);
typedef int(__thiscall *CTIMER_STARTTIMER_F)(CTimer *This, int a2, int a3, int iDelay, int iElapse, int uResolution);
typedef int(__thiscall *CTIMER_STOPTIMER_F)(CTimer *This);
typedef int(__thiscall *DRAWCITYMAP_F)(CWinBase *This, int a2);
typedef int(__cdecl *GETFOODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithFarm);
typedef int(__cdecl *GETPRODCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithMine);
typedef int(__cdecl *GETENERGYCOUNT_F)(int iForFaction, int a2, int iTileX, int iTileY, bool fWithSolar);
typedef int(__thiscall *IMAGEFROMCANVAS_F)(CImage *This, CCanvas *poCanvasSource, int iLeft, int iTop, int iWidth, int iHeight, int a7);
typedef int(__cdecl *GETELEVATION_F)(int iTileX, int iTileY);
typedef int(__thiscall *CIMAGE_COPYTOCANVAS2_F)(CImage *This, CCanvas *poCanvasDest, int x, int y, int a5, int a6, int a7);
typedef int(__thiscall *CMAINMENU_ADDSUBMENU_F)(CMainMenu* This, int iMenuID, int iMenuItemID, char *lpString);
typedef int(__thiscall *CMAINMENU_ADDBASEMENU_F)(CMainMenu *This, int iMenuItemID, const char *pszCaption, int a4);
typedef int(__thiscall *CMAINMENU_ADDSEPARATOR_F)(CMainMenu *This, int iMenuItemID, int iSubMenuItemID);
typedef int(__thiscall *CMAINMENU_UPDATEVISIBLE_F)(CMainMenu *This, int a2);
typedef int(__thiscall *CMAINMENU_RENAMEMENUITEM_F)(CMainMenu *This, int iMenuItemID, int iSubMenuItemID, const char *pszCaption);
typedef long(__thiscall *CMAP_GETCORNERYOFFSET_F)(CMap *This, int iTileX, int iTileY, int iCorner);

typedef struct ACADDRESS_S {
	START_F							pfncWinMain;
	HDC*							phDC;
	HWND*							phWnd;
	HPALETTE*						phPallete;
	HINSTANCE*						phInstance;
	WNDPROC_F						pfncWinProc;
	CMAIN_ZOOMPROCESSING_F			pfncZoomProcessing;
	CMain*							pMain;
	int*							piMaxTileX;
	int*							piMaxTileY;
	PROC_ZOOM_KEY_F					pfncProcZoomKey;
	CMAIN_TILETOPT_F				pfncTileToPoint;
	RECT*							prVisibleTiles;
	int*							piMapFlags;
	CMAIN_MOVEMAP_F					pfncMoveMap;
	CMAIN_REDRAWMAP_F				pfncRedrawMap;
	CMAIN_DRAWMAP_F					pfncDrawMap;
	CMAIN_PTTOTILE_F				pfncPtToTile;
	CInfoWin*						pInfoWin;
	CINFOWIN_DRAWTILEINFO_F			pfncDrawTileInfo;
	CMain**							ppMain;
	PAINTHANDLER_F					pfncPaintHandler;
	PAINTMAIN_F						pfncPaintMain;
	CSPRITE_FROMCANVASRECTTRANS_F	pfncSpriteFromCanvasRectTrans;
	CCanvas*						poLoadingCanvas;
	CCANVAS_DESTROY4_F				pfncCanvasDestroy4;
	CSPRITE_STRETCHCOPYTOCANVAS_F	pfncSpriteStretchCopyToCanvas;
	CSPRITE_STRETCHCOPYTOCANVAS1_F	pfncSpriteStretchCopyToCanvas1;
	CSPRITE_STRETCHDRAWTOCANVAS2_F  pfncSpriteStretchDrawToCanvas2;
	CSprite*                        pSprResourceIcons;
	CWinBase*                       pCityWindow;
	CWinBase*                       pAnotherWindow;
	CWinBase*                       pAnotherWindow2;
	int*                            pfGameNotStarted;
	CWINBASE_ISVISIBLE_F		    pfncWinIsVisible;
	CTIMER_STARTTIMER_F				pfncStartTimer;
	CTIMER_STOPTIMER_F              pfncStopTimer;
	DRAWCITYMAP_F                   pfncDrawCityMap;
	int*                            piZoomNum;
	int*                            piZoomDenom;
	int*                            piSourceScale;
	int*                            piDestScale;
	int*                            piResourceExtra;
	int*                            piTilesPerRow;
	CTile**                         paTiles;
	GETFOODCOUNT_F					pfncGetFoodCount;
	GETPRODCOUNT_F					pfncGetProdCount;
	GETENERGYCOUNT_F				pfncGetEnergyCount;
	IMAGEFROMCANVAS_F				pfncImageFromCanvas;
	GETELEVATION_F					pfncGetElevation;
	CIMAGE_COPYTOCANVAS2_F			pfncImageCopytoCanvas2;
	CMAINMENU_ADDSUBMENU_F          pfncMainMenuAddSubMenu;
	CMAINMENU_ADDBASEMENU_F			pfncMainMenuAddBaseMenu;
	CMAINMENU_ADDSEPARATOR_F		pfncMainMenuAddSeparator;
	CMAINMENU_UPDATEVISIBLE_F		pfncMainMenuUpdateVisible;
	CMAINMENU_RENAMEMENUITEM_F		pfncMainMenuRenameMenuItem;
	CMAP_GETCORNERYOFFSET_F			pfncMapGetCornerYOffset;

} ACADDRESSES_T;

// Locations of functions in terran(x).exe

ACADDRESSES_T m_STACAddresses = {
	(START_F)						_cx(0x0046C1A0, 0x0045F950),
	(HDC*)							_cx(0x009C0014, 0x009B7B2C), //84E8
	(HWND*)							_cx(0x009C0010, 0x009B7B28), //84E8
	(HPALETTE*)						_cx(0x009C0660, 0x009B8178), //84E8
	(HINSTANCE*)					_cx(0x009BFFFC, 0x009B7B14), //84E8
	(WNDPROC_F)						_cx(0x006094D0, 0x005F0650),
	(CMAIN_ZOOMPROCESSING_F)		_cx(0x0046F3B0, 0x00462980),
	(CMain*)						_cx(0x0091F500, 0x009156B0),//9E50
	(int*)							_cx(0x009B8AF8, 0x00949870),//6F28C
	(int*)							_cx(0x009B8AFC, 0x00949874),//6F28C
	(PROC_ZOOM_KEY_F)				_cx(0x00528F20, 0x005150D0),
	(CMAIN_TILETOPT_F)				_cx(0x0046F930, 0x00462F00),
	(RECT*)							_cx(0x007F0B40, 0x007D3C28),
	(int*)							_cx(0x009B8B14, 0x0094988C),
	(CMAIN_MOVEMAP_F)				_cx(0x00477A40, 0x0046B1F0),
	(CMAIN_REDRAWMAP_F)				_cx(0x00476DA0, 0x0046A550),
	(CMAIN_DRAWMAP_F)				_cx(0x004764F0, 0x00469CA0),
	(CMAIN_PTTOTILE_F)				_cx(0x0046FA70, 0x00463040),
	(CInfoWin*)						_cx(0x008CF290, 0x008C5568),//9D28
	(CINFOWIN_DRAWTILEINFO_F)		_cx(0x004CE6D0, 0x004B8893),
	(CMain**)						_cx(0x007F0B54, 0x007D3C3C),
	(PAINTHANDLER_F)				_cx(0x006101A0, 0x005F7320),
	(PAINTMAIN_F)					_cx(0x00608BA0, 0x005EFD20),
	(CSPRITE_FROMCANVASRECTTRANS_F) _cx(0x005FC7D0, 0x005E39A0),
	(CCanvas*)						_cx(0x007C9C48, 0x00798668),
	(CCANVAS_DESTROY4_F)			_cx(0x005F0270, 0x005D7470),
	(CSPRITE_STRETCHCOPYTOCANVAS_F) _cx(0x005FD9E0, 0x005E4B9A),
	(CSPRITE_STRETCHCOPYTOCANVAS1_F)_cx(0x005FD990, 0x005E4B4A),
	(CSPRITE_STRETCHDRAWTOCANVAS2_F)_cx(0x005FCC40, 0x005E3E00),
	(CSprite*)						_cx(0x0077DC60, 0x007A72A0),
	(CWinBase*)						_cx(0x006C7310, 0x006A7628),
	(CWinBase*)						_cx(0x008AFF98, 0x008A6270),
	(CWinBase*)						_cx(0x008D0B88, 0x008C6E68),
	(int*)							_cx(0x006AEACC, 0x0068F21C),
	(CWINBASE_ISVISIBLE_F)			_cx(0x00610D10, 0x005F7E90),
	(CTIMER_STARTTIMER_F)			_cx(0x0062FE60, 0x00616350),
	(CTIMER_STOPTIMER_F)			_cx(0x00630290, 0x00616780),
	(DRAWCITYMAP_F)					_cx(0x00413730, 0x0040F0F0),
	(int*)							_cx(0x006B0114, 0x00691E6C),
	(int*)							_cx(0x006B0118, 0x00691E70),
	(int*)							_cx(0x006B4FBC, 0x00696D1C),
	(int*)							_cx(0x006B4FB8, 0x00696D18),
	(int*)							_cx(0x00918634, 0x0090E998),
	(int*)							_cx(0x006AEF44, 0x0068FAF0),
	(CTile**)						_cx(0x009B9594, 0x0094A30C),
	(GETFOODCOUNT_F)				_cx(0x004FC6C0, 0x004E6E50),
	(GETPRODCOUNT_F)				_cx(0x004FCAB0, 0x004E7310),
	(GETENERGYCOUNT_F)				_cx(0x004FCDE0, 0x004E7750),
	(IMAGEFROMCANVAS_F)				_cx(0x00634010, 0x00619710),
	(GETELEVATION_F) 				_cx(0x005A8E10, 0x005919C0),
	(CIMAGE_COPYTOCANVAS2_F)		_cx(0x0063DCC0, 0x006233C0),
	(CMAINMENU_ADDSUBMENU_F)		_cx(0x006141D0, 0x005FB100),
	(CMAINMENU_ADDBASEMENU_F)		_cx(0x00613F10, 0x005FAEF0),
	(CMAINMENU_ADDSEPARATOR_F)		_cx(0x00614230, 0x005FB160),
	(CMAINMENU_UPDATEVISIBLE_F)		_cx(0x0046D690, 0x00460DD0),
	(CMAINMENU_RENAMEMENUITEM_F)	_cx(0x00614840, 0x005FB700),
	(CMAP_GETCORNERYOFFSET_F)		_cx(0x0047CA10, 0x0046FE70)
};

ACADDRESSES_T* m_pAC = &m_STACAddresses;

MENU_HANDLER_CB_F m_pfncMainMenuHandler = NULL;


// Function signatures of WINAPI calls that SMAC uses.
// During PRACXHook, these function pointers are set to whatever SMAC was
// trying to point at originally. Instead of calling these, we've made SMAC
// call the PRACX versions of these functions as defined in this file.
//
// Why not just call the WINAPI functions direct from here? Maybe someone else
// has already overridden them, this way we don't undo that.
ATOM (WINAPI *pfncRegisterClassA)(WNDCLASS*) = NULL;
HWND (WINAPI *pfncCreateWindowEx)(
	_In_      DWORD dwExStyle,
	_In_opt_  LPCTSTR lpClassName,
	_In_opt_  LPCTSTR lpWindowName,
	_In_      DWORD dwStyle,
	_In_      int x,
	_In_      int y,
	_In_      int nWidth,
	_In_      int nHeight,
	_In_opt_  HWND hWndParent,
	_In_opt_  HMENU hMenu,
	_In_opt_  HINSTANCE hInstance,
	_In_opt_  LPVOID lpParam
	) = NULL;
BOOL (WINAPI *pfncShowWindow)(
	_In_  HWND hWnd,
	_In_  int nCmdShow
	) = NULL;
int (WINAPI *pfncGetSystemMetrics)(
	_In_  int nIndex
	) = NULL;
SHORT (WINAPI *pfncGetAsyncKeyState)(
	_In_  int vKey
	) = NULL;
BOOL (WINAPI *pfncGetCursorPos)(
	_Out_  LPPOINT lpPoint
	) = NULL;
HDC (WINAPI *pfncBeginPaint)(
	_In_   HWND hwnd,
	_Out_  LPPAINTSTRUCT lpPaint
	) = NULL;
BOOL (WINAPI *pfncEndPaint)(
	_In_  HWND hWnd,
	_In_  const PAINTSTRUCT *lpPaint
	) = NULL;
BOOL (WINAPI *pfncInvalidateRect)(
	_In_  HWND hWnd,
	_In_  const RECT *lpRect,
	_In_  BOOL bErase
	) = NULL;
BOOL(WINAPI *pfncGetOpenFileName)(LPOPENFILENAME lpofn) = NULL;
BOOL(WINAPI *pfncGetSaveFileName)(LPOPENFILENAME lpofn) = NULL;

// Flags to give CreateWindowEx, ShowWindow, etc.
#define AC_WS_WINDOWED (WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | \
	WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_CLIPCHILDREN)
#define AC_WS_FULLSCREEN (WS_POPUP | WS_CLIPCHILDREN)

// Height in px of the UI console (the big grey thing) in map mode of SMAC.
#define CONSOLE_HEIGHT 219

// Settings read from Alpha Centauri.ini
CSettings m_ST;

bool m_fWindowed = false;
bool m_Minimised = false;
RECT m_rWindowedRect;
RECT m_rPaintSaved = { 0 };

bool m_fPlayingMovie = false;
bool m_fScrolling = false;

double m_dScrollOffsetX = 0.0;
double m_dScrollOffsetY = 0.0;

int m_fShiftShowUnworkedCityResources = 0;

POINT m_ptScrollDragPos = { 0, 0 };
bool m_fRightButtonDown = false;
bool m_fScrollDragging = false;

int m_fGrayResources = false;
CSprite m_astGrayResourceSprites[24];
CImage m_aimgFactionColors[8];
CImage m_aimgRaininess[3];
CImage m_aimgRockiness[3];
CImage m_aimgElevation[4];

int m_fGlobalTimersEnabled = false;

int m_iResourceMode = 0;

int m_iDrawTileX = 0;
int m_iDrawTileY = 0;
CMain* m_pDrawTileMain = NULL;
int m_iTerrainMode = 0;

HMODULE m_hlib = NULL;

bool m_fShowingCommDialog = false;

#define Round(d) \
	((d < 0) ? (int)(d - 0.5) : (int)(d + 0.5))
#define RoundUp(d) \
	((d < 0) ? (int)(d - 0.9999999) : (int)(d + 0.9999999))


void SetTerrainMode(int iMode);
void SetResourceMode(int iMode);

// Is city management window showing
bool IsCityShowing(void)
{
	bool fRet = 
		m_fGlobalTimersEnabled && 
		!*m_pAC->pfGameNotStarted &&
		m_pAC->pfncWinIsVisible(m_pAC->pCityWindow);

	return fRet;
}


bool IsMapShowing(void)
{
	bool fRet = 
		m_fGlobalTimersEnabled &&
		!*m_pAC->pfGameNotStarted &&
		!IsCityShowing() &&
		!m_pAC->pfncWinIsVisible(m_pAC->pAnotherWindow) &&
		!m_pAC->pfncWinIsVisible(m_pAC->pAnotherWindow2);

	return fRet;
}

// This maps points from the Client (Game window) coordinate system to the Backbuffer (Buffer which gets rendered to screen)
void ClientToBackbuffer(POINT* pPt)
{
	RECT r;

	if (m_fWindowed && m_pAC && *m_pAC->phWnd)
	{
		GetClientRect(*m_pAC->phWnd, &r);
		if (r.left < r.right && r.top < r.bottom)
		{
			pPt->x = (pPt->x - r.left) * m_ST.m_ptScreenSize.x / (r.right - r.left);
			pPt->y = (pPt->y - r.top) * m_ST.m_ptScreenSize.y / (r.bottom - r.top);
		}
	}
}

// Maps points from Backbuffer to client coordinate systems
void BackbufferToClient(POINT* pPt)
{
	RECT r;

	if (m_fWindowed && m_pAC && *m_pAC->phWnd)
	{
		GetClientRect(*m_pAC->phWnd, &r);
		if (r.left < r.right && r.top < r.bottom)
		{
			pPt->x = (r.right - r.left) * pPt->x / m_ST.m_ptScreenSize.x + r.left;
			pPt->y = (r.bottom - r.top) * pPt->y / m_ST.m_ptScreenSize.y + r.top;
		}
	}
}

// Set the video mode of the monitor (change resolution)
void SetVideoMode(void)
{
	if ((m_ST.m_ptScreenSize.x != m_ST.m_ptDefaultScreenSize.x || m_ST.m_ptScreenSize.y != m_ST.m_ptDefaultScreenSize.y ) && !m_fWindowed)
	{
		DEVMODE stDV = { 0 };

		stDV.dmPelsWidth = m_ST.m_ptScreenSize.x;
		stDV.dmPelsHeight = m_ST.m_ptScreenSize.y;
		stDV.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
		stDV.dmSize = sizeof(stDV);

		ChangeDisplaySettings(&stDV, CDS_FULLSCREEN);
	}
}

// Return monitor to default settings.
void RestoreVideoMode(void)
{
	ChangeDisplaySettings(NULL, 0);
}

// Calls an external program to play videos, pausing SMAC until that process completes.
//
// I think built-in SMAC video playing system is better - it overlays text at
// the end of the videos and has fewer controls.
//
// Plotinus intercepts the movie playing system because one of the destructors
// of that system re-initialises DirectX, or maybe just SMAC internals, and
// that means SMAC gets rendered at a low resolution and the menus go weird.
// TODO: investigate this more, maybe I can make a smaller change to the
// destructor or just decompile SMAC's video renderer.
void __cdecl PRACXShowMovie(const char *pszFileName)
{
	log(pszFileName);

	// SMAC sometimes provides a filename with extension, and sometimes does not - normalise.
	std::string filename(pszFileName);
	if (filename.length() < 1)
		// This should never happen, but SMAC is buggy, so let's be defensive.
		return;
	else if (filename[filename.length() - 4] != '.')
		filename += ".wve";

	// Accept an arbitrary commandline from the config file to append the filepath to
	// and execute.
	//
	// This is configurable because playuv15 (the default) isn't very good (and
	// because it's useful to some people to pass options to playuv15).
	//
	// It would be nice to use ffplay or similar, but the videos are encoded with EA's
	// proprietary TQI codec, and as of July 2017, ffmpeg still doesn't have a good
	// codec for it - playing with mpv (with or without all the fancy upscaling
	// algorithms) gives a worse result than playuv15.
	std::string command = m_ST.m_szMoviePlayerCommand;
	command += " .\\movies\\";
	command += filename;

	// Set this flag so we can react appropriately to some interrupting signals
	// (currently just toggle windowed mode)
	m_fPlayingMovie = true;

	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	if (CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	// Reset display mode after movie.
	// Maybe Plotinus did it this way because SMAC waits on this signal for some reason?
	// TODO: test that hyp.
	PostMessage(*m_pAC->phWnd, WM_MOVIEOVER, 0, 0);
}

// Create the main window of the game. Size from config file.
HWND WINAPI PRACXCreateWindowEx(
	_In_      DWORD dwExStyle,
	_In_opt_  LPCTSTR lpClassName,
	_In_opt_  LPCTSTR lpWindowName,
	_In_      DWORD dwStyle,
	_In_      int x,
	_In_      int y,
	_In_      int nWidth,
	_In_      int nHeight,
	_In_opt_  HWND hWndParent,
	_In_opt_  HMENU hMenu,
	_In_opt_  HINSTANCE hInstance,
	_In_opt_  LPVOID lpParam
	)
{
	nWidth = m_ST.m_ptScreenSize.x;
	nHeight = m_ST.m_ptScreenSize.y;
	dwExStyle = 0;
	dwStyle |= WS_CLIPCHILDREN;

	log("w: " << nWidth << "\th: " << nHeight << "\tx: " << x << "\ty: " << y);

	return pfncCreateWindowEx(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		x,
		y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam
	);
}

BOOL WINAPI PRACXGetCursorPos(
	_Out_  LPPOINT p
	)
{
	RECT r;
	BOOL fRet;

	fRet = pfncGetCursorPos(p);
	if (m_fWindowed)
	{
		// Have to map the coordinate to the backbuffer if we're windowed.
		GetClientRect(*m_pAC->phWnd, &r);
		// WinAPI call
		ScreenToClient(*m_pAC->phWnd, p);

		fRet = PtInRect(&r, *p);
		
		ClientToBackbuffer(p);
	}

	return fRet;
}

// Unused?
SHORT WINAPI PRACXGetAsyncKeyState(
	_In_  int vKey
	)
{
	log(vKey);
	SHORT sRet = 0;

	sRet = pfncGetAsyncKeyState(vKey);

	return sRet;
}

// Set the main SMAC window to be fullscreen or windowed.
void SetWindowed(bool fValue)
{
	static bool fInitialized = false;

	if (m_fWindowed != fValue && *m_pAC->phWnd && !m_fPlayingMovie)
	{
		log(fValue);
		m_fWindowed = fValue;

		if (m_fWindowed)
		{
			RestoreVideoMode();
			SetWindowLong(*m_pAC->phWnd, GWL_STYLE, AC_WS_WINDOWED);
			PostMessage(*m_pAC->phWnd, WM_USER + 3, 0, 0);
		}
		else
		{
			WINDOWPLACEMENT wp;

			SetVideoMode();
			m_fWindowed = false;
			if (fInitialized)
			{
				memset(&wp, 0, sizeof(wp));
				wp.length = sizeof(wp);
				GetWindowPlacement(*m_pAC->phWnd, &wp);
				memcpy(&m_rWindowedRect, &wp.rcNormalPosition, sizeof(m_rWindowedRect));
			}
			else
				fInitialized = true;
			SetWindowLong(*m_pAC->phWnd, GWL_STYLE, AC_WS_FULLSCREEN);
			SetWindowPos(*m_pAC->phWnd, HWND_TOPMOST, 0, 0, m_ST.m_ptScreenSize.x, m_ST.m_ptScreenSize.x, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
			PostMessage(*m_pAC->phWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
	}
}

// Minimise/restore SMAC if it's not in the desired state already
void SetMinimised(bool minimise)
{

	// Previously this also refused to minimise if a movie was playing.
	if (m_Minimised != minimise && *m_pAC->phWnd)
	{
		log(minimise);
		m_Minimised = minimise;
		if (minimise)
		{
			RestoreVideoMode();
			ShowWindow(*m_pAC->phWnd, SW_MINIMIZE);
		}
		else
		{
			ShowWindow(*m_pAC->phWnd, SW_RESTORE);
			SetVideoMode();
		}
	}
}


// Unused?
BOOL WINAPI PRACXShowWindow(
	_In_  HWND hWnd,
	_In_  int nCmdShow
	)
{
//	if (!m_fWindowed)
//		nCmdShow = SW_SHOWMAXIMIZED;

	return pfncShowWindow(hWnd, nCmdShow);
}

// Unused override.
int __stdcall PRACXWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	log(hInstance << "\t" << hPrevInstance << "\t" << lpCmdLine << "\t" << nShowCmd);
	return m_pAC->pfncWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

// Override the windows api call of same name to give fake screensize values to SMAC.
int WINAPI PRACXGetSystemMetrics(int nIndex)
{
	int iResult;

	switch (nIndex)
	{
	case SM_CXSCREEN:
		iResult = m_ST.m_ptScreenSize.x;
		break;
	case SM_CYSCREEN:
		iResult = m_ST.m_ptScreenSize.y;
		break;
	default:
		iResult = pfncGetSystemMetrics(nIndex);
		break;
	}

	log(nIndex << "\t" << iResult << "\t" << pfncGetSystemMetrics(nIndex));

	return iResult;
}

// Callback for mouse movement. Calculate which tile mouse is over and update info window accordingly.
void MouseOver(POINT* p)
{
	static POINT ptLastTile = { 0, 0 };
	POINT ptTile;

	if (m_ST.m_fMouseOverTileInfo &&
		// Check we're in <V> mode?
		!m_pAC->pMain->fUnitNotViewMode &&
		// Checking pointer is over map
		p->x >= 0 && p->x < m_ST.m_ptScreenSize.x &&
		p->y >= 0 && p->y < (m_ST.m_ptScreenSize.y - CONSOLE_HEIGHT) &&
		// Getting tile address
		0 == m_pAC->pfncPtToTile(m_pAC->pMain, *p, &ptTile.x, &ptTile.y) &&
		// Only redraw info window if tile has changed.
		0 != memcmp(&ptTile, &ptLastTile, sizeof(POINT)))
	{
		// Redraw info window
		m_pAC->pInfoWin->iTileX = ptTile.x;
		m_pAC->pInfoWin->iTileY = ptTile.y;
		m_pAC->pfncDrawTileInfo(m_pAC->pInfoWin);

		memcpy(&ptLastTile, &ptTile, sizeof(POINT));
	}
}

// Some kind of performance measurement?
ULONGLONG GetMSCount(void)
{
	static LONGLONG llFrequency = 0;
	static ULONGLONG ullLast = 0;
	static ULONGLONG ullHigh = 0;
	ULONGLONG ullRet;
	
	if (llFrequency == 0)
	{
		if (!QueryPerformanceFrequency((LARGE_INTEGER*)&llFrequency))
			llFrequency = -1LL;
	}

	if (llFrequency > 0)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&ullRet);
		ullRet *= 1000;
		ullRet /= (ULONGLONG)llFrequency;
	}
	else if (llFrequency < 0) 
	{
		ullRet = GetTickCount();
		if (ullRet < ullLast)
			ullHigh += 0x100000000ULL;
		ullLast = ullRet;
		ullRet += ullHigh;
	}

	return ullRet;
}

// TODO: Fix #3: I think this is buggy and hangs when the pointer is too close to the upper or lower bounds of the map. Bug might be in PRACXCheckScroll, though.
bool DoScroll(double x, double y)
{
	bool fScrolled = false;
	CMain* pMain = m_pAC->pMain;
	int mx = *m_pAC->piMaxTileX;
	int my = *m_pAC->piMaxTileY;
	int i;
	int h = m_ST.m_ptScreenSize.y - CONSOLE_HEIGHT;
	int d;

	if (x && pMain->oMap.iMapTilesEvenX + pMain->oMap.iMapTilesOddX < mx)
	{
		if (x < 0 && (!(*m_pAC->piMapFlags & 1) || pMain->oMap.iMapTileLeft > 0))
		{
			i = (int)m_dScrollOffsetX;
			m_dScrollOffsetX -= x;
			fScrolled = fScrolled || (i != (int)m_dScrollOffsetX);

			while (m_dScrollOffsetX >= pMain->oMap.iPixelsPerTileX)
			{
				m_dScrollOffsetX -= pMain->oMap.iPixelsPerTileX;
				pMain->oMap.iTileX -= 2;
				if (pMain->oMap.iTileX < 0)
				{
					if (*m_pAC->piMapFlags & 1)
					{
						pMain->oMap.iTileX = 0;
						pMain->oMap.iTileY &= ~1;
						m_dScrollOffsetX = 0;
					}
					else
					{
						pMain->oMap.iTileX += mx;
					}
				}
			}
		}
		else if (x < 0 && (*m_pAC->piMapFlags & 1))
		{
			fScrolled = true;
			m_dScrollOffsetX = 0;
		}

		if (x > 0 &&
			(!(*m_pAC->piMapFlags & 1) ||
			pMain->oMap.iMapTileLeft +
			pMain->oMap.iMapTilesEvenX +
			pMain->oMap.iMapTilesOddX <= mx))
		{
			i = (int)m_dScrollOffsetX;
			m_dScrollOffsetX -= x;
			fScrolled = fScrolled || (i != (int)m_dScrollOffsetX);

			while (m_dScrollOffsetX <= -pMain->oMap.iPixelsPerTileX)
			{
				m_dScrollOffsetX += pMain->oMap.iPixelsPerTileX;
				pMain->oMap.iTileX += 2;
				if (pMain->oMap.iTileX > mx)
				{
					if (*m_pAC->piMapFlags & 1)
					{
						pMain->oMap.iTileX = mx;
						pMain->oMap.iTileY &= ~1;
						m_dScrollOffsetX = 0;
					}
					else
					{
						pMain->oMap.iTileX -= mx;
					}
				}
			}
		}
		else if (x > 0 && (*m_pAC->piMapFlags & 1))
		{
			fScrolled = true;
			m_dScrollOffsetX = 0;
		}
	}

	if (y && pMain->oMap.iMapTilesEvenY + pMain->oMap.iMapTilesOddY < my)
	{
		int iMinTileY = pMain->oMap.iMapTilesOddY - 2;
		int iMaxTileY = my + 4 - pMain->oMap.iMapTilesOddY;

		while (pMain->oMap.iTileY < iMinTileY) 
			pMain->oMap.iTileY += 2;

		while (pMain->oMap.iTileY > iMaxTileY)
			pMain->oMap.iTileY -= 2;

		d = (pMain->oMap.iTileY - iMinTileY) * pMain->oMap.iPixelsPerHalfTileY - (int)m_dScrollOffsetY;

		if (y < 0 && d > 0 )
		{
			if (y < -d)
				y = -d;
			
			i = (int)m_dScrollOffsetY;
			m_dScrollOffsetY -= y;
			fScrolled = fScrolled || (i != (int)m_dScrollOffsetY);

			while (m_dScrollOffsetY >= pMain->oMap.iPixelsPerTileY && pMain->oMap.iTileY - 2 >= iMinTileY)
			{
				m_dScrollOffsetY -= pMain->oMap.iPixelsPerTileY;
				pMain->oMap.iTileY -= 2;
			}
		}

		d = (iMaxTileY - pMain->oMap.iTileY + 1) * pMain->oMap.iPixelsPerHalfTileY + (int)m_dScrollOffsetY;

		if (y > 0 && d > 0)
		{
			if (y > d)
				y = d;

			i = (int)m_dScrollOffsetY;
			m_dScrollOffsetY -= y;
			fScrolled = fScrolled || (i != (int)m_dScrollOffsetY);

			while (m_dScrollOffsetY <= -pMain->oMap.iPixelsPerTileY && pMain->oMap.iTileY + 2 <= iMaxTileY)
			{
				m_dScrollOffsetY += pMain->oMap.iPixelsPerTileY;
				pMain->oMap.iTileY += 2;
			}
		}
	}

	if (fScrolled)
	{
		m_pAC->pfncRedrawMap(pMain, 0);
		m_pAC->pfncPaintHandler(NULL, 0);
		m_pAC->pfncPaintMain(NULL);
		ValidateRect(*m_pAC->phWnd, NULL);
	}

	return fScrolled;
}

void __stdcall PRACXCheckScroll(void)
{
	CMain* pMain = m_pAC->pMain;
	int mx = *m_pAC->piMaxTileX;
	int my = *m_pAC->piMaxTileY;
	int w = m_ST.m_ptScreenSize.x;
	int h = m_ST.m_ptScreenSize.y;
	ULONGLONG ullOldTickCount = 0;
	ULONGLONG ullNewTickCount = 0;
	double dTPS;
	POINT p;
	BOOL fScrolled;
	BOOL fScrolledAtAll = false;
	int iScrollArea = m_ST.m_iScrollArea * m_ST.m_ptScreenSize.x / 1024;
	BOOL fLeftButtonDown = ( GetAsyncKeyState(VK_LBUTTON) < 0 );
	double dx, dy;
	static ULONGLONG ullDeactiveTimer = 0;

	if (m_fScrolling || ( !PRACXGetCursorPos(&p) && !(m_fScrollDragging && m_fRightButtonDown )))
		return;

	m_fScrolling = true;

	if (m_fRightButtonDown && GetAsyncKeyState(VK_RBUTTON) < 0)
	{
		if (labs((long)hypot((double)(p.x-m_ptScrollDragPos.x), (double)(p.y-m_ptScrollDragPos.y))) > 2.5)
		{
			m_fScrollDragging = true;
			SetCursor(LoadCursor(0, IDC_HAND));
		}
	}

	m_dScrollOffsetX = pMain->oMap.iMapPixelLeft;
	m_dScrollOffsetY = pMain->oMap.iMapPixelTop;

	ullNewTickCount = GetMSCount();
	ullOldTickCount = ullNewTickCount;

	do {
		dx = 0;
		dy = 0;
		fScrolled = false;

		if (m_fScrollDragging && m_fRightButtonDown)
		{
			dx = m_ptScrollDragPos.x - p.x;
			dy = m_ptScrollDragPos.y - p.y;
			memcpy(&m_ptScrollDragPos, &p, sizeof(POINT));
			fScrolled = true;
		}
		else if (ullNewTickCount - ullDeactiveTimer > 100 && !m_fScrollDragging)
		{
			double dMin = (double)m_ST.m_iScrollMin;
			double dMax = (double)m_ST.m_iScrollMax;
			double dArea = (double)iScrollArea;

			if (p.x <= iScrollArea && p.x >= 0)
			{
				fScrolled = true;
				dTPS = dMin + (dArea - (double)p.x) / dArea * (dMax - dMin);
				dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileX / -1000.0;
			}
			else if ((w - p.x) <= iScrollArea && w >= p.x)
			{
				fScrolled = true;
				dTPS = dMin + (dArea - (double)(w - p.x)) / dArea * (dMax - dMin);
				dx = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileX / 1000.0;
			}


			if (p.y <= iScrollArea && p.y >= 0)
			{
				fScrolled = true;
				dTPS = dMin + (dArea - (double)p.y) / dArea * (dMax - dMin);
				dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileY / -1000.0;
			}
			else if (h - p.y <= iScrollArea && h >= p.y &&
				(p.x <= (m_ST.m_ptScreenSize.x - 1024) / 2 ||
				 p.x >= (m_ST.m_ptScreenSize.x - 1024) / 2 + 1024 ||
				 h - p.y < ((m_fWindowed)?8:5) * m_ST.m_ptScreenSize.y / 756 ) )
			{
				fScrolled = true;
				dTPS = dMin + (dArea - (double)(h - p.y)) / dArea * (dMax - dMin);
				dy = (double)(ullNewTickCount - ullOldTickCount) * dTPS * (double)pMain->oMap.iPixelsPerTileY / 1000.0;
			}
		}

		if (fScrolled)
		{
			ullOldTickCount = ullNewTickCount;

			if (DoScroll(dx, dy))
				fScrolledAtAll = true;
			else
				Sleep(0);

			ullNewTickCount = GetMSCount();

			if (m_fRightButtonDown)
				m_fRightButtonDown = (GetAsyncKeyState(VK_RBUTTON) < 0);
		}
	} while (fScrolled && (PRACXGetCursorPos(&p) || (m_fScrollDragging && m_fRightButtonDown)));

	if (fScrolledAtAll)
	{
		pMain->oMap.field_21A44 = 1;
		m_pAC->pfncMoveMap(pMain, pMain->oMap.iTileX, pMain->oMap.iTileY, 1);
		pMain->oMap.field_21A44 = 0;
		for (int i = 1; i < 8; i++)
		{
			if (m_pAC->ppMain[i] && m_pAC->ppMain[i]->oMap.field_1DD74 &&
				(!fLeftButtonDown || m_pAC->ppMain[i]->oMap.field_1DD80) &&
				m_pAC->ppMain[i]->oMap.iMapTilesOddX + m_pAC->ppMain[i]->oMap.iMapTilesEvenX < *m_pAC->piMaxTileX)
			{
				m_pAC->pfncMoveMap(m_pAC->ppMain[i], pMain->oMap.iTileX, pMain->oMap.iTileY, 1);
			}
		}

		if (m_fScrollDragging)
			ullDeactiveTimer = ullNewTickCount;
	}

	// TODO: Fix #13: check if mouse has moved since last tick, if it has update MouseOver, else, don't.
	MouseOver(&p);

	m_fScrolling = false;
}

__declspec(naked) void PRACXCheckScroll_Thunk(void)
{
	__asm{
		PUSHA
		CALL PRACXCheckScroll
		POPA
		MOV EDI, [m_pAC]
		MOV EDI, [EDI + ACADDRESSES_T.ppMain]
		ADD EDI, 4
		RET
	}
}

// React to events from window manager (Mouse, Keyboard, WM stuff).
//
// TODO: Is this broken because it doesn't callnexthook?
LRESULT __stdcall PRACXWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int iDeltaAccum = 0;
	int iRet = 0;
	bool fHasFocus = (GetFocus() == *m_pAC->phWnd);

	const std::string msgName = wm2str(msg, false);
	if (!msgName.empty()) {
		log(msgName);
	}

	if (msg == WM_MOVIEOVER)
	{
		log("WM_MOVIEOVER");
		m_fPlayingMovie = false;

		if (!m_fWindowed)
		{
			if (!IsZoomed(hwnd))
			{
				m_fWindowed = true;
				SetWindowed(false);
			}
			else
				SetVideoMode();
		}
	}
	else if (msg == WM_MOUSEWHEEL &&  fHasFocus)
	{
		log("WM_MOUSEWHEEL");
		int iDelta = GET_WHEEL_DELTA_WPARAM(wParam) + iDeltaAccum;
		iDeltaAccum = iDelta % WHEEL_DELTA;
		iDelta /= WHEEL_DELTA;
		bool fUp = (iDelta >= 0);
		iDelta = labs(iDelta);

		// TODO: Fix #5. Check for more dialogue boxes before allowing zooming.
		if (IsMapShowing() && *m_pAC->piMaxTileX)
		{
			int iZoomType = (fUp) ? 515 : 516;

			for (int i = 0; i < iDelta; i++)
			{
				m_pAC->pfncProcZoomKey(iZoomType, 0);
			}
		}
		else
		{
			int iKey = (fUp) ? VK_UP : VK_DOWN;
			iDelta *= m_ST.m_iListScrollDelta;

			for (int i = 0; i < iDelta; i++)
			{
				PostMessage(hwnd, WM_KEYDOWN, iKey, 0);
				PostMessage(hwnd, WM_KEYUP, iKey, 0);
			}

			return 0;
		}
	}
	// If window has just become inactive e.g. ALT+TAB
	else if (msg == WM_ACTIVATEAPP && !m_fWindowed && !m_fShowingCommDialog)
	{
		// wParam is 0 if the window has become inactive.
		if (!LOWORD(wParam))
			SetMinimised(true);
		else 
			SetMinimised(false);
		iRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	// Toggle windowed/fullscreen on alt-enter
	else if ((msg == WM_KEYDOWN && LOWORD(wParam) == VK_RETURN) && GetAsyncKeyState(VK_MENU) < 0)
	{
		SetWindowed(!m_fWindowed);
	}
	else if (msg == WM_KEYDOWN && LOWORD(wParam) == VK_SHIFT && IsCityShowing() &&
		!m_fShiftShowUnworkedCityResources)
	{
		m_fShiftShowUnworkedCityResources = true;
		m_pAC->pfncDrawCityMap(m_pAC->pCityWindow, 0);
		iRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	else if (msg == WM_KEYUP && LOWORD(wParam) == VK_SHIFT && m_fShiftShowUnworkedCityResources)
	{
		m_fShiftShowUnworkedCityResources = false;
		if (IsCityShowing())
			m_pAC->pfncDrawCityMap(m_pAC->pCityWindow, 0);
		iRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	// Signal WM_USER+3 is sent by SetWindowed(true)
	// Why run this code here rather than in SetWindowed? I dunno.
	else if (msg == WM_USER + 3)
	{
		log("WM_USER+3");
		WINDOWPLACEMENT wp;

		memset(&wp, 0, sizeof(wp));
		wp.length = sizeof(wp);
		GetWindowPlacement(hwnd, &wp);
		wp.flags = 0;
		wp.showCmd = SW_SHOWNORMAL;		
		wp.rcNormalPosition = m_rWindowedRect;
		SetWindowPlacement(hwnd, &wp);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	}
	// If the window is not fullscreen, and the user presses the maximize button, set fullscreen.
	else if (msg == WM_SYSCOMMAND && m_fWindowed && (wParam & 0xFFF0) == SC_MAXIMIZE)
	{
		log("WM_SYSCOMMAND\tMAXIMIZE");
		SetWindowed(false);
	}
	// If we get sent the close signal (close button, alt+f4), simulate pressing escape.
	else if (msg == WM_SYSCOMMAND)
	{
		log("WM_SYSCOMMAND\t"<<wParam);
		if ((wParam & 0xFFF0) == SC_CLOSE)
		{
			PostMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
			return -1;
		}
			//wParam = SC_MINIMIZE;

		iRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	else if (msg == WM_SIZING || msg == WM_MOVING)
	{
		log("WM_SIZING or MOVING");
		InvalidateRect(hwnd, NULL, false);
		iRet = DefWindowProc(hwnd, msg, wParam, lParam);
	}
	// What does this do?
	else if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
	{
		POINT p;
		POINTS* pps;

		pps = (POINTS*)&lParam;
		p.x = pps->x;
		p.y = pps->y;

		/* log("WM_MOUSEsomething\t" << p.x << "\t" << p.y); */

		ClientToBackbuffer(&p);

		pps->x = (SHORT)p.x;
		pps->y = (SHORT)p.y;

		// just a place to catch in debugging
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
		{
			pps->x = (SHORT)p.x;
		}

		if (!IsMapShowing() || !fHasFocus)
		{
			m_fRightButtonDown = false;
			m_fScrollDragging = false;
			iRet = m_pAC->pfncWinProc(hwnd, msg, wParam, lParam);
		}
		else if (msg == WM_RBUTTONDOWN)
		{
			m_fRightButtonDown = true;
			memcpy(&m_ptScrollDragPos, &p, sizeof(POINT));
		}
		else if (msg == WM_RBUTTONUP)
		{
			m_fRightButtonDown = false;
			if (m_fScrollDragging)
			{
				m_fScrollDragging = false;
				SetCursor(LoadCursor(0, IDC_ARROW));
				iRet = 0;
			}
			else
			{
				m_pAC->pfncWinProc(hwnd, WM_RBUTTONDOWN, wParam | MK_RBUTTON, lParam);
				m_pAC->pfncWinProc(hwnd, WM_RBUTTONUP, wParam, lParam);
			}
		}
		else if (m_fRightButtonDown)
			PRACXCheckScroll();
		else 
			iRet = m_pAC->pfncWinProc(hwnd, msg, wParam, lParam);
	}
	// alt-r: cycle resource viewing mode.
	else if (msg == WM_CHAR && wParam == 'r' && GetAsyncKeyState(VK_MENU) < 0)
	{
		log("WM_CHAR alt+r");
		SetResourceMode(m_iResourceMode + 1);
		iRet = 0;
	}
	// alt-t: cycle terrain viewing mode.
	else if (msg == WM_CHAR && wParam == 't' && GetAsyncKeyState(VK_MENU) < 0)
	{
		log("WM_CHAR alt+t");
		SetTerrainMode(m_iTerrainMode + 1);
		iRet = 0;
	}
	// alt-p: open on-screen PRACX menu.
	else if (msg == WM_CHAR && wParam == 'p' && GetAsyncKeyState(VK_MENU) < 0)
	{
		log("WM_CHAR alt+p");
		m_ST.Show(*m_pAC->phInstance, hwnd);
	}
	// Catch escape if the PRACX menu is open and close the menu (if PRACX
	// isn't open, escape event will fall through to somewhere else.
	else if (msg == WM_KEYDOWN && LOWORD(wParam) == VK_ESCAPE && m_ST.IsShowing())
	{
		log("WM_KEYDOWN ESC");
		m_ST.Close();
	}
	// Else send event to SMAC.
	else
		iRet = m_pAC->pfncWinProc(hwnd, msg, wParam, lParam);

	return iRet;
}

// Intercepting file open dialogue for some UI reason.
//
// PRACX doesn't listen for the WM_COMMON_DIALOG_DONE signal, as far as I know,
// so I don't know why that gets sent. Presumably, if it were important to
// SMAC, SMAC would already send the signal in pfncGetOpenFileName.
BOOL WINAPI PRACXGetOpenFileName(LPOPENFILENAME lpofn)
{
	BOOL fRet;

	m_fShowingCommDialog = true;
	fRet = pfncGetOpenFileName(lpofn);
	PostMessage(*m_pAC->phWnd, WM_COMMON_DIALOG_DONE, 0, 0);

	log(fRet);

	return fRet;
}

// See comment for PRACXGetOpenFileName.
BOOL  WINAPI PRACXGetSaveFileName(LPOPENFILENAME lpofn)
{
	BOOL fRet;

	m_fShowingCommDialog = true;
	fRet = pfncGetSaveFileName(lpofn);
	PostMessage(*m_pAC->phWnd, WM_COMMON_DIALOG_DONE, 0, 0);

	log(fRet);

	return fRet;
}

// Register PRACX to receive WINAPI events.
ATOM WINAPI PRACXRegisterClassA(WNDCLASS* pstWndClass)
{
	pstWndClass->lpfnWndProc = PRACXWinProc;
	log(pstWndClass);

	return pfncRegisterClassA(pstWndClass);
}

// Intercept BitBlt calls so we can scale them if we're windowed.
// TODO: Fix #7: Is this where scaling code would go? Probably.
BOOL WINAPI PRACXWindowBitBlt(
	_In_  HDC hdcDest,
	_In_  int nXDest,
	_In_  int nYDest,
	_In_  int nWidth,
	_In_  int nHeight,
	_In_  HDC hdcSrc,
	_In_  int nXSrc,
	_In_  int nYSrc,
	_In_  DWORD dwRop
	)
{
	int iRet;

	if (m_fWindowed)
	{
		RECT rClient;

		GetClientRect(*m_pAC->phWnd, &rClient);

		int iOld = SetStretchBltMode(hdcDest, HALFTONE);
		SetBrushOrgEx(hdcDest, 0, 0, NULL);

		iRet = StretchBlt(hdcDest, 0, 0, rClient.right, rClient.bottom,
			hdcSrc, 0, 0, m_ST.m_ptScreenSize.x, m_ST.m_ptScreenSize.y, dwRop);

		SetStretchBltMode(hdcDest, iOld);
		SetBrushOrgEx(hdcDest, 0, 0, NULL);
	}
	else
		iRet = BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);

	return iRet;
}


// Called before each draw to the screen. This scales the paintstruct if it needs to.
HDC WINAPI PRACXBeginPaint(
	_In_   HWND hwnd,
	_Out_  LPPAINTSTRUCT lpPaint
	)
{
	HDC hdc = pfncBeginPaint(hwnd, lpPaint);

	if (hdc && m_fWindowed)
	{
		memcpy(&m_rPaintSaved, &lpPaint->rcPaint, sizeof(RECT));
		if (!IsRectEmpty(&lpPaint->rcPaint))
		{
			ClientToBackbuffer((POINT*)&lpPaint->rcPaint.left);
			ClientToBackbuffer((POINT*)&lpPaint->rcPaint.right);
		}
	}

	return hdc;
}

// Unscales the paintstruct, if SMAC is windowed.
BOOL WINAPI PRACXEndPaint(
	_In_  HWND hWnd,
	_In_  const PAINTSTRUCT *lpPaint
	)
{
	if (m_fWindowed)
		memcpy((void*)&lpPaint->rcPaint, &m_rPaintSaved, sizeof(RECT));

	return pfncEndPaint(hWnd, lpPaint);
}


// TODO: Comment on how zooming works
void ZoomFactorToPt(int iZoom, POINT* pptPixelsPerTile)
{
	int i = ( 50 * (iZoom + 16) / 16 + 1 ) / 4;

	pptPixelsPerTile->x = max(i * 8, 1);
	pptPixelsPerTile->y = max(i * 4, 1);
}

int* m_iZoomFactors = NULL;
int m_iZoomZeroIndex = 0;
int m_iZoomFactorCount = 0;

void ZoomInit(void)
{
	static int s_iLastZoomInc = -1;
	static int s_iLastWidth = -1;

	log(s_iLastZoomInc << "\t" << m_ST.m_iZoomLevels << "\t" << s_iLastWidth << "\t" << *m_pAC->piMaxTileX);

	if (s_iLastZoomInc != m_ST.m_iZoomLevels || s_iLastWidth != *m_pAC->piMaxTileX)
	{
		if (m_iZoomFactors)
			delete[] m_iZoomFactors;

		m_iZoomFactors = new int[m_ST.m_iZoomLevels];

		int w = m_ST.m_ptScreenSize.x;
		int h = m_ST.m_ptScreenSize.y - 213;
		int mx = *m_pAC->piMaxTileX;
		int my = *m_pAC->piMaxTileY;
		POINT ptScale;

		int iZoomFactorMin = 1;
		do {
			iZoomFactorMin -= 1;

			ZoomFactorToPt(iZoomFactorMin, &ptScale);
		} while (iZoomFactorMin > -14 && w * 2 / ptScale.x < mx || h * 2 / ptScale.y < my);

		int iZoomFactorMax = 0;
		do {
			iZoomFactorMax += 2;

			ZoomFactorToPt(iZoomFactorMax, &ptScale);
		} while (w / ptScale.x > 6 && h / ptScale.y > 3);

		double d = (double)(iZoomFactorMax - iZoomFactorMin) / (double)(m_ST.m_iZoomLevels - 1);

		for (int i = 0; i < m_ST.m_iZoomLevels; i++)
			m_iZoomFactors[i] = Round(d * (double)i + (double)iZoomFactorMin);

		m_iZoomFactorCount = m_ST.m_iZoomLevels;

		for (int i = 1; i < m_iZoomFactorCount; i++)
		{
			if (m_iZoomFactors[i] == m_iZoomFactors[i - 1])
			{
				for (int j = i + 1; j < m_iZoomFactorCount; j++)
					m_iZoomFactors[j - 1] = m_iZoomFactors[j];
				i--;
				m_iZoomFactorCount--;
			}
		}

		if (m_iZoomFactors[0])
		{

			m_iZoomZeroIndex = min(1, m_iZoomFactorCount - 1);
			for (int i = 1; i < m_iZoomFactorCount - 1; i++)
			if (labs(m_iZoomFactors[i]) < labs(m_iZoomFactors[m_iZoomZeroIndex]))
				m_iZoomZeroIndex = i;

			m_iZoomFactors[m_iZoomZeroIndex] = 0;
		}
		else
			m_iZoomZeroIndex = 0;
		
		s_iLastZoomInc = m_ST.m_iZoomLevels;
		s_iLastWidth = *m_pAC->piMaxTileX;
	}
}

// Overrides SMAC's ZoomKeyPress stuff. Calls PRACX's zoom code.
void __stdcall PRACXZoomKeyPress(CMain* This, int iZoomType)
{
	// Don't know in what case CMain wouldn't be set...
	if (This)
	{
		log(iZoomType);
		ZoomInit();

		int iCurrent = 0;
		int d = labs(This->oMap.iZoomFactor - m_iZoomFactors[0]);

		for (int i = 1; i < m_ST.m_iZoomLevels; i++)
		{
			int d2 = labs(This->oMap.iZoomFactor - m_iZoomFactors[i]);
			if (d2 <= d)
			{
				iCurrent = i;
				d = d2;
			}
		}

		// Don't know where these magic numbers (515-520) come from.
		switch (iZoomType)
		{
		case 515:
			if (iCurrent < m_iZoomFactorCount - 1)
				iCurrent++;
			break;
		case 516:
			if (iCurrent > 0)
				iCurrent--;
			break;
		case 517:
			iCurrent = m_iZoomZeroIndex;
			break;
		case 518:
			iCurrent = m_iZoomZeroIndex;
			if(iCurrent < m_iZoomFactorCount - 1)
				iCurrent++;
			break;
		case 519:
			iCurrent = m_iZoomFactorCount - 1;
			break;
		case 520:
			iCurrent = 0;
			break;
		}

		This->oMap.iZoomFactor = m_iZoomFactors[iCurrent];
	}
}

int __stdcall PRACXZoomProcessing(CMain* This)
{
	int iRet;
	int iOldZoom;
	int w, h;
	POINT ptOldCenter;
	POINT ptNewCenter;
	static POINT ptOldTile = { -1, -1 };
	POINT ptNewTile;
	POINT ptScale;
	int dx, dy;
	bool fx, fy;

	w = ((CWinBuffed*)((int)This + (int)This->oMap.vtbl->iOffsetofoClass2))->oCanvas.stBitMapInfo.bmiHeader.biWidth;
	h = -((CWinBuffed*)((int)This + (int)This->oMap.vtbl->iOffsetofoClass2))->oCanvas.stBitMapInfo.bmiHeader.biHeight;

	if (This == m_pAC->pMain )
	{		
		iOldZoom = This->oMap.iLastZoomFactor;

		ptNewTile.x = This->oMap.iTileX;
		ptNewTile.y = This->oMap.iTileY;

		fx = (ptNewTile.x == ptOldTile.x);
		fy = (ptNewTile.y == ptOldTile.y);

		memcpy(&ptOldTile, &ptNewTile, sizeof(POINT));

		m_pAC->pfncTileToPoint(This, ptNewTile.x, ptNewTile.y, &ptOldCenter.x, &ptOldCenter.y);

		iRet = m_pAC->pfncZoomProcessing(This);

		log(iRet);

		if (m_fScrolling)
		{
			This->oMap.iMapPixelLeft = (int)m_dScrollOffsetX;
			This->oMap.iMapPixelTop = (int)m_dScrollOffsetY;
		}
		else if (iOldZoom != -9999)
		{
			ptScale.x = This->oMap.iPixelsPerTileX;
			ptScale.y = This->oMap.iPixelsPerTileY;

			m_pAC->pfncTileToPoint(This, ptNewTile.x, ptNewTile.y, &ptNewCenter.x, &ptNewCenter.y);

			dx = ptOldCenter.x - ptNewCenter.x;
			dy = ptOldCenter.y - ptNewCenter.y;

			if (!This->oMap.iMapPixelLeft && fx && dx > -ptScale.x * 2 && dx < ptScale.x * 2)
			{
				This->oMap.iMapPixelLeft = dx;
			}
			if (!This->oMap.iMapPixelTop &&
				fy &&
				dy > -ptScale.y * 2 && dy < ptScale.y * 2 &&
				( dy + h ) / ptScale.y < (*m_pAC->piMaxTileY - This->oMap.iMapTileTop) / 2)
			{
				This->oMap.iMapPixelTop = dy;
			}
		}
	}
	else
		iRet = m_pAC->pfncZoomProcessing(This);

	return iRet;
}

THISCALL_THUNK(PRACXZoomProcessing, PRACXZoomProcessing_Thunk)

// Overrides SMACDrawMap in order to fiddle with some variables if This == pMain.
//
// If This == pMain, edit map drawing related variables, then call SMAC's
// DrawMap, then restore the original variables. You'll have to look at the
// decompilation of DrawMap to understand what's going on here.
//
// Scient's decompilation possibly explains the CMap struct enough for you to
// just read that.
//
// I don't know when This would not be pMain. Perhaps for the minimap?
int __stdcall PRACXDrawMap(CMain* This, int iOwner, int fUnitsOnly)
{
	int iRet;

	if (This == m_pAC->pMain)
	{

		// Save these values to restore them later
		int iMapPixelLeft = This->oMap.iMapPixelLeft;
		int iMapPixelTop = This->oMap.iMapPixelTop;
		int iMapTileLeft = This->oMap.iMapTileLeft;
		int iMapTileTop = This->oMap.iMapTileTop;
		int iMapTilesOddX = This->oMap.iMapTilesOddX;
		int iMapTilesOddY = This->oMap.iMapTilesOddY;
		int iMapTilesEvenX = This->oMap.iMapTilesEvenX;
		int iMapTilesEvenY = This->oMap.iMapTilesEvenY;

		// These are just aliased to save typing and are not modified
		int mx = *m_pAC->piMaxTileX;
		int my = *m_pAC->piMaxTileY;

		if (iMapTilesOddX + iMapTilesEvenX < mx && !(*m_pAC->piMapFlags & 1))
		{
			if (iMapPixelLeft > 0)
			{
				This->oMap.iMapPixelLeft -= This->oMap.iPixelsPerTileX;
				This->oMap.iMapTileLeft -= 2;
				This->oMap.iMapTilesEvenX++;
				This->oMap.iMapTilesOddX++;
				if (This->oMap.iMapTileLeft < 0)
					This->oMap.iMapTileLeft += mx;
			}
			else if (iMapPixelLeft < 0 )
			{
				This->oMap.iMapTilesEvenX++;
				This->oMap.iMapTilesOddX++;
			}
		}

		if (iMapTilesOddY + iMapTilesEvenY < my)
		{
			if (iMapPixelTop > 0)
			{
				This->oMap.iMapPixelTop -= This->oMap.iPixelsPerTileY;
				This->oMap.iMapTileTop -= 2;
				This->oMap.iMapTilesEvenY++;
				This->oMap.iMapTilesOddY++;
			}
			else if (iMapPixelTop < 0)
			{
				This->oMap.iMapTilesEvenY++;
				This->oMap.iMapTilesOddY++;
			}
		}

		// Call SMAC's DrawMap function using our modified This
		iRet = m_pAC->pfncDrawMap(This, iOwner, fUnitsOnly);

		// Restore This's original values
		This->oMap.iMapPixelLeft = iMapPixelLeft;
		This->oMap.iMapPixelTop = iMapPixelTop;
		This->oMap.iMapTileLeft = iMapTileLeft;
		This->oMap.iMapTileTop = iMapTileTop;
		This->oMap.iMapTilesOddX = iMapTilesOddX;
		This->oMap.iMapTilesOddY = iMapTilesOddY;
		This->oMap.iMapTilesEvenX = iMapTilesEvenX;
		This->oMap.iMapTilesEvenY = iMapTilesEvenY;
	}
	else
		iRet = m_pAC->pfncDrawMap(This, iOwner, fUnitsOnly);

	return iRet;
}

THISCALL_THUNK(PRACXDrawMap, PRACXDrawMap_Thunk)

// Another overridden WINAPI call to enable scaling the window in windowed mode.
BOOL WINAPI PRACXInvalidateRect(
	_In_  HWND hWnd,
	_In_  const RECT *lpRect,
	_In_  BOOL bErase
	)
{
	RECT r;

	if (m_fWindowed && lpRect)
	{
		memcpy(&r, lpRect, sizeof(RECT));
		BackbufferToClient((POINT*)&r.left);
		BackbufferToClient((POINT*)&r.right);
		return pfncInvalidateRect(hWnd, &r, bErase);
	}
	else
		return pfncInvalidateRect(hWnd, lpRect, bErase);
}

// Load sprites from Icons.pcx and store them in memory.
int __stdcall PRACXLoadIcons(void)
{
	log("");
	CSprite* pSprite = m_astGrayResourceSprites;

	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			m_pAC->pfncSpriteFromCanvasRectTrans(pSprite, m_pAC->poLoadingCanvas, 0x109, 
				x * 41, y * 41 + 150, 40, 40, 0);
			pSprite++;
		}
	}

	for (int x = 0; x < 8; x++)
		m_pAC->pfncImageFromCanvas(&m_aimgFactionColors[x], m_pAC->poLoadingCanvas,
			x * 57 + 1, 429, 56, 56, 0);

	for (int x = 0; x < 3; x++)
		m_pAC->pfncImageFromCanvas(&m_aimgRaininess[x], m_pAC->poLoadingCanvas,
			x * 57 + 1, 372, 56, 56, 0);

	for (int x = 0; x < 4; x++)
		m_pAC->pfncImageFromCanvas(&m_aimgElevation[x], m_pAC->poLoadingCanvas,
			x * 57 + 229, 486, 56, 56, 0);

	for (int x = 0; x < 3; x++)
		m_pAC->pfncImageFromCanvas(&m_aimgRockiness[x], m_pAC->poLoadingCanvas,
		x * 57 + 229, 543, 56, 56, 0);

	return m_pAC->pfncCanvasDestroy4(m_pAC->poLoadingCanvas);
}

// Return 1 if we should draw city management resource yield sprites. If sprite should also be gray, set m_fGrayResources.
// See PRACXDrawCityStretchCopyToCanvas1.
int __stdcall PRACXDrawCityRes(CCity* pCity, int iTile)
{
	int iRet = 0;

	m_fGrayResources = false;

	// Don't know what this condition means.
	if (pCity->iRadius & (1 << iTile))
		iRet = 1;
	else if (m_ST.m_fShowUnworkedCityResources ^ m_fShiftShowUnworkedCityResources)
	{
		m_fGrayResources = true;
		iRet = 1;
	}

	log(pCity << "\t" << iTile << "\t" << iRet << "\t" << m_fGrayResources);

	return iRet;
}

__declspec(naked) void PRACXDrawCityRes_Thunk(void)
{
	__asm{
		PUSH ECX
		PUSH EAX
		PUSH ES
		PUSH GS

		PUSH ECX
		PUSH ESI
		CALL PRACXDrawCityRes
		MOV	 EDX, EAX

		POP  GS
		POP  ES
		POP  EAX
		POP  ECX
		RET
	}
}

// Draw resource sprites to city management window. If m_fGrayResources, draw gray sprites instead of normal ones.
void __stdcall PRACXDrawCityStretchCopyToCanvas1(
	CSprite *This, CCanvas *poCanvasDest, int cTransparentIndex, 
	int iLeft, int iTop, int iDestScale, int iSourceScale)
{
	if (m_fGrayResources)
	{
		This = (CSprite*)((UINT)This + (UINT)&m_astGrayResourceSprites[0] - (UINT)m_pAC->pSprResourceIcons);
	}

	m_pAC->pfncSpriteStretchCopyToCanvas1(This, poCanvasDest, cTransparentIndex,
		iLeft, iTop, iDestScale, iSourceScale);
}

THISCALL_THUNK(PRACXDrawCityStretchCopyToCanvas1, PRACXDrawCityStretchCopyToCanvas1_Thunk)

__declspec(naked) void PRACXDrawCityStretchCopyToCanvas_Thunk(void)
{
	_asm{
		MOV  EAX, [m_fGrayResources]
		TEST EAX, EAX
		JNZ   getout
		MOV  EAX, [m_pAC]
		ADD  EAX, offset ACADDRESSES_T.pfncSpriteStretchCopyToCanvas
		MOV  EAX, [EAX]
		JMP  EAX
	getout:
		RET 10h
	}
}

__declspec(naked) void PRACXDrawCityStretchDrawToCanvas2_Thunk(void)
{
	_asm{
		MOV  EAX, [m_fGrayResources]
		TEST EAX, EAX
		JNZ   getout
		MOV  EAX, [m_pAC]
		ADD  EAX, offset ACADDRESSES_T.pfncSpriteStretchDrawToCanvas2
		MOV  EAX, [EAX]
		JMP  EAX
	getout:
		RET 1Ch
	}
}

// PRACX likes to check if the global timer is enabled or not sometimes, so this wraps the StartTimer call or something?
_declspec(naked) void PRACXStartGlobalTimers_Thunk(void)
{
	_asm{
		MOV [m_fGlobalTimersEnabled], 1
		MOV EAX, [m_pAC]
		ADD EAX, offset ACADDRESSES_T.pfncStartTimer
		MOV EAX, [EAX]
		JMP EAX
	}
}

// PRACX likes to check if the global timer is enabled or not sometimes, so this wraps the StopTimer call or something?
_declspec(naked) void PRACXStopGlobalTimers_Thunk(void)
{
	_asm{
		MOV[m_fGlobalTimersEnabled], 0
		MOV EAX, [m_pAC]
		ADD EAX, offset ACADDRESSES_T.pfncStopTimer
		MOV EAX, [EAX]
		JMP EAX
	}
}

// Draw resource overlay
int __stdcall PRACXDrawResource(CMain* pMain, int iTileX, int iTileY, int iLeft, int iTop)
{
	int aiResCounts[3];
	int aiWidths[3];
	int iTotalWidth = 0;
	CCanvas* pCanvas;
	int iFaction;
	int iSourceScale;
	int iDestScale;
#define MAX_RES_SCALE _cx(2, 4)

	if (pMain == m_pAC->pMain && m_iResourceMode) 
	{
		pCanvas = &((CWinBuffed*)((int)pMain + (int)pMain->oMap.vtbl->iOffsetofoClass2))->oCanvas;

		iFaction = m_pAC->pMain->cOwner;

#ifdef _SMAC
		iDestScale = *m_pAC->piDestScale;
		iSourceScale = *m_pAC->piSourceScale;

#else
		iSourceScale = m_pAC->pSprResourceIcons[0].iSpriteWidth * 3;
		iDestScale =m_pAC->pMain->oMap.iPixelsPerTileX / 2;

		m_pAC->pfncTileToPoint(pMain, iTileX, iTileY, (long*)&iLeft, (long*)&iTop);
		
#endif

		CTile* pTile = &(*m_pAC->paTiles)[iTileY * *m_pAC->piTilesPerRow + iTileX / 2];

		if ((1 << iFaction) & pTile->cDiscovered)
		{

			for (int iResType = 0; iResType < 3; iResType++)
			{
				*m_pAC->piResourceExtra = 0;
				int iResCount = 0;
				bool fWithMine = false;
				bool fWithSolar = false;

				switch (iResType) {
				case 0:
					iResCount = m_pAC->pfncGetFoodCount(iFaction, -1, iTileX, iTileY, 0);
					if (m_iResourceMode == 2 &&
						(!(pTile->cTop2BitsRockiness & 0x80) &&
						(!(pTile->field_8 & 0x20) || ((int)pTile->field_0 & 0xE0) < 0x40) &&
						!(pTile->field_8 & 0xA000)))
					{
						iResCount++;
					}
					break;
				case 1:
					if (m_iResourceMode == 2 &&
						!(pTile->field_8 & 0x1002000) &&
						!(pTile->field_8 & 0x10) &&
						(!(pTile->field_8 & 0x20) || ((int)pTile->field_0 & 0xE0) < 0x40))
					{
						fWithMine = true;
					}
					iResCount = m_pAC->pfncGetProdCount(iFaction, -1, iTileX, iTileY, fWithMine);
					break;
				case 2:
					if (m_iResourceMode == 2 &&
						!(pTile->field_8 & 0x1002000) &&
						!(pTile->field_8 & 0x40) &&
						(!(pTile->field_8 & 0x20) || ((int)pTile->field_0 & 0xE0) < 0x40))
					{
						fWithSolar = true;
					}
					iResCount = m_pAC->pfncGetEnergyCount(iFaction, -1, iTileX, iTileY, fWithSolar);
					break;
				}

				iResCount += *m_pAC->piResourceExtra - 1;
				if (iResCount > 7)
					iResCount = 7;
				aiResCounts[iResType] = iResCount;

				if (iResCount > -1)
				{
					int iWidth = m_pAC->pSprResourceIcons[iResType * 8 + iResCount].iSpriteWidth;
					iWidth = iDestScale * iWidth / iSourceScale;
					aiWidths[iResType] = iWidth;
					iTotalWidth += iWidth;
				}
			}

			if (iTotalWidth)
			{
				iLeft += pMain->oMap.iPixelsPerTileX / 2 - iTotalWidth / 2;

				for (int iResType = 0; iResType < 3; iResType++)
				{
					int iResCount = aiResCounts[iResType];

					if (iResCount > -1)
					{

						m_pAC->pfncSpriteStretchCopyToCanvas1(
							&m_pAC->pSprResourceIcons[iResType * 8 + iResCount],
							pCanvas,
							m_pAC->pSprResourceIcons[iResType * 8 + iResCount].cTransparentIndex,
							iLeft, iTop, iDestScale, iSourceScale);
						iLeft += aiWidths[iResType];
					}
				}
			}
		}
	}

	return *m_pAC->piZoomNum;
}

// Insert a call to PRACXDrawResource
_declspec(naked) void PRACXDrawResource_Thunk(void)
{
#define DR_iLeft		dword ptr [ebp+28h]
#define DR_iTop			dword ptr [ebp+2Ch]
#define DR_poCanvas		dword ptr [ebp+8]
#define DR_iTileX       dword ptr [ebp+20h]
#define DR_iTileY       dword ptr [ebp+24h]
#define DR_pMain        dword ptr [ebp+0Ch]
#define DR_iFaction     dword ptr [ebp+14h]

	_asm{
		PUSH ECX
		PUSH ES
		PUSH GS

		MOV  EAX, DR_iTop
		PUSH EAX
		MOV  EAX, DR_iLeft
		PUSH EAX
		MOV  EAX, DR_iTileY
		PUSH EAX
		MOV  EAX, DR_iTileX
		PUSH EAX
		MOV  EAX, DR_pMain
		PUSH EAX

		CALL PRACXDrawResource

		POP  GS
		POP  ES
		POP  ECX

		RET
	}
}

// Load up some member variables for use in PRACXDrawTileDraw. Don't know why
// this approach was taken here rather than making a function call in ASM as
// elsewhere.
__declspec(naked) void PRACXDrawTile_Thunk(void)
{
#define DTT_iTileX	dword ptr [ebp+20h]
#define DTT_iTileY  dword ptr [ebp+24h]
#define DTT_pMain   dword ptr [ebp+0Ch]
	__asm{
		MOV	EAX, DTT_iTileX
		MOV [m_iDrawTileX], EAX
		MOV EAX, DTT_iTileY
		MOV [m_iDrawTileY], EAX
		MOV EAX, DTT_pMain
		MOV [m_pDrawTileMain], EAX

		MOV EAX, ECX
		MOV dword ptr[EAX], 0

		RET
	}
}

// Draw terrain overlay
// Replace the tile background with an appropriate sprite if terrainmode in 1..4
//
// Looks like it probably overrides a similar SMAC call.
int __stdcall PRACXDrawTileDraw(CImage *This, CCanvas *poCanvasDest, int x, int y, int a5, int a6, int a7)
{
	CTile* pTile = &(*m_pAC->paTiles)[m_iDrawTileY * *m_pAC->piTilesPerRow + m_iDrawTileX / 2];
	int iOwner;
	int iElevation;
	int iRaininess;
	int iRockiness;

	if (m_pDrawTileMain == m_pAC->pMain)
	{
		switch (m_iTerrainMode) {
		case 1:
			iOwner = pTile->cOwner;

			if (iOwner > -1 && iOwner <= 7)
				This = &m_aimgFactionColors[iOwner];

			break;
		case 2:
			iElevation = m_pAC->pfncGetElevation(m_iDrawTileX, m_iDrawTileY);

			if (iElevation < 0)
				iElevation = 0;
			else if (iElevation > 3000)
				iElevation = 3000;

			This = &m_aimgElevation[iElevation / 1000];

			break;
		case 3:
			iRaininess = (pTile->field_0 >> 3) & 3;

			if (iRaininess > 2)
				iRaininess = 2;

			This = &m_aimgRaininess[iRaininess];

			break;

		case 4:
			iRockiness = ((UINT)pTile->cTop2BitsRockiness) >> 6;

			if (iRockiness > 2)
				iRockiness = 2;

			This = &m_aimgRockiness[iRockiness];
		}
	}

	return m_pAC->pfncImageCopytoCanvas2(This, poCanvasDest, x, y, a5, a6, a7);
}
THISCALL_THUNK(PRACXDrawTileDraw, PRACXDrawTileDraw_Thunk)

#define BMENUID_BASE		20
#define BMENUID_PRACX		( BMENUID_BASE + 1 )
#define MENUID_BASE			3000
#define MENUID_SETTINGS		( MENUID_BASE + 0  )
#define MENUID_TERRAIN		( MENUID_BASE + 1  )
#define MENUID_TERRAIN0		( MENUID_BASE + 2  )
#define MENUID_TERRAIN1		( MENUID_BASE + 3  )
#define MENUID_TERRAIN2		( MENUID_BASE + 4  )
#define MENUID_TERRAIN3		( MENUID_BASE + 5  )
#define MENUID_TERRAIN4		( MENUID_BASE + 6  )
#define MENUID_RESOURCES	( MENUID_BASE + 7  )
#define MENUID_RESOURCES0	( MENUID_BASE + 8  )
#define MENUID_RESOURCES1	( MENUID_BASE + 9 )
#define MENUID_RESOURCES2	( MENUID_BASE + 10 )
#define MENUID_TOGGLE_WINDOWED (MENUID_BASE + 11 )

// Helper for setting menu values properly.
char* GetMenuCaption(int iMenuID)
{
	log(iMenuID);
	static const char* MENU_CAPTIONS[] = {
		"PRACX Preferences|Alt+P",
		"Terrain Color Modes|Alt+T",
		"    Normal Mode",
		"    Faction Owner Mode",
		"    Elevation Mode",
		"    Raininess Mode",
		"    Rockiness Mode",
		"Tile Resource Display Mode|Alt+R",
		"    Normal Mode",
		"    Current Resource Yield",
		"    Potential Resource Yield",
		"Toggle Window/Full Screen|Alt+Enter"
	};

	static char m_pszCaption[255];

	strcpy(m_pszCaption, MENU_CAPTIONS[iMenuID - MENUID_BASE]);

	if (iMenuID == m_iTerrainMode + MENUID_TERRAIN0 ||
		iMenuID == m_iResourceMode + MENUID_RESOURCES0)
		memcpy(m_pszCaption, "  *", 3);

	return m_pszCaption;
}

// Set terrain overlay mode and request redraw.
void SetTerrainMode(int iMode)
{
	iMode = iMode % 5;

	log(iMode);

	if (iMode != m_iTerrainMode)
	{
		int i = m_iTerrainMode;
		m_iTerrainMode = iMode;

		// Don't know what this does.
		m_pAC->pfncMainMenuRenameMenuItem(&m_pAC->pMain->oMainMenu, BMENUID_PRACX, i + MENUID_TERRAIN0,
			GetMenuCaption(i + MENUID_TERRAIN0));

		m_pAC->pfncMainMenuRenameMenuItem(&m_pAC->pMain->oMainMenu, BMENUID_PRACX, iMode + MENUID_TERRAIN0,
			GetMenuCaption(iMode + MENUID_TERRAIN0));


		m_pAC->pfncRedrawMap(m_pAC->pMain, 0);
		m_pAC->pfncPaintHandler(NULL, 0);
		m_pAC->pfncPaintMain(NULL);
		ValidateRect(*m_pAC->phWnd, NULL);
	}
}

// Set resource overlay mode and request redraw.
void SetResourceMode(int iMode)
{
	iMode = iMode % 3;

	log(iMode);

	if (iMode != m_iResourceMode)
	{
		int i = m_iResourceMode;
		m_iResourceMode = iMode;

		// Don't know what this does.
		m_pAC->pfncMainMenuRenameMenuItem(&m_pAC->pMain->oMainMenu, BMENUID_PRACX, i + MENUID_RESOURCES0,
			GetMenuCaption(i + MENUID_RESOURCES0));

		m_pAC->pfncMainMenuRenameMenuItem(&m_pAC->pMain->oMainMenu, BMENUID_PRACX, iMode + MENUID_RESOURCES0,
			GetMenuCaption(iMode + MENUID_RESOURCES0));
		
		m_pAC->pfncRedrawMap(m_pAC->pMain, 0);
		m_pAC->pfncPaintHandler(NULL, 0);
		m_pAC->pfncPaintMain(NULL);
		ValidateRect(*m_pAC->phWnd, NULL);
	}
}

void __cdecl PRACXMainMenuHandler(int iMenuItemId)
{
	log(iMenuItemId);
	if (iMenuItemId == MENUID_SETTINGS)
		m_ST.Show(*m_pAC->phInstance, *m_pAC->phWnd);
	else if (iMenuItemId == MENUID_TERRAIN)
		SetTerrainMode(m_iTerrainMode + 1);
	else if (iMenuItemId >= MENUID_TERRAIN0 && iMenuItemId <= MENUID_TERRAIN4)
		SetTerrainMode(iMenuItemId - MENUID_TERRAIN0);
	else if (iMenuItemId == MENUID_RESOURCES)
		SetResourceMode(m_iResourceMode + 1);
	else if (iMenuItemId >= MENUID_RESOURCES0 && iMenuItemId <= MENUID_RESOURCES2)
		SetResourceMode(iMenuItemId - MENUID_RESOURCES0);
	else if (iMenuItemId == MENUID_TOGGLE_WINDOWED)
		SetWindowed(!m_fWindowed);
	else
		m_pfncMainMenuHandler(iMenuItemId);
}

int __stdcall PRACXMainMenuUpdateVisible(CMainMenu *This, int a2)
{
	log(a2);
	char sz[255];

	m_pAC->pfncMainMenuAddBaseMenu(This, BMENUID_PRACX, strcpy(sz, "PRACX"), 1);

	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_SETTINGS, GetMenuCaption(MENUID_SETTINGS));

	m_pAC->pfncMainMenuAddSeparator(This, BMENUID_PRACX, 0);
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN, GetMenuCaption(MENUID_TERRAIN));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN0, GetMenuCaption(MENUID_TERRAIN0));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN1, GetMenuCaption(MENUID_TERRAIN1));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN2, GetMenuCaption(MENUID_TERRAIN2));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN3, GetMenuCaption(MENUID_TERRAIN3));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TERRAIN4, GetMenuCaption(MENUID_TERRAIN4));
	m_pAC->pfncMainMenuAddSeparator(This, BMENUID_PRACX, 0);
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_RESOURCES, GetMenuCaption(MENUID_RESOURCES));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_RESOURCES0, GetMenuCaption(MENUID_RESOURCES0));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_RESOURCES1, GetMenuCaption(MENUID_RESOURCES1));
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_RESOURCES2, GetMenuCaption(MENUID_RESOURCES2));
	m_pAC->pfncMainMenuAddSeparator(This, BMENUID_PRACX, 0);
	m_pAC->pfncMainMenuAddSubMenu(This, BMENUID_PRACX, MENUID_TOGGLE_WINDOWED, GetMenuCaption(MENUID_TOGGLE_WINDOWED));

	return m_pAC->pfncMainMenuUpdateVisible(This, a2);
}

THISCALL_THUNK(PRACXMainMenuUpdateVisible, PRACXMainMenuUpdateVisible_Thunk)

// Unload this library.
_declspec(naked) void PRACXFreeLib(void)
{
	__asm
	{
		mov		eax, [m_hlib]
		test	eax, eax
		jz		no_lib
		push	eax
		xor     eax, eax
		mov		[m_hlib], eax
		jmp		ds:FreeLibrary
	no_lib:
		ret
	}
}

// Overwrite bits of SMAC's running machine code to make it call out to this library, and some other miscellaneous problems.
__declspec(dllexport) void __stdcall PRACXHook(HMODULE hLib)
{
	log("");
	// If we can't load the settings from Alpha Centauri.ini, don't load this library.
	if (!m_ST.Load())
	{
		return;
	}

	// {{{ Misc. setup
	
	// Store hlib for PRACXFreeLib
	m_hlib = hLib;

	// Set up the window.
	SetVideoMode();
	SetRect(&m_rWindowedRect, 0, 0, m_ST.m_ptWindowSize.x, m_ST.m_ptWindowSize.y);

	// Zero our sprite arrays
	memset(m_astGrayResourceSprites, 0, sizeof(m_astGrayResourceSprites));
	memset(m_aimgElevation, 0, sizeof(m_aimgElevation));
	memset(m_aimgFactionColors, 0, sizeof(m_aimgFactionColors));
	memset(m_aimgRaininess, 0, sizeof(m_aimgRaininess));
	memset(m_aimgRockiness, 0, sizeof(m_aimgRockiness));

	// }}}

	// {{{ Fiddle with SMAC's memory.

	typedef struct PRACXHOOK_S {
		int   iAddress;
		void* pFunction;
	} PRACXHOOK_T;

	typedef struct PRACXCHANGE_S {
		int   iAddress;
		char* pacNewValue;
		UINT  uiNewValue;
		int   iSize;
	} PRACXCHANGE_T;

	typedef struct ACAPI_S {
		int   iAddress;
		void* pNewFunction;
		void** ppOldFunction;
	} ACAPI_T;

	// Near call/jmp overrides--calc the offset of function from Address + 4
	// and write that to Address
	PRACXHOOK_T m_astHooks[] = {
		{ _cx(0x00661799, 0x00646D79), PRACXWinMain },
		{ _cx(0x0060900C, 0x005F018C), PRACXWindowBitBlt },
		{ _cx(0x00476DFA, 0x0046A5AA), PRACXZoomProcessing_Thunk },
		{ _cx(0x004991B3, 0x0048B6C3), PRACXZoomProcessing_Thunk },
		{ _cx(0x0049934F, 0x0048B894), PRACXZoomProcessing_Thunk },
		{ _cx(0x004993E1, 0x0048B920), PRACXZoomProcessing_Thunk },
		{ _cx(0x004994BC, 0x0048BA16), PRACXZoomProcessing_Thunk },
		{ _cx(0x005291BB, 0x0051534A), PRACXZoomKeyPress },
		{ _cx(0x004773D2, 0x0046AB82), PRACXDrawMap_Thunk },
		{ _cx(0x00477422, 0x0046ABD2), PRACXDrawMap_Thunk },
		{ _cx(0x004774A7, 0x0046AC57), PRACXDrawMap_Thunk },
		{ _cx(0x00477516, 0x0046ACC6), PRACXDrawMap_Thunk },
		{ _cx(0x00407081, 0x00403BE1), PRACXShowMovie },
		{ _cx(0x00522C25, 0x0050EBB6), PRACXCheckScroll_Thunk },
		// Gray city icons
		{ _cx(0x0045FF44, 0x00454D01), PRACXLoadIcons },
		{ _cx(0x00413BB1, 0x0040F540), PRACXDrawCityRes_Thunk },
		{ _cx(0x00413F27, 0x0040F8A9), PRACXDrawCityStretchCopyToCanvas_Thunk },
		{ _cx(0x00413ECA, 0x0040F84D), PRACXDrawCityStretchCopyToCanvas1_Thunk },
		{ _cx(0x00413E92, 0x0040F81B), PRACXDrawCityStretchCopyToCanvas1_Thunk },
		{ _cx(0x00413E51, 0x0040F7DB), PRACXDrawCityStretchDrawToCanvas2_Thunk },
		// Timer tracking
		{ _cx(0x00523417, 0x0050F3E7), PRACXStartGlobalTimers_Thunk },
		{ _cx(0x00523476, 0x0050F446), PRACXStopGlobalTimers_Thunk },
		// Map drawing
//		{ _cx(0x00471069, 0x00466C33), PRACXDrawResource_Thunk },
		{ _cx(0x00473227, 0x00466C70), PRACXDrawResource_Thunk },
		{ _cx(0x0046FD2D, 0x004632FD), PRACXDrawTile_Thunk },
		{ _cx(0x00470B25, 0x00464216), PRACXDrawTileDraw_Thunk },
		{ _cx(0x00470C60, 0x00464354), PRACXDrawTileDraw_Thunk },
		// Menu Handling
		{ _cx(0x0046D669, 0x00460DB3), PRACXMainMenuUpdateVisible_Thunk }
		
	};

	// Overwrite absolute addresses and store old address
	ACAPI_T m_astApi[] = {
		{ _cx(0x0068B2D8, 0x0066929C), PRACXRegisterClassA, (void**)&pfncRegisterClassA },
		{ _cx(0x0068B2D4, 0x00669298), PRACXCreateWindowEx, (void**)&pfncCreateWindowEx },
		{ _cx(0x0068B358, 0x00669334), PRACXGetSystemMetrics, (void**)&pfncGetSystemMetrics },
		{ _cx(0x0068B2C0, 0x00669284), PRACXGetCursorPos, (void**)&pfncGetCursorPos },
		{ _cx(0x0068B2F4, 0x006692B8), PRACXBeginPaint, (void**)&pfncBeginPaint },
		{ _cx(0x0068B2F0, 0x006692B4), PRACXEndPaint, (void**)&pfncEndPaint },
		{ _cx(0x0068B318, 0x00669304), PRACXInvalidateRect, (void**)&pfncInvalidateRect },
		{ _cx(0x005328EE, 0x0051D99E), PRACXMainMenuHandler, (void**)&m_pfncMainMenuHandler },
		{ _cx(0x0068B3D0, 0x00669398), PRACXGetOpenFileName, (void**)&pfncGetOpenFileName },
		{ _cx(0x0068B3D4, 0x00669394), PRACXGetSaveFileName, (void**)&pfncGetSaveFileName }
	};
	
	// Direct memory overwrites, char* for multiple byte changes, uint for single byte changes.
	//
	// Single byte changes are special cased so that pointers to our functions can be easily
	// inserted (it's more faff to cast a function pointer to a uint, then convert to a char*).
	//
	// These cases could be covered in astApi, but:
	//   1) astApi wants to store the old address somewhere;
	//   2) I think that a semantically different thing is happening (i.e. this is replacing which
	//      function pointer is given to the windows API for winproc).
	PRACXCHANGE_T m_astChanges[] = {
		{ _cx(0x005EEA90, 0x005D5C70), NULL, (UINT)PRACXWinProc, 4 }, // Redirect WinProc
		{ _cx(0x0060900B, 0x005F018B), NULL, 0xE8, 1 }, // Redirect BitBlt for Window
		{ _cx(0x00609010, 0x005F0190), NULL, 0x90, 1 }, // NOP for BitBlt for Window
		{ _cx(0x005291B8, 0x00515347), _cx("\x55\x50\xE8", "\x53\x50\xE8"), 0, 3 }, //push ebp/ebx, push eax, call short ...
		{ _cx(0x005291BF, 0x0051534E), _cx("\xEB\x5E", "\xEB\x63"), 0, 2 }, // jmp short loc_52921F
		{ _cx(0x00407080, 0x00403BE0), NULL, 0xE9, 1 }, // jmp for PRACXShowMovie
		{ _cx(0x00522C24, 0x0050EBB5), NULL, 0xE8, 1 }, // for PRACXCheckScroll_Thunk
		{ _cx(0x00522C29, 0x0050EBBA), "\xE9\x5F\x02\x00\x00", 0, 5 }, //jmp loc_522E8D, past loop
		{ _cx(0x004D5770, 0x004BF406), NULL, 0xC3, 1 }, //ret--Kill CMovie::~CMovie, which would re-init DX
		{ _cx(0x00413BB0, 0x0040F53F), "\xE8", 0, 1 }, //call PRACXDrawCityRes_Thunk
		{ _cx(0x00413BB5, 0x0040F544), "\x85\xD2\x90\x90\x90", 0, 5 },// test edx, edx, overwriting iWorkedTile test 
		//		{ _cx(0x00471068, 0x00466C32), NULL, 0xE8, 1 },//Call to PRACXDrawResource
		{ _cx(0x00473226, 0x00466C6F), NULL, 0xE8, 1 },
		{ _cx(0x0047106D, 0x00466C74), "\x90\x90", 0, 2 },
		// Value passed to iWhatToDraw
		{ _cx(0x0052937E, 0x0051550E), NULL, 0x60010EE2, 4 }, //draw all but farms, trees, and fungus
		// Jump over fungus drawing
#ifdef _SMAC
		{ 0x0047183E, "\xF6\x45\x1C\x01", 0, 4 },  //           test    byte ptr[ebp + iWhatToDrawFlags], 1
		{ 0x00471842, "\x0F\x84\xB8\x01\x00\x00", 0, 6 }, //    jz      loc_471A00
		{ 0x00471848, "\x83\x7D\x88\x03", 0, 4 },  //           cmp[ebp + var_78], 3
		{ 0x0047184C, "\x0F\x8C\xF1\x00\x00\x00", 0, 6 }, //    jl      loc_471943
		{ 0x00471852, "\xF7\x45\xDC\x00\x00\x20\x00", 0, 7 }, // test[ebp + iField8_2], 200000h
#else
		{0x00464F26, "\xF6\x45\x1C\x01\x0F\x84\xB8\x01\x00\x00\x83\xBD\x78\xFF\xFF\xFF\x03\x0F\x8C\xF1\x00\x00\x00\xF7\x45\xE0\x00\x00\x20\x00", 0, 0x1E},
#endif
		{ _cx(0x0046D3D2, 0x00460B1C), NULL, 0, 1 }, // Set help menu to not be last
		{ _cx(0x006A6674, 0x006826F4), NULL, (UINT)PRACXFreeLib },
		// Die DDraw Die!
		{ _cx(0x0046C225, 0x0045F9D0), "\x33\xF6", 0, 2} // xor esi, esi

	};
	
	// All  the locations of constants preventing drawing of details when zoomed way out
	int m_aiZoomDetailChanges[] = {
		_cx(0x004700E3, 0x004636B1),
		_cx(0x00471965, 0x00465053),
		_cx(0x005719DC, 0x0055AF62),
		_cx(0x0056FDA0, 0x0055953E),
		_cx(0x0047264F, 0x00465DBB),
		_cx(0x0047269F, 0x00465E0B),
		_cx(0x00472728, 0x00465E94),
		_cx(0x00472756, 0x00465EC2),
		_cx(0x00472786, 0x00465EF2),
		_cx(0x004727C5, 0x00465F31),
		_cx(0x00472848, 0x00465FB4),
		_cx(0x00472890, 0x00466008),
		_cx(0x004728DB, 0x0046605F),
		_cx(0x00472904, 0x00466088),
		_cx(0x0047292D, 0x004660B1),
		_cx(0x00472992, 0x00466116),
		_cx(0x00472A43, 0x00466447)
	};

	DWORD dwOrigProtection;
	DWORD dwOrigProtection2;
	// Make SMAC memory writable and then overwrite a bunch of it.
	if (VirtualProtect((LPVOID)0x401000, _cx(0x285000, 0x263000), PAGE_EXECUTE_READWRITE, &dwOrigProtection) &&
		VirtualProtect((LPVOID)_cx(0x68B000, 0x669000), _cx(0x1B000, 0x19000), PAGE_READWRITE, &dwOrigProtection2) )
	{
		// Override API calls (absolute jumps) in SMAC by replacing the absolute address with the address of our functions.
		for (int i = 0; i < sizeof(m_astApi) / sizeof(m_astApi[0]); i++)
		{
			// Point our local definitions of the SMAC API calls (which
			// currently just == NULL), to the real addresses of the API calls
			// (in the Windows API, etc).  
			//
		    // Dereference e.g. pfncCreateWindowEx and set it as the address SMAC has.
			*m_astApi[i].ppOldFunction = (void*)*(int*)m_astApi[i].iAddress;

			// Then make SMAC call our functions for these APIs instead.
			// 
			// Interpret the address as a pointer and dereference it. i.e. write to the address iAddress.
			*(int*)m_astApi[i].iAddress = (int)m_astApi[i].pNewFunction;
		}
		
		// Override near/relative JMP calls in SMAC by calculating the offset to our functions, then replacing the JMP.
		for (int i = 0; i < sizeof(m_astHooks) / sizeof(m_astHooks[0]); i++)
		{
			int iOffset = (int)m_astHooks[i].pFunction - (m_astHooks[i].iAddress + 4);
			*(int*)m_astHooks[i].iAddress = iOffset;
		}

		// Replace arbitrary bytes in SMAC
		for (int i = 0; i < sizeof(m_astChanges) / sizeof(m_astChanges[0]); i++)
		{
			if (m_astChanges[i].pacNewValue)
				memcpy((void*)m_astChanges[i].iAddress, m_astChanges[i].pacNewValue, m_astChanges[i].iSize);
			else
				memcpy((void*)m_astChanges[i].iAddress, &m_astChanges[i].uiNewValue, m_astChanges[i].iSize);
		}
		
		// Overwrite some constants related to ZoomDistance.
		if (m_ST.m_fZoomedDetails)
		{
			for (int i = 0; i < sizeof(m_aiZoomDetailChanges) / sizeof(m_aiZoomDetailChanges[0]); i++)
			{
				UCHAR* c = (UCHAR*)m_aiZoomDetailChanges[i];
				if (*c < 0xF2)
					*c = 1;
				*c = 0xF2;
			}
		}

		// Set the memory regions back to whatever level of permission they were at originally.
		VirtualProtect((LPVOID)0x401000, _cx(0x285000, 0x263000), dwOrigProtection, &dwOrigProtection);
		VirtualProtect((LPVOID)_cx(0x68B000, 0x669000), _cx(0x1B000, 0x19000), dwOrigProtection2, &dwOrigProtection2);
	}

	// }}}
}

// Entry point for this library. If the settings file says enabled, continue loading the library, else return false (SMAC will detach).
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	log(hModule << "\t" << ul_reason_for_call << "\t" << lpReserved);
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (!CSettings::IsEnabled())
			return false;
		log("Loaded");
		break;
	case DLL_PROCESS_DETACH:
		log("Unloaded");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

// vi:ts=4
