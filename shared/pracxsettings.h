#include <string>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

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

using namespace std;

// Logs
//
// https://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing#1644898
		/* fprintf(logfile, "%s:%d:%s(): " fmt, __FILE__, \ */
		/* 		__LINE__, __func__, __VA_ARGS__);\ */
#define DEBUG 1
#define log(msg) \
	do { if (DEBUG) {\
		ofstream logfile;\
		logfile.open("pracx.log", ios::app);\
		logfile << GetTickCount() << ":" << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\t" << msg << endl;\
		logfile.close();\
	} } while (0)

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

	string m_szMoviePlayerCommand = string(".\\movies\\playuv15.exe -software");

	bool Load();
	void Save();
	void Show(HINSTANCE hInstance, HWND hwndParent);
	bool IsShowing();
	bool IsMyWindow(HWND hwnd);
	void Close();
	~CSettings();
	
	static bool IsEnabled();

private:
	const string DEFAULT_MOVIE_PLAYER_COMMAND = string(".\\movies\\playuv15.exe -software");
	static int ReadIniInt(char* pszKey, int iDefault = 0, int iMax = 0, int iMin = 0);
	static void WriteIniInt(char* pszKey, int iValue, int iDefault = 0);
	static void WriteIniString(char* pszKey, string value, string defaultvalue);
	static string ReadIniString(char* pszKey, string szDefault);
};
