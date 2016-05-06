#define APPVERSION "1.06"
#define	APPNAME		"PRACX"

#define DEFAULT_LIST_SCROLL_DELTA		1
#define DEFAULT_ZOOM_LEVELS				6
#define DEFAULT_SCROLL_AREA				40
#define DEFAULT_SCROLL_MIN				1
#define DEFAULT_SCROLL_MAX				20
#define DEFAULT_ZOOMED_DETAILS			1
#define DEFAULT_MOUSE_OVER_TILE_INFO	1
#define DEFAULT_SHOW_UNWORKED			1

class CSettings {
public:
	POINT m_ptScreenSize;
	POINT m_ptWindowSize;
	int m_iListScrollDelta = DEFAULT_LIST_SCROLL_DELTA;
	int m_iZoomLevels = DEFAULT_ZOOM_LEVELS;
	int m_iScrollArea = DEFAULT_SCROLL_AREA;
	int m_fZoomedDetails = DEFAULT_ZOOMED_DETAILS;
	int m_fMouseOverTileInfo = DEFAULT_MOUSE_OVER_TILE_INFO;
	int m_fShowUnworkedCityResources = DEFAULT_SHOW_UNWORKED;
	int m_iScrollMin = DEFAULT_SCROLL_MIN;
	int m_iScrollMax = DEFAULT_SCROLL_MAX;
	int m_fDisabled = false;

	POINT m_ptDefaultScreenSize;
	POINT m_ptDefaultWindowSize;
	
	POINT m_ptNewScreenSize;

	void* m_pWin = NULL;

	bool Load();
	void Save();
	void Show(HINSTANCE hInstance, HWND hwndParent);
	bool IsShowing();
	bool IsMyWindow(HWND hwnd);
	void Close();
	~CSettings();
	
	static bool IsEnabled();

private:
	static int ReadIniInt(char* pszKey, int iDefault = 0, int iMax = 0, int iMin = 0);
	static void WriteIniInt(char* pszKey, int iValue, int iDefault = 0);
};
