
// Import.h

typedef struct {
    HANDLE hAlloc;
    LPTSTR data;
} LISTENTRY;

typedef struct {
    HANDLE hAlloc;
    LISTENTRY * entrylist;
    DWORD dwItemsInList;
    DWORD dwItemsAllocated;
} LIST;

typedef struct _MPLAY_OPTIONS {
	BOOL bFullPath;
	BOOL bOnTop;
	BOOL bDisplayTracker;
	BOOL bHide;
} MPLAY_OPTIONS;

extern MPLAY_OPTIONS oOptions;


#define GetListLength(List) \
	((List)->dwItemsInList)

#define GetListItem(List, Item) \
	((Item) >= 0 && (Item)<GetListLength(List))? \
	((List)->entrylist[Item].data): \
	NULL

#define GetListItemU(List, Item) \
	((Item)<GetListLength(List))? \
	((List)->entrylist[Item].data): \
	NULL

#ifndef APIENTRY
#define APIENTRY PASCAL
#endif

#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 4
#endif

#ifndef _stprintf_s
#define _stprintf_s _sntprintf
#endif

LRESULT APIENTRY MplayPropertiesDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MplayMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LONG APIENTRY abCB(HWND hWnd, UINT message, UINT wParam, LONG lParam);

HINSTANCE GethInst();                                 //APP.CPP
HWND GethWndCli();                                    //APP.CPP
HWND GethWndMain();                                   //APP.CPP
HWND GethWndStatus();                                 //APP.CPP
HWND GethWndPtr();

//
//  Interface routines (inter.c)
//
int MplayRefreshTitle(LPTSTR szTitle);
int MplayAddTrayIcon(HWND hWnd);
int MplayDelTrayIcon(HWND hWnd);
int MplaySetTrackerPos(DWORD dwValue);
int MplaySetTrackerLength(DWORD dwValue);
int MplayGetTrackerPos();
int MplaySetOpenFile(LPTSTR szFile);
int MplaySetAudioPos(HWND hWndNotify);
int MplayUpdatePos();
int MplayPauseAudio(HWND hWnd);
int MplayResumeAudio(HWND hWnd);
int MplayFindEntry(LIST * RecentItems, LPDWORD dwFlags);
LPTSTR MplayGetPtr(LPTSTR szPath, DWORD dwTo);
BOOL MplayIsTracker();
BOOL MplayIsPlaying();
int MplayDispWindow(HWND hWnd, BOOL bDisp);
int MplaySetOnTop(HWND hWnd, BOOL bOnTop);
int MplaySetTracker(HWND hWnd, BOOL bDispTrack);
void MplayResizeCallback(HWND hWnd, WPARAM wParam, LPARAM lParam);
void MplayTrayCallback(HWND hWnd, WPARAM wParam, LPARAM lParam, MPLAY_OPTIONS * oOptions);
int MplayUpdatePauseStatus(HWND hWnd, BOOL bResumable);
int MplayGetaFile(LPTSTR szFile, BOOL Save);

int MplayListUiGetActive(HWND hWnd, BOOL EnsureSelected);
int MplayListUiSetActive(HWND hWnd, DWORD dwNewItem);
int MplayListUiRefresh(HWND hWnd, LIST * List, BOOL bFullPath);
int MplayListUiRefreshAppend(HWND hWnd, LIST * List, BOOL bFullPath, DWORD dwStartItem);
int MplayListUiSwapItems(HWND hWnd, LIST * List, DWORD dwSrcItem, DWORD dwDestItem, BOOL bFullPath);
int MplayListUiDeleteItem(HWND hWnd, DWORD dwItem);

//
//  Media routines (media.c)
//
int MplayResetHandlers();
int MplayAddHandler(LPCTSTR szExt, LPCTSTR szHandler);
LPCTSTR MplayGetHandler(LPCTSTR szExt);
LPTSTR MplayEnumHandlerExtensions(LPTSTR szBuffer, DWORD cchBufferLength, LPCTSTR szSep);
int MplayCloseStream(HWND hWnd);
int MplayPlayMediaFile(HWND hWndNotify, LIST * List, LONG dwNewIndex);
int MplayGetVolume(DWORD * dwVol);
int MplaySetVolume(DWORD dwVol);


//
//  List manipulation routines (plgui.c)
//
int MplayClearList(LIST * List);
int MplayFreeList(LIST * List);
int MplayInitList(LIST * List);
int MplayInputFile(LIST * List, LPTSTR szFile);
int MplayShuffleList(LIST * List);
int MplaySortList(LIST * List, int iCriteria);
int MplayWriteList(LIST * List, LPTSTR szFile);
int MplayInsertListItem(LIST * List, LPTSTR szItem);
int MplayGenerateTree(LIST * List, LPTSTR szPath);
int MplayFindNextText(LIST * List, DWORD dwStart, LPTSTR szText, DWORD dwFlags);
int MplaySwapListItems(LIST * List, DWORD dwItem1, DWORD dwItem2);
int MplayDeleteListItem(LIST * List, DWORD dwItem);

#ifdef SCRIPTING
//
//  Scripting routines (script.c)
//
int MplayPlayScript(LIST * List, LPTSTR szFile);
#endif 

//
//  Platform abstraction routines (abstract.c)
//
#define pi_find WIN32_FIND_DATA
#define HINSTANCE_T HINSTANCE
#define FA_SUBDIR FILE_ATTRIBUTE_DIRECTORY
#define INVALID_FIND_VALUE INVALID_HANDLE_VALUE
HANDLE MplayFindFirst(LPCTSTR szFile, pi_find * fileinfo);
HANDLE MplayFindNext(HANDLE hFind, pi_find * fileinfo);
BOOL   MplayFindClose(HANDLE hFind);

unsigned int MplayGetAttr(pi_find *);
LPTSTR       MplayGetName(pi_find *);


int MplayCreateAppWindows(HINSTANCE_T hInstance, int nCmdShow);

#define NOTIFYICON_ID 2000
#define TRAY_CALLBACK  (WM_USER+0x0200)
#define C_WM_CMDLINE   (WM_USER+0x0201)
#define C_WM_REALBEGIN (WM_USER+0x0202)

#define IDM_NEW                         101
#define IDM_OPEN                        102
#define IDM_SAVE                        103
#define IDM_SAVEAS                      104
#define IDM_UPDATE_CURRENT              1111
#define IDM_UPDATE_STABLE               1121
#define IDM_UPDATE_DAILY                1122
#define IDM_UPDATE_SOURCE               1131
#define IDM_EXIT                        121
#define IDM_ABOUT                       131
#define IDM_DEBUG                       141

#define IDM_PLAY                        201
#define IDM_STOP                        202
#define IDM_PAUSE                       203
#define IDM_RESUME                      204
#define IDM_NEXT                        2111
#define IDM_PREVIOUS                    2112
#define IDM_FIRST                       2113
#define IDM_LAST                        2114
#define IDM_REP_NEXTTRACK               2121
#define IDM_REP_THISTRACK               2122
#define IDM_REP_NOTRACK                 2123
#define IDM_VOL_UP                      2211
#define IDM_VOL_DOWN                    2212



#define IDM_SORTDISPLAY                 3011
#define IDM_SORTFILE                    3012
#define IDM_SORTPATH                    3013
#define IDM_SHUFFLE                     3014
#define IDM_MOVEUP                      3021
#define IDM_MOVEDOWN                    3022
#define IDM_DELETE                      3023
#define IDM_FIND                        311
#define IDM_FINDNEXT                    312
#define IDM_FINDPREV                    313
#define IDM_STRIP                       314
#define IDM_DISPPATH                    3211
#define IDM_ONTOP                       3212
#define IDM_DISPTRACK                   3213
#define IDM_SAVEOPTIONS                 3221
#define IDM_REFRESH                     331
#define IDM_IMPORTFILE                  332
#ifdef INTERNAL
#define IDM_PROPERTIES                  341
#endif

#define IDC_FINDTEXT                    5001
#define IDC_STARTTOP                    5010
#define IDC_MATCHCASE                   5011
#define IDC_SEARCHPATH                  5012
#define IDC_INVERT                      5013

#define IDC_TRACKNAME                   5101
#define IDC_ARTIST                      5102
#define IDC_SOURCE                      5103
#define IDC_SOURCETYPE                  5104
#define IDC_TRACK                       5105

#define IDI_ICOMAIN                     7000
#define IDI_ICOBLACK                    7001
#define IDI_ICOWHITE                    7002

#define TIMER_ID 1
#define TIMER_TICK 1000

#define SORT_PATH 0
#define SORT_FILE 1
#define SORT_DISPLAY 2

#define PLAY_NEXT -1
#define PLAY_PREV -2
#define PLAY_SAME -3

#define FIND_STARTTOP   0x00000001
#define FIND_MATCHCASE  0x00000002
#define FIND_INVERT     0x00000004
#define FIND_BACKWARDS  0x00000008
#define FIND_RECENTMASK 0x00000002
#define FIND_SEARCHMASK 0xffff0000
#define FIND_SEARCHDIV  0x00010000
#define FIND_SEARCHPATH 0x00000000
#define FIND_SEARCHFILE 0x00010000
#define FIND_SEARCHDISP 0x00020000

#define REPEAT_NEXTTRACK 0x00
#define REPEAT_THISTRACK 0x01
#define REPEAT_NOTRACK   0x02


#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define MPLAY_ERROR_SUCCESS                  0
#define MPLAY_ERROR_OVERFLOW                 1
#define MPLAY_ERROR_ALREADY_EXISTS           2
#define MPLAY_ERROR_NO_MEDIA                 3
#define MPLAY_ERROR_TRACK_OUT_OF_RANGE       4
#define MPLAY_ERROR_NO_MEMORY                5
#define MPLAY_ERROR_UNKNOWN_FAILURE          6
#define MPLAY_ERROR_NOT_SUPPORTED            7
#define MPLAY_ERROR_USER_CANCEL              8
#define MPLAY_ERROR_BAD_PATH                 9
#define MPLAY_ERROR_REGISTRY                 10

// ERROR CODES >1000 are MCI error codes

#define MCI_ERROR_OFFSET 1000

// This cast should be safe on Win16 because MCI errors aren't numbered that high
#define MCI_ERROR_TO_MPLAY_ERROR(x) \
	(int)(x+MCI_ERROR_OFFSET)

#define MPLAY_ERROR_TO_MCI_ERROR(x) \
	(x-MCI_ERROR_OFFSET)

#define MPLAY_WINVER_MAJOR(x) \
	(LOBYTE(LOWORD(x)))

#define MPLAY_WINVER_MINOR(x) \
	(LOBYTE(LOWORD(x)))

#define MPLAY_REGKEY_ONTOP            _T("Always on top")
#define MPLAY_REGKEY_FULLPATH         _T("Display paths")
#define MPLAY_REGKEY_DISPLAYTRACKER   _T("Display tracker")
#define MPLAY_REGKEY_WINPOSLEFT       _T("Window position left")
#define MPLAY_REGKEY_WINPOSTOP        _T("Window position top")
#define MPLAY_REGKEY_WINPOSWIDTH      _T("Window position width")
#define MPLAY_REGKEY_WINPOSHEIGHT     _T("Window position height")
#define MPLAY_REGKEY_WINPOSHIDE       _T("Window hide")


