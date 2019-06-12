/*
 * App.cpp
 *
 * This file takes care of launching the App and processing its core messages,
 * and some interface work.
 *
 * Copyright (c) Malcolm Smith 2001-2017.  No warranty is provided.
 */

#include "pch.h"

HINSTANCE_T hInst;
int nCmdShowGlobal;
DWORD WinVer;

MPLAY_OPTIONS oOptions;

HINSTANCE_T GethInst()
{
    return hInst;
}

BOOL
MplayIsTracker()
{
    return oOptions.bDisplayTracker;
}

static int
MplayProcessCmd(LPSTR lpCmdLine)
{
#ifdef UNICODE
    LPTSTR UniCmdLine;

    UniCmdLine = (LPTSTR)malloc((strlen(lpCmdLine) + 1) * sizeof(TCHAR));

    if (UniCmdLine == NULL) {
        //
        // We're probably going to crash anyway, but abort
        // the command line parsing.
        //
        return MPLAY_ERROR_NO_MEMORY;
    }

    OemToChar( lpCmdLine, UniCmdLine );

    SendMessage(GethWndMain(), C_WM_CMDLINE, 0, (LPARAM)UniCmdLine);

    free(UniCmdLine);

#else
    PostMessage(GethWndMain(), C_WM_CMDLINE, 0, (LPARAM)lpCmdLine);
#endif
    return MPLAY_ERROR_SUCCESS;
}

BOOL MplayInitApplication(HINSTANCE_T hInstance, MPLAY_OPTIONS * Options)
{
    WNDCLASSEX wc;
    DWORD IconWidth;
    DWORD IconHeight;
    DWORD SmallIconWidth;
    DWORD SmallIconHeight;

    LPCTSTR SmallIcon;

    //
    //  Define defaults to use if registry queries fail.  We do this
    //  here to ensure that the tracker will be resized correctly
    //  when windows are first created.
    //

    Options->bOnTop = FALSE;
    Options->bFullPath = FALSE;
    Options->bDisplayTracker = TRUE;
    Options->bHide = FALSE;

    //
    // If we go to high priority, this should protect against audio
    // crappiness when the system is under load.  We might not be
    // allowed to do this, so do it blind and ignore errors.
    //

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    //
    //  Load resizeable dialog support.
    //

    ResizeDialogInitialize( hInstance );

    WinVer = GetVersion();

    //
    //  This is a horrible hack.  I can't see a better approach.
    //  Win95 etc use a blue title bar with white text, so a white
    //  icon looks better.  Vista uses a really pale title bar, so
    //  the black icon looks better.  I'd base this choice off the
    //  title bar, but the same icon is also used in the taskbar.
    //  Worse, there's no guarantee the same icon can be right in
    //  both places.  On the other hand, most apps only have *one*
    //  icon, so we can do better than that ;).
    //

    if (MPLAY_WINVER_MAJOR(WinVer) >= 6) {
        SmallIcon = MAKEINTRESOURCE( IDI_ICOBLACK );
    } else {
        SmallIcon = MAKEINTRESOURCE( IDI_ICOWHITE );
    }

    IconWidth = GetSystemMetrics(SM_CXICON);
    IconHeight = GetSystemMetrics(SM_CYICON);
    SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
    SmallIconHeight = GetSystemMetrics(SM_CYSMICON);

    if (IconWidth == 0 || IconHeight == 0) {
        ASSERT( IconWidth != 0 && IconHeight != 0 );
        IconWidth = IconHeight = 32;
    }
    if (SmallIconWidth == 0 || SmallIconHeight == 0) {
        ASSERT( SmallIconWidth != 0 && SmallIconHeight != 0 );
        SmallIconWidth = SmallIconHeight = 16;
    }

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)MplayMainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH) GetStockObject( LTGRAY_BRUSH );
    wc.lpszMenuName =  _T("MPMENU");
    wc.lpszClassName = _T("SerenityAudioPlayer");
    //wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE( IDI_ICOMAIN ));
    wc.cbSize = sizeof(wc);
    //wc.hIconSm = LoadIcon(hInstance, SmallIcon);
    wc.hIconSm = LoadImage(hInstance,
        SmallIcon,
        IMAGE_ICON,
        SmallIconWidth,
        SmallIconHeight,
        LR_DEFAULTCOLOR);
    wc.hIcon = LoadImage(hInstance,
        MAKEINTRESOURCE( IDI_ICOMAIN ),
        IMAGE_ICON,
        IconWidth,
        IconHeight,
        LR_DEFAULTCOLOR);

    return RegisterClassEx(&wc);
}


int WINAPI
WinMain(HINSTANCE_T hInstance,HINSTANCE_T hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    MSG msg;
    HWND hWnd;
    HACCEL acctable;

    UNREFERENCED_PARAMETER(hPrevInstance);

    hInst = hInstance;
    nCmdShowGlobal = nCmdShow;

    //
    //  Note the implicit startup order:
    //  1. Window is created (WM_CREATE.)
    //  2. After UI is fully set up, C_WM_REALBEGIN.  This executes default.msc.
    //  3. Process command.  This may also run a script.
    //

    srand((UINT)GetTickCount());
    if (!MplayInitApplication(hInstance, &oOptions)) {
        return FALSE;
    }
    if (MplayCreateAppWindows(hInstance, nCmdShow) != MPLAY_ERROR_SUCCESS) {
        return FALSE;
    }
    acctable = LoadAccelerators(hInstance, _T("KEYS"));
    hWnd = GethWndMain();
    SendMessage(hWnd, C_WM_REALBEGIN, 0, 0);
    MplayProcessCmd(lpCmdLine);
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hWnd, acctable, &msg)) {
            TranslateMessage(&msg);
        }
        DispatchMessage(&msg);
    }
    return 0;
}


#define WIN_WIDTH_MIN  100
#define WIN_HEIGHT_MIN 100

#ifndef SM_XVIRTUALSCREEN
#define SM_XVIRTUALSCREEN 76
#endif
#ifndef SM_YVIRTUALSCREEN
#define SM_YVIRTUALSCREEN 77
#endif

static int 
MplayInitState( HWND hWnd, LIST * List, MPLAY_OPTIONS * Options )
{
    TCHAR   szTemp[MAX_PATH];
    HKEY    Key;
    LONG    Error;
    DWORD   Data;
    DWORD   DataSize;
    DWORD   DataType;
    DWORD   NewWidth;
    DWORD   NewHeight;
    DWORD   NewTop;
    DWORD   NewLeft;
    DWORD   ScreenWidth;
    DWORD   ScreenHeight;
    RECT    CurrentSize;

    //
    //  Firstly, load any values from the user's registry and perform any 
    //  UI fixups required.  At this point, the list should be empty.
    //

    Error = RegOpenKeyEx(HKEY_CURRENT_USER,
        _T("Software\\") _T(MPLAY_NAME),
        0,
        KEY_QUERY_VALUE,
        &Key);
    if (Error == ERROR_SUCCESS) {

        //
        //  If there were any items in the list at this point, we'd
        //  need to refresh.
        //

        ASSERT( GetListLength( List ) == 0 );

        DataSize = sizeof(Data);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_ONTOP,
            NULL,
            &DataType,
            (LPBYTE)&Data,
            &DataSize);
        if (Error == ERROR_SUCCESS && (DataType == REG_DWORD || DataType == REG_BINARY) && Data != 0) {
            Options->bOnTop = TRUE;
            MplaySetOnTop(hWnd, Options->bOnTop);
        }

        DataSize = sizeof(Data);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_FULLPATH,
            NULL,
            &DataType,
            (LPBYTE)&Data,
            &DataSize);
        if (Error == ERROR_SUCCESS && (DataType == REG_DWORD || DataType == REG_BINARY) && Data != 0) {
            Options->bFullPath = TRUE;
            CheckMenuItem(GetMenu(hWnd),
                IDM_DISPPATH,
                MF_CHECKED);
        }

        //
        //  Note that we display the tracker by default.  Only suppress display if the value exists
        //  and is zero.
        //

        DataSize = sizeof(Data);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_DISPLAYTRACKER,
            NULL,
            &DataType,
            (LPBYTE)&Data,
            &DataSize);
        if (Error == ERROR_SUCCESS && (DataType == REG_DWORD || DataType == REG_BINARY) && Data == 0) {
            Options->bDisplayTracker = FALSE;
            MplaySetTracker(hWnd, Options->bDisplayTracker);
        }

        //
        //  Now load any custom window size/position information.
        //

        DataSize = sizeof(NewLeft);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_WINPOSLEFT,
            NULL,
            &DataType,
            (LPBYTE)&NewLeft,
            &DataSize);
        if (Error != ERROR_SUCCESS || (DataType != REG_DWORD && DataType != REG_BINARY)) {
            NewLeft = MAXDWORD;
        }

        DataSize = sizeof(NewTop);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_WINPOSTOP,
            NULL,
            &DataType,
            (LPBYTE)&NewTop,
            &DataSize);
        if (Error != ERROR_SUCCESS || (DataType != REG_DWORD && DataType != REG_BINARY)) {
            NewTop = MAXDWORD;
        }

        DataSize = sizeof(NewWidth);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_WINPOSWIDTH,
            NULL,
            &DataType,
            (LPBYTE)&NewWidth,
            &DataSize);
        if (Error != ERROR_SUCCESS || (DataType != REG_DWORD && DataType != REG_BINARY)) {
            NewWidth = MAXDWORD;
        }

        DataSize = sizeof(NewHeight);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_WINPOSHEIGHT,
            NULL,
            &DataType,
            (LPBYTE)&NewHeight,
            &DataSize);
        if (Error != ERROR_SUCCESS || (DataType != REG_DWORD && DataType != REG_BINARY)) {
            NewHeight = MAXDWORD;
        }

        DataSize = sizeof(Data);
        Error = RegQueryValueEx(Key,
            MPLAY_REGKEY_WINPOSHIDE,
            NULL,
            &DataType,
            (LPBYTE)&Data,
            &DataSize);
        if (Error == ERROR_SUCCESS && (DataType == REG_DWORD || DataType == REG_BINARY) && Data != 0) {
            Options->bHide = TRUE;
        }

        GetWindowRect( hWnd, &CurrentSize );

        //
        //  Sanity check window position before blindly applying.
        //

        if (NewWidth < WIN_WIDTH_MIN) {
            NewWidth = WIN_WIDTH_MIN;
        }

        if (NewHeight < WIN_HEIGHT_MIN) {
            NewHeight = WIN_HEIGHT_MIN;
        }

        if (NewWidth == MAXDWORD) {
            NewWidth = CurrentSize.right - CurrentSize.left;
        }

        if (NewHeight == MAXDWORD) {
            NewHeight = CurrentSize.bottom - CurrentSize.top;
        }

        if (NewTop == MAXDWORD) {
            NewTop = CurrentSize.top;
        }

        if (NewLeft == MAXDWORD) {
            NewLeft = CurrentSize.left;
        }

        ScreenWidth = (DWORD)GetSystemMetrics( SM_XVIRTUALSCREEN );
        ScreenHeight = (DWORD)GetSystemMetrics( SM_YVIRTUALSCREEN );

        if (ScreenWidth == 0) {
            ScreenWidth = (DWORD)GetSystemMetrics( SM_CXSCREEN );
        }

        if (ScreenHeight == 0) {
            ScreenHeight = (DWORD)GetSystemMetrics( SM_CYSCREEN );
        }

        if (NewLeft + NewWidth > ScreenWidth) {
            NewLeft = ScreenWidth - NewWidth;
        }

        if (NewTop + NewHeight > ScreenHeight) {
            NewTop = ScreenHeight - NewHeight;
        }

        SetWindowPos( hWnd, (Options->bOnTop?HWND_TOPMOST:HWND_TOP), NewLeft, NewTop, NewWidth, NewHeight, 0 );

        RegCloseKey(Key);
    }

#if defined(SCRIPTING)&&(!defined(_CYGWIN))
    _tsearchenv(_T("default.msc"), _T("PATH"), szTemp);
    if (szTemp && _tcslen(szTemp)) {
        MplayPlayScript(List,
            szTemp);
        MplayListUiRefresh(GethWndCli(),
            List,
            Options->bFullPath);
    }
#endif

    //
    //  If, after loading the registry and scripts, the user wants us
    //  to display the window, go ahead and do so.
    //

    if (!Options->bHide) {

        ShowWindow(hWnd, nCmdShowGlobal);
        UpdateWindow(hWnd);
    }

    if (MplayIsPlaying() && MplayIsTracker() && IsWindowVisible(GethWndMain()))
        SetTimer(hWnd, TIMER_ID, TIMER_TICK, NULL);

    return MPLAY_ERROR_SUCCESS;
}

static int 
MplaySaveOptions( HWND hWnd, MPLAY_OPTIONS * Options )
{
    HKEY    Key = NULL;
    LONG    Error;
    DWORD   Data;
    DWORD   DataSize;
    RECT    CurrentSize;
    DWORD   Disposition;

    UNREFERENCED_PARAMETER(Options);

    //
    //  Open the key with all access, creating if it doesn't exist. 
    //

    Error = RegCreateKeyEx(HKEY_CURRENT_USER,
        _T("Software\\") _T(MPLAY_NAME),
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &Key,
        &Disposition);

    if (Error == ERROR_SUCCESS) {

        //
        //  Save user selectable options
        //

        Data = oOptions.bOnTop;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_ONTOP,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = oOptions.bFullPath;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_FULLPATH,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = oOptions.bDisplayTracker;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_DISPLAYTRACKER,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = oOptions.bHide;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_WINPOSHIDE,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        //
        //  Now save current window geometry
        //

        GetWindowRect( hWnd, &CurrentSize );

        Data = CurrentSize.left;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_WINPOSLEFT,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = CurrentSize.top;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_WINPOSTOP,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = CurrentSize.right - CurrentSize.left;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_WINPOSWIDTH,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        Data = CurrentSize.bottom - CurrentSize.top;
        DataSize = sizeof(Data);
        Error = RegSetValueEx(Key,
            MPLAY_REGKEY_WINPOSHEIGHT,
            0,
            REG_DWORD,
            (LPBYTE)&Data,
            sizeof(Data));

        if (Error != ERROR_SUCCESS) 
            goto RegError;

        RegCloseKey(Key);
    }

    return MPLAY_ERROR_SUCCESS;

RegError:

    if (Key != NULL)
        RegCloseKey(Key);

    return MPLAY_ERROR_REGISTRY;
}

BOOL
MplayAssembleUrl(
    LPTSTR szUrl,
    DWORD szUrlLength,
    LPTSTR szVerString,
    LPTSTR szFileName,
    LPTSTR szFileExtension
    )
{
    TCHAR szTempFileName[128];

    if (szFileName == NULL) {
        _stprintf_s(szTempFileName,
                    sizeof(szTempFileName)/sizeof(szTempFileName[0]),
                    _T("%s%s/serenity"),
#ifdef _M_AMD64
                    _T("amd64"),
#elif defined(_M_IX86)
                    _T("win32"),
#endif
#ifdef UNICODE
                    _T("-unicode")
#else
                    _T("-ansi")
#endif
            );

        szFileName = szTempFileName;
    }

    _stprintf_s(szUrl,
                szUrlLength,
                _T("http://www.malsmith.net/download/?obj=serenity/%s/%s%s"),
                szVerString?szVerString:
#ifdef PRERELEASE
                _T("latest-daily"),
#else
                _T("latest-stable"),
#endif
                szFileName,
                szFileExtension?szFileExtension:_T(".exe")
            );

    return TRUE;
}

typedef enum {
    MplayUpdateTypeBinary = 0,
    MplayUpdateTypeSource,
    MplayUpdateTypeStable,
    MplayUpdateTypeDaily
} MPLAY_UPDATE_TYPE;

BOOL
MplayUpdateBinary(
    MPLAY_UPDATE_TYPE UpdateType
    )
{
    TCHAR szAgent[128];
    TCHAR szUrl[250];
    TCHAR szVer[32];
    TCHAR szResult[512];
    LPTSTR TargetName = NULL;
    UpdError Error = 0;

#if MPLAY_BUILD_ID
    _stprintf_s(szVer, sizeof(szVer)/sizeof(szVer[0]), _T("%i.%i.%i.%i"), MPLAY_VER_MAJOR, MPLAY_VER_MINOR, MPLAY_VER_MICRO, MPLAY_BUILD_ID);
#else
    _stprintf_s(szVer, sizeof(szVer)/sizeof(szVer[0]), _T("%i.%i.%i"), MPLAY_VER_MAJOR, MPLAY_VER_MINOR, MPLAY_VER_MICRO);
#endif
    _stprintf_s(szAgent, sizeof(szAgent)/sizeof(szAgent[0]), _T("serenity %s"), szVer);

    if (UpdateType == MplayUpdateTypeBinary) {
        MplayAssembleUrl(szUrl, sizeof(szUrl)/sizeof(szUrl[0]), NULL, NULL, NULL);
    } else if (UpdateType == MplayUpdateTypeSource) {
        MplayAssembleUrl(szUrl, sizeof(szUrl)/sizeof(szUrl[0]), szVer, _T("serenity-source"), _T(".zip"));
        TargetName = _T("serenity-source.zip");
    } else if (UpdateType == MplayUpdateTypeStable) {
        MplayAssembleUrl(szUrl, sizeof(szUrl)/sizeof(szUrl[0]), _T("latest-stable"), NULL, NULL);
    } else if (UpdateType == MplayUpdateTypeDaily) {
        MplayAssembleUrl(szUrl, sizeof(szUrl)/sizeof(szUrl[0]), _T("latest-daily"), NULL, NULL);
    }
    Error = UpdateBinaryFromUrl(szUrl, TargetName, szAgent);

    if (Error == UpdErrorSuccess) {
        if (UpdateType == MplayUpdateTypeBinary ||
            UpdateType == MplayUpdateTypeStable ||
            UpdateType == MplayUpdateTypeDaily) {

            MessageBox(GethWndMain(), _T("Update successful.  Please restart the application to launch the new version."), _T("Update"), 0);
        } else {
            _stprintf_s(szResult, sizeof(szResult)/sizeof(szResult[0]), _T("Successfully downloaded %s for version %s."), TargetName, szVer);
            MessageBox(GethWndMain(), szResult, _T("Update"), 0);
        }
    } else {
        _stprintf_s(szResult, sizeof(szResult)/sizeof(szResult[0]), _T("Update error: %s\nUpdating from: %s"), UpdateErrorString(Error), szUrl);
        MessageBox(GethWndMain(), szResult, _T("Update"), 0);
    }
    return TRUE;
}

LRESULT APIENTRY
MplayMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static  LIST List;
    static  LIST RecentSearches;
    static  DWORD dwFindFlags;
    static  TCHAR szOpenFile[MAX_PATH];
    static  DWORD dwRepeat;
    TCHAR   szTemp[MAX_PATH];
    LPNMHDR hdr;

    switch (message) {
    case WM_NOTIFY:
        hdr = (LPNMHDR)lParam;
        switch( hdr->code ) {
            case NM_DBLCLK:
                {
                    int nIndex;
                    nIndex = MplayListUiGetActive(GethWndCli(), TRUE);
                    ASSERT( nIndex >= 0 );
                    MplayPlayMediaFile(hWnd, &List, nIndex);
                    MplayUpdatePauseStatus(hWnd, FALSE);
                }
                break;
        }
     case WM_COMMAND:
        switch(HIWORD(wParam)) {
        case LBN_SELCANCEL:
            MplayCloseStream(hWnd);
            break;

        case LBN_DBLCLK:
            {
                int nIndex;
                nIndex = MplayListUiGetActive(GethWndCli(), TRUE);
                ASSERT( nIndex >= 0 );
                MplayPlayMediaFile(hWnd, &List, nIndex);
                MplayUpdatePauseStatus(hWnd, FALSE);
            }
            break;
        }
        switch(LOWORD(wParam)) {
            /*************/
            /* FILE MENU */
            /*************/
            case IDM_NEW:
                _tcscpy(szOpenFile, _T(""));
                MplayRefreshTitle(szOpenFile);
                MplayClearList(&List);
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;
            case IDM_OPEN:
                _tcscpy(szTemp, _T(""));
                if (MplayGetaFile(szTemp,FALSE) == MPLAY_ERROR_SUCCESS) {
                    LPTSTR a;
                    MplayClearList(&List);
                    MplayInputFile(&List,
                        szTemp);
                    MplayListUiRefresh(GethWndCli(),
                        &List,
                        oOptions.bFullPath);
                    _tcscpy(szOpenFile,szTemp);
                    _tcsupr(szTemp);
                    if ((a = _tcsrchr(szTemp, _T('.'))) != NULL) {
                        if (_tcscmp(a+1, _T("M3U"))) {
                            _tcscpy(szOpenFile, _T(""));
                        }
                    } else _tcscpy(szOpenFile, _T(""));
                    MplayRefreshTitle(szOpenFile);
                }
                break;
            case IDM_SAVE:
                if (_tcslen(szOpenFile)) {
                    MplayWriteList(&List,szOpenFile);
                } else {
                    MessageBeep(MB_ICONEXCLAMATION);
                    MessageBox(hWnd,
                        _T("There is currently no open file."),
                        _T(MPLAY_NAME),
                        MB_ICONEXCLAMATION);
                }
                break;

            case IDM_SAVEAS:
                _tcscpy(szTemp, szOpenFile);
                if (MplayGetaFile(szTemp,TRUE) == MPLAY_ERROR_SUCCESS) {
                    _tcscpy(szOpenFile, szTemp);
                    MplayRefreshTitle(szOpenFile);
                    MplayWriteList(&List,szTemp);
                }
                break;
            case IDM_UPDATE_CURRENT:
                MplayUpdateBinary(MplayUpdateTypeBinary);
                break;
            case IDM_UPDATE_STABLE:
                MplayUpdateBinary(MplayUpdateTypeStable);
                break;
            case IDM_UPDATE_DAILY:
                MplayUpdateBinary(MplayUpdateTypeDaily);
                break;
            case IDM_UPDATE_SOURCE:
                MplayUpdateBinary(MplayUpdateTypeSource);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                PostQuitMessage(0);
                break;
            case IDM_ABOUT:
                {
                    _stprintf(szTemp, _T("%s, %i.%i.%i.")
#ifdef PRERELEASE
                        _T(" Prerelease build, built on %s. Compiled with ")
#ifdef MINICRT
                        _T("MiniCrt and")
#else
                        _T("MSVCRT and")
#endif
#ifdef UNICODE
                        _T(" Unicode.")
#else
                        _T(" ANSI.")
#endif
#endif
                        _T(" %s.  ")
                        _T("This software is licensed under the terms of the GNU Public License, ")
                        _T("version 2 or any later version."),
                        _T(MPLAY_NAME),
                        MPLAY_VER_MAJOR,
                        MPLAY_VER_MINOR,
                        MPLAY_VER_MICRO,
#ifdef PRERELEASE
                        _T(__DATE__),
#endif
                        _T(MPLAY_COPYRIGHT)
                        );
                    MessageBox(hWnd,
                        szTemp,
                        _T(MPLAY_NAME),
                        MB_ICONINFORMATION);
                    break;
                }
#ifdef DEBUG_MPLAY
            case IDM_DEBUG:
                ASSERT( !"User breakin request.  This is NOT a bug." );
                break;
#endif
            /**************/
            /* MEDIA MENU */
            /**************/
            case IDM_PLAY:
                {
                    int nIndex;
                    nIndex = MplayListUiGetActive(GethWndCli(), TRUE);
                    ASSERT( nIndex >= 0 );
                    MplayPlayMediaFile(hWnd,
                        &List,
                        nIndex);
                    MplayUpdatePauseStatus(hWnd, FALSE);
                }
                break;
            case IDM_STOP:
                MplayCloseStream(hWnd);
                MplayUpdatePauseStatus(hWnd, FALSE);
                break;
            case IDM_PAUSE:
                MplayPauseAudio(hWnd);
                MplayUpdatePauseStatus(hWnd, TRUE);
                break;
            case IDM_RESUME:
                MplayResumeAudio(hWnd);
                MplayUpdatePauseStatus(hWnd, FALSE);
                break;

            case IDM_NEXT:
                MplayPlayMediaFile(hWnd, &List, PLAY_NEXT);
                break;
            case IDM_PREVIOUS:
                MplayPlayMediaFile(hWnd, &List, PLAY_PREV);
                break;
            case IDM_FIRST:
                MplayPlayMediaFile(hWnd, &List, 0);
                break;
            case IDM_LAST:
                ASSERT( GetListLength(&List) < INT_MAX );
                MplayPlayMediaFile(hWnd,
                    &List,
                    (int)GetListLength(&List)-1);
                break;
            case IDM_REP_NEXTTRACK:
                dwRepeat = REPEAT_NEXTTRACK;

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NEXTTRACK,
                    MF_CHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_THISTRACK,
                    MF_UNCHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NOTRACK,
                    MF_UNCHECKED);
                break;
            case IDM_REP_THISTRACK:
                dwRepeat = REPEAT_THISTRACK;
                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NEXTTRACK,
                    MF_UNCHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_THISTRACK,
                    MF_CHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NOTRACK,
                    MF_UNCHECKED);
                break;
            case IDM_REP_NOTRACK:
                dwRepeat = REPEAT_NOTRACK;
                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NEXTTRACK,
                    MF_UNCHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_THISTRACK,
                    MF_UNCHECKED);

                CheckMenuItem(GetMenu(hWnd),
                    IDM_REP_NOTRACK,
                    MF_CHECKED);
                break;
            case IDM_VOL_UP:
                {
                    DWORD Vol;
                    if (MplayGetVolume(&Vol) == MPLAY_ERROR_SUCCESS) {
                        Vol = ((Vol / 0x800) + 1) * 0x8;
                        if (Vol > 0xff) Vol = 0xff;
                        Vol = MAKEWORD(Vol, Vol);
                        MplaySetVolume(Vol);
                    } else {
                        MessageBox(hWnd,
                            _T("Could not change volume"),
                            _T(MPLAY_NAME),
                            MB_ICONEXCLAMATION);
                    }
                }
                break;
            case IDM_VOL_DOWN:
                {
                    DWORD Vol;
                    if (MplayGetVolume(&Vol) == MPLAY_ERROR_SUCCESS) {
                        Vol = (Vol / 0x800);
                        if (Vol > 0) Vol = (Vol - 1) * 0x8;
                        Vol = MAKEWORD(Vol, Vol);
                        MplaySetVolume(Vol);
                    } else {
                        MessageBox(hWnd,
                            _T("Could not change volume"),
                            _T(MPLAY_NAME),
                            MB_ICONEXCLAMATION);
                    }
                }
                break;
            /**************/
            /* Items Menu */
            /**************/
            case IDM_SORTDISPLAY:
                MplaySortList(&List, SORT_DISPLAY);
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;

            case IDM_SORTFILE:
                MplaySortList(&List, SORT_FILE);
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;
            case IDM_SORTPATH:
                MplaySortList(&List, SORT_PATH);
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;

            case IDM_SHUFFLE:
                MplayShuffleList(&List);
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;
            case IDM_MOVEUP:
                {
                    int i;
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    if (i > 0) {
                        MplaySwapListItems(&List, i, i-1);
                        MplayListUiSwapItems( GethWndCli(), &List, i, i - 1, oOptions.bFullPath );
                        MplayListUiSetActive( GethWndCli(), i - 1 );
                    }
                }
                break;
            case IDM_MOVEDOWN:
                {
                    int i;
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    if (i >= 0 && i < (int)GetListLength(&List) - 1) {
                        MplaySwapListItems(&List, i, i + 1 );
                        MplayListUiSwapItems( GethWndCli(), &List, i, i + 1, oOptions.bFullPath );
                        MplayListUiSetActive( GethWndCli(), i + 1 );
                    }
                }
                break;
            case IDM_DELETE:
                {
                    DWORD i;
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    MplayDeleteListItem(&List, i);
                    MplayListUiDeleteItem( GethWndCli(), i );
                    if (i == GetListLength(&List)) i--;
                    MplayListUiSetActive( GethWndCli(), i );
                }
                break;
            case IDM_DISPPATH:
                oOptions.bFullPath = !oOptions.bFullPath;
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                if (oOptions.bFullPath) {
                    CheckMenuItem(GetMenu(hWnd),
                        IDM_DISPPATH,
                        MF_CHECKED);
                } else {
                    CheckMenuItem(GetMenu(hWnd),
                        IDM_DISPPATH,
                        MF_UNCHECKED);
                }
                break;
            case IDM_ONTOP:
                oOptions.bOnTop = !oOptions.bOnTop;
                MplaySetOnTop(hWnd, oOptions.bOnTop);
                break;
            case IDM_DISPTRACK:
                oOptions.bDisplayTracker = !MplayIsTracker();
                MplaySetTracker(hWnd, oOptions.bDisplayTracker);
                break;
            case IDM_SAVEOPTIONS:
                MplaySaveOptions(hWnd, &oOptions);
                break;
            case IDM_REFRESH:
                MplayListUiRefresh(GethWndCli(),
                    &List,
                    oOptions.bFullPath);
                break;
            case IDM_IMPORTFILE:
                _tcscpy(szTemp, _T(""));
                if (MplayGetaFile(szTemp, FALSE) == MPLAY_ERROR_SUCCESS) {
                    DWORD dwInitialListSize = GetListLength(&List);
                    MplayInputFile(&List,szTemp);
                    MplayListUiRefreshAppend(GethWndCli(),
                        &List,
                        oOptions.bFullPath,
                        dwInitialListSize);
                }
                break;
            case IDM_FIND:
                if (MplayFindEntry(&RecentSearches, &dwFindFlags) == MPLAY_ERROR_SUCCESS) {
                    LPTSTR szFindText = GetListItemU(&RecentSearches, 0);
                    int i;
                    ASSERT(szFindText != NULL);
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    if (i == LB_ERR) {
                        i = 0;
                    }
                    ASSERT( i >= 0 );
                    i = MplayFindNextText( &List,
                        i,
                        szFindText,
                        dwFindFlags);
                    if (i >= 0) {
                        MplayListUiSetActive( GethWndCli(), i );
                    } else {
                        MessageBox(hWnd,
                            _T("No matches found."),
                            _T(MPLAY_NAME),
                            MB_ICONINFORMATION);
                    }
                }
                break;
            case IDM_FINDNEXT:
                {
                    long i = -1;
                    LPTSTR szFindText = GetListItemU(&RecentSearches, 0);
                    if (szFindText != NULL) {
                        i = MplayListUiGetActive(GethWndCli(), FALSE);
                        if (i == LB_ERR) {
                            i = 0;
                        } else {
                            i++;
                        }
                        ASSERT( i >= 0 );
                        if (dwFindFlags & FIND_STARTTOP) dwFindFlags-=FIND_STARTTOP;
                        i = MplayFindNextText(&List,
                            i,
                            szFindText,
                            dwFindFlags);
                    }
                    if (i >= 0) {
                        MplayListUiSetActive( GethWndCli(), i );
                    } else {
                        MessageBox(hWnd,
                            _T("No more matches found."),
                            _T(MPLAY_NAME),
                            MB_ICONINFORMATION);
                    }
                }
                break;
            case IDM_FINDPREV:
                {
                    long i = -1;
                    LPTSTR szFindText = GetListItemU(&RecentSearches, 0);
                    if (szFindText != NULL) {
                        DWORD dwLocalFindFlags;
                        i = MplayListUiGetActive(GethWndCli(), FALSE);
                        if (i == LB_ERR) {
                            i = GetListLength(&List) - 1;
                        } else {
                            i--;
                        }
                        if (dwFindFlags & FIND_STARTTOP) dwFindFlags -= FIND_STARTTOP;
                        if (i >= 0) {
                            dwLocalFindFlags = dwFindFlags ^ FIND_BACKWARDS;
                            i = MplayFindNextText(&List,
                                i,
                                szFindText,
                                dwLocalFindFlags);
                        }
                    }
                    if (i >= 0) {
                        MplayListUiSetActive( GethWndCli(), i );
                    } else {
                        MessageBox(hWnd,
                            _T("No more matches found."),
                            _T(MPLAY_NAME),
                            MB_ICONINFORMATION);
                    }
                }
                break;
            case IDM_STRIP:
                if (MplayFindEntry(&RecentSearches,&dwFindFlags) == MPLAY_ERROR_SUCCESS) {
                    LPTSTR szFindText = GetListItemU(&RecentSearches, 0);
                    int i;
                    ASSERT(szFindText != NULL);
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    if (i==LB_ERR) {
                        i = 0;
                    }
                    ASSERT( i >= 0 );
                    i = MplayFindNextText(&List,
                        i,
                        szFindText,
                        dwFindFlags);
                    while (i >= 0) {
                        MplayDeleteListItem(&List, i);
                        i--;
                        if (i<0) i=0;
                        i = MplayFindNextText(&List,
                            i,
                            szFindText,
                            dwFindFlags);
                    }
                    MplayListUiRefresh(GethWndCli(),
                        &List,
                        oOptions.bFullPath);
                }
                break;
#ifdef INTERNAL
            case IDM_PROPERTIES:
                {
                    LONG i;
                    i = MplayListUiGetActive(GethWndCli(), FALSE);
                    if (i >= 0) {
                        LPTSTR szListItem = GetListItemU(&List, (DWORD)i);
                        DialogBoxParam(GethInst(),
                            _T("IDDPROP"),
                            GethWndMain(),
                            (DLGPROC)MplayPropertiesDlgProc,
                            (LPARAM)szListItem);
                    } else {
                        MessageBox(hWnd,
                            _T("Please select the track which you would like to obtain Properties for."),
                            _T(MPLAY_NAME),
                            MB_ICONINFORMATION);
                    }
                }
                break;
#endif
            default:
                return DefWindowProc(hWnd,
                    message,
                    wParam,
                    lParam);
                break;
            }
        break;
    case WM_CREATE:
        {
            oOptions.bFullPath = FALSE;
            oOptions.bOnTop = FALSE;
            dwRepeat = REPEAT_NEXTTRACK;
            _tcscpy(szOpenFile, _T(""));
            dwFindFlags = 0;
            MplayResetHandlers();
            MplayAddHandler(_T("mp3"), _T("MPEGVideo"));
            MplayInitList(&List);
            MplayInitList(&RecentSearches);
            DragAcceptFiles(hWnd,TRUE);
            MplayAddTrayIcon(hWnd);
        }

        break;
    case C_WM_REALBEGIN:
        MplayInitState(hWnd, &List, &oOptions);
        break;
#ifdef WM_MOUSEWHEEL
    case WM_MOUSEWHEEL:
        PostMessage(GethWndCli(), WM_MOUSEWHEEL, wParam, lParam);
        break;
#endif
    case WM_DROPFILES:
        {
            DWORD i,nFiles;
            HDROP hDrop;
            HANDLE hFind;
            pi_find fd;
            DWORD InitialListSize;
            hDrop = (HDROP)wParam;
            nFiles = DragQueryFile(hDrop, 0xffffffff, NULL, 0);
            InitialListSize = GetListLength(&List);
            for (i = 0; i < nFiles; i++) {
                DragQueryFile(hDrop,
                    (int)i,
                    szTemp,
                    sizeof(szTemp)/sizeof(TCHAR));

                hFind = MplayFindFirst(szTemp, &fd);
                MplayFindClose(hFind);
                if (MplayGetAttr(&fd) & FA_SUBDIR) { 
                    MplayGenerateTree(&List, szTemp);
                } else { 
                    MplayInputFile(&List, szTemp);
                }
            }
            DragFinish(hDrop);
            MplayListUiRefreshAppend(GethWndCli(),
                &List,
                oOptions.bFullPath,
                InitialListSize);

        }
        break;
    case WM_CLOSE:
#if !defined(BROKEN_MINIMIZE)
        oOptions.bHide = TRUE;
        MplayDispWindow(hWnd, FALSE);
#else
        MplayCloseStream(hWnd);
        DestroyWindow(hWnd);
        PostQuitMessage(0);
#endif
        break;
    case TRAY_CALLBACK:
        MplayTrayCallback(hWnd, wParam, lParam, &oOptions);
        break;
    case WM_DESTROY:
        MplayFreeList(&List);
        MplayDelTrayIcon(hWnd);
        break;
    case MM_MCINOTIFY:
            if (wParam==MCI_NOTIFY_SUCCESSFUL) {
                switch(dwRepeat) {
                case REPEAT_NEXTTRACK:
                    if (MplayPlayMediaFile(hWnd, &List, PLAY_NEXT) != MPLAY_ERROR_SUCCESS) {
                        MplayCloseStream(hWnd);
                    }
                    break;
                case REPEAT_THISTRACK:
                    if (MplayPlayMediaFile(hWnd, &List, PLAY_SAME) != MPLAY_ERROR_SUCCESS) {
                        MplayCloseStream(hWnd);
                    }
                    break;
                }
            }

        break;
    case C_WM_CMDLINE:
        {
            LPTSTR szPtr;
            HANDLE hFind;
            pi_find fd;
            szPtr = (LPTSTR)lParam;

            //
            //  We're about to copy into a MAX_PATH buffer.  Since the only
            //  input we expect here are paths to files, a command line of
            //  longer than that isn't something we can handle anyway.
            //
            if (_tcslen(szPtr) < MAX_PATH) {

                if (szPtr[0] == '"') {
                    _tcscpy(szTemp, szPtr + 1);
                    szPtr = _tcschr(szTemp, _T('"'));
                    if (szPtr) szPtr[0]='\0';
                } else {
                    _tcscpy(szTemp,(LPTSTR)lParam);
                }
                if ((hFind = MplayFindFirst(szTemp, &fd)) != 0) {
                    if (szTemp && _tcslen(szTemp)) {
                        MplayFindClose(hFind);
                        if (MplayGetAttr(&fd) & FA_SUBDIR) {
                            MplayGenerateTree(&List,
                                szTemp);
                        } else {
                            MplayInputFile(&List,
                                szTemp);
                        }
                        MplayListUiRefresh(GethWndCli(),
                            &List,
                            oOptions.bFullPath);
                        MplayPlayMediaFile(hWnd,
                            &List,
                            0);
                    }
                }
            }
        }

        break;

    case WM_SIZE:
        MplayResizeCallback(hWnd, wParam, lParam);
        break;
    case WM_TIMER:
        if (MplayIsTracker())
            MplayUpdatePos();
        break;
    case WM_HSCROLL:
        if (LOWORD(wParam) == TB_THUMBTRACK)
            KillTimer(hWnd, TIMER_ID);
        if (LOWORD(wParam) == SB_ENDSCROLL) {
            MplaySetAudioPos(hWnd);
            if (MplayIsPlaying() && MplayIsTracker())
                SetTimer(hWnd, TIMER_ID, TIMER_TICK, NULL);
            MplayUpdatePauseStatus(hWnd, FALSE);
        }

        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

