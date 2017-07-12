/*
 * pracxsettings.cpp
 *
 * Three things in one:
 * 	A namespace for pracx's settings variables
 * 	Definition of the in-game UI overlay that controls these settings
 * 	Functions to read and write these settings from and to Alpha Centauri.ini.
 *
 */

#include "pracxsettings.h"
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <math.h>
#include <commctrl.h>
#include <vector>

// TODO: PR's usage of std:: is inconsistant. Sometimes it's used when it's not needed in this file.
using namespace std;

// {{{ In-game options screen

enum SETTING_CONTROLS_E {
	CB_RESOLUTION = 101,
	TRK_ZOOMLEVELS,
	TRK_SCROLLMIN,
	TRK_SCROLLMAX,
	TRK_SCROLLAREA,
	CHK_FREELOOK,
	CHK_UNWORKED,
	TRK_LISTDELTA,
	CHK_ZOOMDETAILS
};

class CControl {
public:
	int  m_iID;
	HWND m_hwndParent;
	HWND m_hwnd;

	CControl(HWND hwndParent, int iID, char* pszToolTip = NULL){ Initialize();  m_hwndParent = hwndParent; m_iID = iID; m_pszToolTip = pszToolTip; }
	bool virtual ProcMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult){ return false; }
	void virtual OnOK(){}
protected:
	char* m_pszToolTip;

	void AddToolTip(int iLeft = 0, int iTop = 0, int iWidth = 0, int iHeight = 0)
	{
		if (m_pszToolTip && *m_pszToolTip)
		{
			HWND hwndTool = CreateWindow(TOOLTIPS_CLASS, "", WS_POPUP | TTS_ALWAYSTIP, // | TTS_BALLOON,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				m_hwndParent, NULL, NULL, NULL);

			TOOLINFO ti = { 0 };

			ti.cbSize = sizeof(ti);
			ti.hwnd = m_hwndParent;
			ti.uId = NULL;
			ti.lpszText = m_pszToolTip;
			ti.uFlags = TTF_SUBCLASS;
			if (iWidth)
			{
				ti.rect.left = iLeft;
				ti.rect.right = iLeft + iWidth;
				ti.rect.top = iTop;
				ti.rect.bottom = iTop + iHeight;
			}
			else
			{
				ti.uFlags |= TTF_IDISHWND;
				ti.uId = (UINT_PTR)m_hwnd;
			}

			SendMessage(hwndTool, TTM_ADDTOOL, 0, (LPARAM)&ti);
			SendMessage(hwndTool, TTM_SETMAXTIPWIDTH, 0, 400);
			SendMessage(hwndTool, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30 * 1000);
		}
	}
private:
	static bool m_fInitialized;

	static void Initialize()
	{
		if (!m_fInitialized)
		{
			INITCOMMONCONTROLSEX icex;
			HMODULE hLib = LoadLibrary("Comctl32.lib");
			BOOL(__stdcall *pInitCommonControlsEx)(LPINITCOMMONCONTROLSEX) =
				(BOOL(__stdcall *)(LPINITCOMMONCONTROLSEX))GetProcAddress(hLib, "InitCommonControlsEx");

			if (pInitCommonControlsEx)
			{
				icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
				icex.dwICC = ICC_BAR_CLASSES;
				pInitCommonControlsEx(&icex);
			}

			m_fInitialized = true;
		}
	}
};
bool CControl::m_fInitialized = false;

class CLabel : public CControl {
public:
	CLabel(HWND hwndParent, int iID, char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, UINT uiStyle = SS_LEFTNOWORDWRAP, char* pszToolTip = NULL)
		: CControl(hwndParent, iID, pszToolTip)
	{
		m_hwnd = CreateWindow("STATIC", pszCaption, WS_CHILD | WS_VISIBLE | uiStyle,
			iLeft, iTop, iWidth, iHeight, hwndParent, (HMENU)iID, NULL, NULL);
		AddToolTip(iLeft, iTop, iWidth, iHeight);
	}
	void SetText(char* pszText){ SetWindowText(m_hwnd, pszText); }
};

// A horizontal scroll bar to select values.
class CTrackbar : public CControl {
public:
	CTrackbar(HWND hwndParent, int iID, char* pszToolTip) : CControl(hwndParent, iID, pszToolTip){};
	CTrackbar(HWND hwndParent, int iID, char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, int iMin, int iMax, int* piValue, char* pszToolTip = NULL)
		: CControl(hwndParent, iID, pszToolTip)
	{
		m_piValue = piValue;

		Initialize(pszCaption, iLeft, iTop, iWidth, iHeight, iMin, iMax, *piValue, 44);
	}
	~CTrackbar(){ if (m_plblValue) delete m_plblValue; if (m_plblCaption) delete m_plblCaption; }
	bool virtual ProcMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
	{
		bool fResult = CControl::ProcMessage(hwnd, msg, wParam, lParam, plResult);

		if (msg == WM_HSCROLL && (HWND)lParam == m_hwnd)
		{
			LRESULT pos = -1;

			if (m_fUseTrackbar)
				pos = SendMessageW(m_hwnd, TBM_GETPOS, 0, 0);
			else
			{
				pos = GetScrollPos(m_hwnd, SB_CTL);
				switch (LOWORD(wParam)){
					case SB_LINELEFT:	pos = max(pos - 1, m_iMin); break;
					case SB_LINERIGHT:	pos = min(pos + 1, m_iMax); break;
					case SB_PAGELEFT:	pos = max(pos - ((m_iMax - m_iMin) / 5 + 1), m_iMin); break;
					case SB_PAGERIGHT:	pos = min(pos + ((m_iMax - m_iMin) / 5 + 1), m_iMax); break;
					case SB_LEFT:		pos = m_iMin; break;
					case SB_RIGHT:		pos = m_iMax; break;
					case SB_THUMBPOSITION:
					case SB_THUMBTRACK: pos = HIWORD(wParam); break;
					default: return fResult;
				}
				SetScrollPos(m_hwnd, SB_CTL, pos, TRUE);
			}

			char szValue[255];
			ValToStr(pos, szValue);
			m_plblValue->SetText(szValue);
			*plResult = 0;
			fResult = true;
		}

		return fResult;
	}
	void virtual OnOK(){ *m_piValue = (m_fUseTrackbar) ? SendMessage(m_hwnd, TBM_GETPOS, 0, 0) : GetScrollPos(m_hwnd, SB_CTL); }
protected:
	int *m_piValue = NULL;
	CLabel* m_plblCaption = NULL;
	CLabel* m_plblValue = NULL;
	long int m_iMin;
	long int m_iMax;

	void virtual ValToStr(int iValue, char* pszValue){ sprintf(pszValue, "%d", iValue); }
	void Initialize(char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, int iMin, int iMax, int iValue, int iValueWidth)
	{
		char szValue[255];

		m_iMin = iMin;
		m_iMax = iMax;

		ValToStr(iValue, szValue);

		m_plblCaption = new CLabel(m_hwndParent, m_iID * 16 + 5000, pszCaption, iLeft, iTop, 112, iHeight, SS_RIGHT, m_pszToolTip);

		iLeft += 120;
		iWidth -= 120;

		iWidth -= 120;
		m_plblValue = new CLabel(m_hwndParent, m_iID * 16 + 5001, szValue, iLeft + iWidth + 8, iTop, iValueWidth, iHeight, SS_LEFT, m_pszToolTip);

		if (m_fUseTrackbar)
		{
			m_hwnd = CreateWindow(TRACKBAR_CLASS, "", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_BOTTOM,
				iLeft, iTop, iWidth, iHeight, m_hwndParent, (HMENU)m_iID, NULL, NULL);

			SendMessage(m_hwnd, TBM_SETBUDDY, (WPARAM)TRUE, (LPARAM)m_plblCaption->m_hwnd);
			SendMessage(m_hwnd, TBM_SETBUDDY, (WPARAM)FALSE, (LPARAM)m_plblValue->m_hwnd);

			int iTick;
			int iRange = iMax - iMin;

			if (iRange <= 10)
				iTick = 1;
			else if (iRange <= 25)
				iTick = 5;
			else if (iRange <= 100)
				iTick = 10;
			else
				iTick = 25;

			SendMessageW(m_hwnd, TBM_SETRANGE, TRUE, MAKELONG(iMin, iMax));
			SendMessageW(m_hwnd, TBM_SETPAGESIZE, 0, iTick);
			SendMessageW(m_hwnd, TBM_SETTICFREQ, iTick, 0);
			SendMessageW(m_hwnd, TBM_SETPOS, FALSE, iValue);
		}
		else
		{
			m_hwnd = CreateWindow( "SCROLLBAR", "", WS_CHILD | WS_VISIBLE | SBS_HORZ,
				iLeft, iTop, iWidth, iHeight, m_hwndParent, (HMENU)m_iID, NULL, NULL);

			SetScrollRange(m_hwnd, SB_CTL, iMin, iMax, false);
			SetScrollPos(m_hwnd, SB_CTL, iValue, true);
		}

		AddToolTip();
	}

protected:
	const bool m_fUseTrackbar = false;
	HWND m_hwndValue;
};

// Horizontal scroll bar to select resolutions.
//
// Uses EnumDisplaySettings to get options.
class CTrackResolution : public CTrackbar {
public:
	CTrackResolution(HWND hwndParent, int iID, char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, POINT* pptValue, char* pszToolTip = NULL)
		: CTrackbar(hwndParent, iID, pszToolTip)
	{
		log(hwndParent << "\t" << iID << "\t" << pszCaption << "\t" << iLeft << "\t" << iTop << "\t" << iWidth << "\t" << iHeight << "\t" << pptValue << "\t" << pszToolTip);

		int iValue;
		DEVMODE dm = { 0 };
		POINT p = { 0 };

		m_piValue = (int*)pptValue;

		dm.dmSize = sizeof(dm);

		for (int iModeNum = 0, m_iResolutionCount = 0;
			EnumDisplaySettings(NULL, iModeNum, &dm);
			iModeNum++)
		{
			if (dm.dmBitsPerPel == 32 && dm.dmPelsWidth >= 1024 && dm.dmPelsHeight >= 768 &&
				(dm.dmPelsWidth != p.x || dm.dmPelsHeight != p.y))
			{
				p.x = dm.dmPelsWidth;
				p.y = dm.dmPelsHeight;
				m_vResolutions.push_back(p);
			}
		}

		if (pptValue->x)
		for (iValue = 0; iValue < (int)m_vResolutions.size() && 0 != memcmp(&m_vResolutions[iValue], pptValue, sizeof(POINT)); iValue++);

		if (!pptValue->x || iValue >= (int)m_vResolutions.size())
		{
			EnumDisplaySettings(NULL, ENUM_REGISTRY_SETTINGS, &dm);

			for (iValue = 0;
				iValue < (int)m_vResolutions.size() && (dm.dmPelsWidth != m_vResolutions[iValue].x || dm.dmPelsHeight != m_vResolutions[iValue].y);
				iValue++);

			if (iValue > (int)m_vResolutions.size())
			{
				POINT p;
				p.x = dm.dmPelsWidth;
				p.y = dm.dmPelsHeight;
				m_vResolutions.push_back(p);
			}
		}

		Initialize(pszCaption, iLeft, iTop, iWidth, iHeight, 0, m_vResolutions.size() - 1, iValue, 112);
	}
	void virtual OnOK(){ *((POINT*)m_piValue) = m_vResolutions[(m_fUseTrackbar) ? SendMessage(m_hwnd, TBM_GETPOS, 0, 0) : GetScrollPos(m_hwnd, SB_CTL)]; };
protected:
	void virtual ValToStr(int iValue, char* pszValue){ sprintf(pszValue, "%dx%d", m_vResolutions[iValue].x, m_vResolutions[iValue].y); };
private:
	vector<POINT> m_vResolutions;
};

class CCheckbox : public CControl {
public:
	CCheckbox(HWND hwndParent, int iID, char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, int* piValue, char* pszToolTip = NULL)
		: CControl(hwndParent, iID, pszToolTip)
	{
		m_piValue = piValue;

		m_hwnd = CreateWindow("button", pszCaption, WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_FLAT,
			iLeft + 20, iTop, iWidth - 20, iHeight, hwndParent, (HMENU)iID, NULL, NULL);
		AddToolTip();

		if (*piValue)
			SendMessage(m_hwnd, BM_SETCHECK, BST_CHECKED, 0);
	}
	bool virtual ProcMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
	{
		bool fRet = CControl::ProcMessage(hwnd, msg, wParam, lParam, plResult);

		if (msg == WM_COMMAND && LOWORD(wParam) == m_iID)
		{
			SendMessage(m_hwnd, BM_SETCHECK,
				(BST_CHECKED != SendMessage(m_hwnd, BM_GETCHECK, 0, 0)) ? BST_CHECKED : BST_UNCHECKED, 0);
			*plResult = 0;
			fRet = true;
		}

		return fRet;
	}
	void virtual OnOK(){ *m_piValue = (BST_CHECKED == SendMessage(m_hwnd, BM_GETCHECK, 0, 0)); }
protected:
	int* m_piValue;
};

class CButton : public CControl {
public:
	CButton(HWND hwndParent, int iID, char* pszCaption, int iLeft, int iTop, int iWidth, int iHeight, char* pszToolTip = NULL) 
		: CControl(hwndParent, iID, pszToolTip)
	{
		m_hwnd = CreateWindow("button", pszCaption, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
			iLeft, iTop, iWidth, iHeight, hwndParent, (HMENU)iID, NULL, NULL);
		AddToolTip();
	}
};

#define OK_ID				1
#define CANCEL_ID			2

class CSettingsWnd {
public:
	HWND m_hwnd = 0;
	CSettings* m_pSettings = NULL;

	CSettingsWnd(HINSTANCE hInstance, HWND hwndParent, CSettings* pSettings)
	{
		pSettings->m_pWin = this;

		m_pSettings = pSettings;
		m_hInstance = hInstance;
		m_hwndParent = hwndParent;

		Register();

		RECT r;
		GetClientRect(hwndParent, &r);

		m_hwnd = CreateWindow(m_pszClassName, APPNAME " Settings",	WS_CHILD | WS_VISIBLE,
			r.left + (r.right - r.left - WIDTH) / 2, r.top + (r.bottom - r.top - HEIGHT) / 2, WIDTH, HEIGHT, 
			hwndParent, (HMENU)51, hInstance, this);
	}
	~CSettingsWnd()
	{
		for (std::vector<CControl*>::iterator c = m_vpControls.begin(); c != m_vpControls.end(); c++)
			delete *c;

		if (m_pSettings)
			m_pSettings->m_pWin = NULL;
	}
private:
	static const COLORREF BACKGROUND = RGB(17, 26, 36); //RGB(6, 12, 19);
	static const COLORREF FOREGROUND = RGB(120, 164, 212);
	static const COLORREF BORDER = RGB(73, 108, 61);

	static const int WIDTH = 640;
	static const int HEIGHT = 280;

	static HBRUSH m_hBrush;
	static bool m_fRegistered;
	static HINSTANCE m_hInstance;
	static char* m_pszClassName;
	
	std::vector<CControl*> m_vpControls;
	HWND m_hwndParent = 0;

	static void Register()
	{
		if (!m_fRegistered)
		{
			WNDCLASS wc = { 0 };

			m_hBrush = CreateSolidBrush(BACKGROUND);
			wc.hbrBackground = m_hBrush;
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpszClassName = m_pszClassName;
			wc.hInstance = m_hInstance;
			wc.lpfnWndProc = WndProc_Thunk;

			RegisterClass(&wc);

			m_fRegistered = true;
		}
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRet = 0;

		UINT checked = false;

		switch (msg)
		{
		case WM_CREATE:
			OnCreate(hwnd);
			break;
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORSCROLLBAR:
			SetBkColor((HDC)wParam, BACKGROUND);
			SetTextColor((HDC)wParam, FOREGROUND);
			return (LRESULT)m_hBrush;
		case WM_KEYDOWN:
			if (LOWORD(wParam) == VK_ESCAPE)
			{
				DestroyWindow(hwnd);
				return 0;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
			case OK_ID:
				if (m_pSettings)
				{
					for (std::vector<CControl*>::iterator c = m_vpControls.begin(); c != m_vpControls.end(); c++)
						(*c)->OnOK();
					m_pSettings->Save();
				}
			case CANCEL_ID:
				DestroyWindow(hwnd);
				return 0;
			}

			break;
		case WM_ERASEBKGND:
			RECT r;
			HDC hdc = (HDC)wParam;
			GetClientRect(hwnd, &r);
			FillRect(hdc, &r, m_hBrush);

			HPEN hPen = CreatePen(PS_INSIDEFRAME, 1, BORDER);
			HGDIOBJ hOld = SelectObject(hdc, hPen);
			SelectObject(hdc, m_hBrush);

			RoundRect(hdc, r.left, r.top, r.right, r.bottom, 8, 8);
			InflateRect(&r, -3, -3);
			RoundRect(hdc, r.left, r.top, r.right, r.bottom, 8, 8);
			
			SelectObject(hdc, hOld);
			DeleteObject(hPen);

			return 0;
		}

		for (std::vector<CControl*>::iterator c = m_vpControls.begin(); c != m_vpControls.end(); c++)
		{
			if ((*c)->ProcMessage(hwnd, msg, wParam, lParam, &lRet))
				return lRet;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	static LRESULT CALLBACK WndProc_Thunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		CSettingsWnd* pSelf;

		if (msg == WM_CREATE)
		{
			pSelf = (CSettingsWnd*)((LPCREATESTRUCT)lParam)->lpCreateParams;
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)pSelf);
		}
		else
			pSelf = (CSettingsWnd*)GetWindowLong(hwnd, GWLP_USERDATA);

		if (pSelf)
		{
			if (msg == WM_DESTROY)
			{
				SetWindowLong(hwnd, GWL_USERDATA, (LONG)NULL);
				delete pSelf;
			}
			else
				return pSelf->WndProc(hwnd, msg, wParam, lParam);
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

#define MKPOS(x,y) 8+x*342, 8+y*28, 350, 20

	void OnCreate(HWND hwnd)
	{
		m_hwnd = hwnd;

		m_vpControls.push_back(new CLabel(hwnd, 0, APPNAME "  v" APPVERSION " Preferences", (WIDTH - 200) / 2, 8, 200, 20, SS_CENTER));

		m_vpControls.push_back(new CTrackResolution(hwnd, 3, "Resolution", MKPOS(0, 2), &m_pSettings->m_ptNewScreenSize, 
			"Full screen resolution.  Change requires restart to take effect."));
		m_vpControls.push_back(new CTrackbar(hwnd, 4, "Zoom Levels", MKPOS(0, 3), 2, 20, &m_pSettings->m_iZoomLevels,
			"# of increments between fully zoomed in and fully zoomed out."));
		m_vpControls.push_back(new CTrackbar(hwnd, 5, "Wheel Scroll", MKPOS(0, 4), 0, 10, &m_pSettings->m_iListScrollDelta,
			"Mouse wheel text scrolling lines per tick."));
		m_vpControls.push_back(new CTrackbar(hwnd, 6, "Scroll Min", MKPOS(1, 2), 0, 50, &m_pSettings->m_iScrollMin,
			"Minimum edge scrolling speed in Tiles Per Second."));
		m_vpControls.push_back(new CTrackbar(hwnd, 7, "Scroll Max", MKPOS(1, 3), 0, 50, &m_pSettings->m_iScrollMax,
			"Maximum edge scrolling speed in Tiles Per Second."));
		m_vpControls.push_back(new CTrackbar(hwnd, 8, "Scroll Area", MKPOS(1, 4), 0, 300, &m_pSettings->m_iScrollArea,
			"Size of edge scrolling box in pixels."));

		
		m_vpControls.push_back(new CCheckbox(hwnd, 9, "Details When Zoomed Out", MKPOS(0, 6), &m_pSettings->m_fZoomedDetails,
			"Show full map details even when fully zoomed out.  Change requires restart to take effect."));
		m_vpControls.push_back(new CCheckbox(hwnd, 10, "Show Unworked City Resources", MKPOS(0, 7), &m_pSettings->m_fShowUnworkedCityResources,
			"Show unworked resource amounts on the city screen. Hold <SHIFT> to hide then, (or to see them if this option is diabled)."));
		m_vpControls.push_back(new CCheckbox(hwnd, 11, "Mouse Over Tile Info", 8 + 1 * 342, 8 + 6 * 28, 250, 20, &m_pSettings->m_fMouseOverTileInfo,
			"Update 'Info on Tile' without clicking when in View Mode (<V> key)."));

		m_vpControls.push_back(new CButton(hwnd, OK_ID, "OK", 16, HEIGHT - 36, WIDTH / 2 - 32, 20));
		m_vpControls.push_back(new CButton(hwnd, CANCEL_ID, "CANCEL", 16 + WIDTH / 2, HEIGHT - 36, WIDTH / 2 - 32, 20));
		
	}
};

HBRUSH CSettingsWnd::m_hBrush = NULL;
bool CSettingsWnd::m_fRegistered = false;
char* CSettingsWnd::m_pszClassName = "PRACXSettingsWnd";
HINSTANCE CSettingsWnd::m_hInstance = NULL;

void CSettings::Show(HINSTANCE hInstance, HWND hwndParent)
{
	if (!IsShowing())
		new CSettingsWnd(hInstance, hwndParent, this);
}

bool CSettings::IsShowing()
{
	return !!m_pWin;
}

bool CSettings::IsMyWindow(HWND hwnd)
{
	return (m_pWin && ((CSettingsWnd*)m_pWin)->m_hwnd == hwnd);
}

void CSettings::Close()
{
	if (m_pWin)
		DestroyWindow(((CSettingsWnd*)m_pWin)->m_hwnd);
}

CSettings::~CSettings()
{
	if (m_pWin)
		((CSettingsWnd*)m_pWin)->m_pSettings = NULL;
}

// }}}

// {{{ Manage ini file.


bool CSettings::IsEnabled()
{
	if (ReadIniInt("Disabled"))
		return false;
	else
		return true;
}

bool CSettings::Load()
{


	DEVMODE dm = { 0 };
	dm.dmSize = sizeof(dm);

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);

	m_ptDefaultScreenSize.x = dm.dmPelsWidth;
	m_ptDefaultScreenSize.y = dm.dmPelsHeight;

	m_ptScreenSize.x = ReadIniInt("ScreenWidth", m_ptDefaultScreenSize.x, m_ptDefaultScreenSize.x, 1024);
	m_ptScreenSize.y = ReadIniInt("ScreenHeight", m_ptDefaultScreenSize.y, m_ptDefaultScreenSize.y, 768);

	log(dm.dmPelsWidth << "\t" << dm.dmPelsHeight << "\t" << m_ptDefaultScreenSize.x << "\t" << m_ptDefaultScreenSize.y << "\t" << m_ptScreenSize.x << "\t" << m_ptScreenSize.y);

	m_ptNewScreenSize = m_ptScreenSize;

	m_ptDefaultWindowSize.x = m_ptScreenSize.x / 2;
	m_ptDefaultWindowSize.y = m_ptScreenSize.y / 2;

	m_ptWindowSize.x = ReadIniInt("WindowWidth", m_ptDefaultWindowSize.x, m_ptScreenSize.x * 3 / 4);
	m_ptWindowSize.y = ReadIniInt("WindowHeight", m_ptDefaultWindowSize.y, m_ptScreenSize.y * 3 / 4);

	m_iZoomLevels = ReadIniInt("ZoomLevels", m_iZoomLevels, 20, 2);

	m_iScrollMin = ReadIniInt("ScrollMin", m_iScrollMin, 50);
	m_iScrollMax = ReadIniInt("ScrollMax", m_iScrollMax, 50, m_iScrollMin);
	m_iScrollArea = ReadIniInt("ScrollArea", m_iScrollArea, 300);

	m_fMouseOverTileInfo = ReadIniInt("MouseOverTileInfo", m_fMouseOverTileInfo, 1);

	m_fShowUnworkedCityResources = ReadIniInt("ShowUnworkedCityResources", m_fShowUnworkedCityResources, 1);

	m_iListScrollDelta = ReadIniInt("ListScrollLines", m_iListScrollDelta, 50);

	m_fZoomedDetails = ReadIniInt("ZoomedOutShowDetails", m_fZoomedDetails, 1);

	m_szMoviePlayerCommand = ReadIniString("MoviePlayerCommand", m_szMoviePlayerCommand);

	return true;
}

void CSettings::Save()
{
	log("");
	WriteIniInt("ScreenWidth", m_ptNewScreenSize.x, m_ptDefaultScreenSize.x);
	WriteIniInt("ScreenHeight", m_ptNewScreenSize.y, m_ptDefaultScreenSize.y);

	WriteIniInt("ZoomLevels", m_iZoomLevels, DEFAULT_ZOOM_LEVELS);

	WriteIniInt("ScrollMin", m_iScrollMin, DEFAULT_SCROLL_MIN);
	WriteIniInt("ScrollMax", m_iScrollMax, DEFAULT_SCROLL_MAX);
	WriteIniInt("ScrollArea", m_iScrollArea, DEFAULT_SCROLL_AREA);

	WriteIniInt("MouseOverTileInfo", m_fMouseOverTileInfo, DEFAULT_MOUSE_OVER_TILE_INFO);

	WriteIniInt("ShowUnworkedCityResources", m_fShowUnworkedCityResources, DEFAULT_SHOW_UNWORKED);

	WriteIniInt("ListScrollLines", m_iListScrollDelta, DEFAULT_LIST_SCROLL_DELTA);

	WriteIniInt("ZoomedOutShowDetails", m_fZoomedDetails, DEFAULT_ZOOMED_DETAILS);

	WriteIniString("MoviePlayerCommand", m_szMoviePlayerCommand, DEFAULT_MOVIE_PLAYER_COMMAND);

}

// Read an int from AC.ini, if the key doesn't exist, write it as "<DEFAULT>" so user knows they can change it.
//
// We write "<DEFAULT>" rather than the real value so we don't bake old defaults into Ini files.
int CSettings::ReadIniInt(char* pszKey, int iDefault, int iMax, int iMin)
{
	char szValue[255] = { 0 };
	int iRet;
	char *pszEnd = NULL;
	
	GetPrivateProfileString("PRACX", pszKey, "", szValue, sizeof(szValue) - 1, ".\\Alpha Centauri.ini");
	szValue[sizeof(szValue)-1] = 0;

	iRet = strtol(szValue, &pszEnd, 10);
	if (pszEnd != szValue && !*pszEnd)
	{
		if (iMax && iRet > iMax)
			iRet = iMax;
		else if (iRet < iMin)
			iRet = iMin;
	}
	else
	{
		iRet = iDefault;
		if (!*szValue)
			WriteIniInt(pszKey, 0, 0);
	}

	return iRet;
}

// Read string from AC.ini, if the key doesn't exist, write it as "<DEFAULT>" so user knows they can change it.
//
// We write "<DEFAULT>" rather than the real value so we don't bake old defaults into Ini files.
std::string CSettings::ReadIniString(char* pszKey, std::string defaultString)
{
	char szValue[1024] = { 0 };
	
	GetPrivateProfileString("PRACX", pszKey, "", szValue, sizeof(szValue) - 1, ".\\Alpha Centauri.ini");
	szValue[sizeof(szValue)-1] = 0;

	std::string sRet(szValue);

	if (sRet == "") 
	{
		sRet = defaultString;
		WriteIniString(pszKey, sRet, sRet);
	}
	else if (sRet == "<DEFAULT>")
	{
		sRet = defaultString;
	}

	log(pszKey << "\t" << defaultString << "\t" << sRet);
	return sRet;
}

	
void CSettings::WriteIniInt(char* pszKey, int iValue, int iDefault)
{
	char szValue[255];

	if (iValue == iDefault)
		strcpy(szValue, "<DEFAULT>");
	else
		sprintf(szValue, "%d", iValue);

	log(pszKey << "\t" << iValue << "\t" << iDefault);
	WritePrivateProfileString("PRACX", pszKey, szValue, ".\\Alpha Centauri.ini");
}

void CSettings::WriteIniString(char* pszKey, std::string value, std::string defaultvalue)
{
	if (value == defaultvalue)
		value = "<DEFAULT>";

	log(pszKey << "\t" << value << "\t" << defaultvalue);
	WritePrivateProfileString("PRACX", pszKey, const_cast<char*>(value.c_str()), ".\\Alpha Centauri.ini");
}
