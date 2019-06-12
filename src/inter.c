/*
 * Inter.c:
 *
 * This file provides all the really platform-dependant interface stuff.
 *
 * Copyright (c) Malcolm Smith 2001-2017.  No warranty is provided.
 */

#include "pch.h"

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

#define PTR_SIZE 32
#define MARGIN 0
DWORD dwPtrHeight = 0;

HWND hWndMain = NULL;
HWND hWndCli = NULL;
HWND hWndPtr = NULL;
HWND hWndStatus = NULL;

HWND GethWndMain()
{
    return hWndMain;
}

HWND GethWndCli()
{
    return hWndCli;
}

HWND GethWndPtr()
{
    return hWndPtr;
}

HWND GethWndStatus()
{
    return hWndStatus;
}

//
//  The following routines implement fundamental primitives
//  to manipulating the UI list.
//

//
//  These routines are internal to this file.
//
#define MPLAY_USE_LISTBOX
#ifdef MPLAY_USE_LISTBOX
static HWND
MplayListUiCreate( HWND hWndParent, HINSTANCE hInstance )
{
    HWND hWnd;
    hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        _T("LISTBOX"),
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
        0,
        0,
        32,
        32,
        (HWND)hWndParent,
        (HMENU)NULL,
        (HINSTANCE_T)hInstance,
        NULL
    );
    return hWnd;
}

static int
MplayListUiRefreshClient( HWND hWnd )
{
    //
    //  List boxes don't need this.
    //
    UNREFERENCED_PARAMETER(hWnd);
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiResize( HWND hWnd, int width, int height )
{
    MoveWindow( hWnd,
        0,
        0,
        (int)width,
        (int)height,
        TRUE);

    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiInsertItem( HWND hWnd, DWORD dwItem, PTCHAR szText )
{
    int err = (int)SendMessage( hWnd, LB_INSERTSTRING, (WPARAM)dwItem, (LPARAM)szText);
    if (err < 0) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    return MPLAY_ERROR_SUCCESS;
}

static int 
MplayListUiAppendItem( HWND hWnd, PTCHAR szText )
{
    int err = (int)SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)szText );
    if (err < 0) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiClear( HWND hWnd )
{
    //
    // According to MSDN, this can't fail.
    //
    SendMessage(hWnd,LB_RESETCONTENT, 0, 0);
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiGetItemText( HWND hWnd, DWORD dwItem, PTCHAR szText )
{
    int err;
    int cchItemLength;
    ASSERT( szText != NULL && hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );

    //
    //  We don't manage items >MAX_PATH.  If there's something in our list box
    //  greater than that, we probably didn't put it there.
    //

    cchItemLength = (int)SendMessage(hWnd, LB_GETTEXTLEN, (WPARAM)dwItem, 0);
    if (cchItemLength >= MAX_PATH) {
        return MPLAY_ERROR_OVERFLOW;
    }
    err = (int)SendMessage(hWnd, LB_GETTEXT, (WPARAM)dwItem, (LPARAM)szText);
    if (err < 0) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    return MPLAY_ERROR_SUCCESS;
}

//
//  The following primitives are exposed to other modules.
//
int
MplayListUiDeleteItem( HWND hWnd, DWORD dwItem )
{
    int err;
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
       err = (int)SendMessage( hWnd, LB_DELETESTRING, (WPARAM)dwItem, 0);
    ASSERT( err >= 0 );
    if (err < 0) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    return MPLAY_ERROR_SUCCESS;
}


int
MplayListUiGetActive(HWND hWnd, BOOL EnsureSelected)
{
    int i;
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    i = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR && EnsureSelected) {
        SendMessage(hWnd, LB_SETCURSEL, 0, 0);
        i = 0;
    }
    return i;
}

int
MplayListUiSetActive(HWND hWnd, DWORD dwNewItem)
{
    int err;
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    err = (int)SendMessage(hWnd,
        LB_SETCURSEL,
        (WPARAM)dwNewItem,
        0);
    //
    // This could happen if we asked to select something beyond the bounds of
    // the list.
    //
    ASSERT(err != LB_ERR);
    return MPLAY_ERROR_SUCCESS;
}
#else

#ifndef LVS_EX_FULLROWSELECT
#define LVS_EX_FULLROWSELECT (0x00000020)
#endif
#ifndef LVS_EX_FLATSB       
#define LVS_EX_FLATSB        (0x00000100)
#endif
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER  (0x00010000)
#endif

#ifndef LVM_SETEXTENDEDLISTVIEWSTYLE
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST + 54)
#endif


static HWND
MplayListUiCreate( HWND hWndParent, HINSTANCE hInstance )
{
    HWND hWnd;
    LV_COLUMN col;
    hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SINGLESEL,
        0,
        0,
        32,
        32,
        (HWND)hWndParent,
        (HMENU)NULL,
        (HINSTANCE_T)hInstance,
        NULL
    );
    if (hWnd == NULL || hWnd == INVALID_HANDLE_VALUE) {
        return hWnd;
    }
    memset( &col, 0, sizeof( col ));
    col.cx = 32;
    col.mask = LVCF_WIDTH;
    ListView_InsertColumn( hWnd, 0, &col );

    //
    //  This should be IE3+.  Some of the values are much newer.  We don't really
    //  require any of this to work, it just helps things look nice.
    //
    SendMessage( hWnd,
        LVM_SETEXTENDEDLISTVIEWSTYLE,
        LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_DOUBLEBUFFER,
        LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | LVS_EX_DOUBLEBUFFER );

    return hWnd;
}

static int
MplayListUiRefreshClient( HWND hWnd )
{
    LV_COLUMN col;
    RECT rect;

    GetClientRect( hWnd, &rect );

    memset( &col, 0, sizeof( col ));
    col.cx = rect.right;
    col.mask = LVCF_WIDTH;
    ListView_SetColumn( hWnd, 0, &col );
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiResize( HWND hWnd, int width, int height )
{
    MoveWindow( hWnd,
        0,
        0,
        (int)width,
        (int)height,
        TRUE);

    return MplayListUiRefreshClient( hWnd );
}


static int
MplayListUiInsertItem( HWND hWnd, DWORD dwItem, PTCHAR szText )
{
    LV_ITEM Item;
    int err;
    memset( &Item, 0, sizeof( Item ) );
    Item.mask=LVIF_TEXT;
    Item.iItem = dwItem;
    Item.pszText = szText;
    err = ListView_InsertItem( hWnd, &Item );
    if (err < 0) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    return MPLAY_ERROR_SUCCESS;
}

static int 
MplayListUiAppendItem( HWND hWnd, PTCHAR szText )
{
    int Count = ListView_GetItemCount( hWnd );
    return MplayListUiInsertItem( hWnd, Count, szText );
}

static int
MplayListUiClear( HWND hWnd )
{
    if (!ListView_DeleteAllItems( hWnd )) {
        ASSERT( FALSE );
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayListUiGetItemText( HWND hWnd, DWORD dwItem, PTCHAR szText )
{
    ASSERT( szText != NULL && hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );

    ListView_GetItemText( hWnd, dwItem, 0, szText, MAX_PATH );
    return MPLAY_ERROR_SUCCESS;
}

//
//  The following primitives are exposed to other modules.
//
int
MplayListUiDeleteItem( HWND hWnd, DWORD dwItem )
{
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    if (!ListView_DeleteItem( hWnd, dwItem )) {
        ASSERT( FALSE );
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    return MPLAY_ERROR_SUCCESS;
}


int
MplayListUiGetActive(HWND hWnd, BOOL EnsureSelected)
{
    int i;
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    i = ListView_GetNextItem( hWnd, -1, LVNI_FOCUSED );
    if (i < 0 && EnsureSelected) {
        MplayListUiSetActive( hWnd, 0 );
        i = 0;
    }
    return i;
}

#ifndef LVIS_GLOW
#define LVIS_GLOW (0x0010)
#endif

int
MplayListUiSetActive(HWND hWnd, DWORD dwNewItem)
{
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    ListView_EnsureVisible( hWnd, dwNewItem, FALSE );
    ListView_SetItemState( hWnd,
        dwNewItem,
        LVIS_FOCUSED|LVIS_SELECTED|LVIS_GLOW,
        LVIS_FOCUSED|LVIS_SELECTED|LVIS_GLOW );
    SetFocus( hWnd );
    return MPLAY_ERROR_SUCCESS;
}
#endif

//
//  These are higher level ListUi routines, which are built on the above
//  primitives.
//
int
MplayListUiRefreshAppend(HWND hWnd,LIST * List,BOOL bFullPath,DWORD dwStartItem)
{
    DWORD i;
    LPTSTR a;
    LPTSTR b = NULL; // Compiler warning suppression
    DWORD ListMax;
    int err;
    ListMax = GetListLength(List);
    for (i = dwStartItem; i < ListMax; i++) {
        a = GetListItemU(List, i);

        if (!bFullPath) {
            a = MplayGetPtr(a, SORT_DISPLAY);
            ASSERT(a != NULL);
            b = _tcsrchr(a, '.');
            if (b != NULL) b[0] = '\0';
        }
        err = MplayListUiAppendItem( hWnd, a );
        if (err != MPLAY_ERROR_SUCCESS) {
            //
            // If this happens, we're screwed.  The list will contain
            // some things and not others, and our list mapping logic
            // will break.
            //
            return err;
        }
        if (!bFullPath) if (b) b[0] = '.';
    }
    MplayListUiRefreshClient( hWnd );
    return MPLAY_ERROR_SUCCESS;
}

int
MplayListUiRefresh(HWND hWnd,LIST * List,BOOL bFullPath)
{
    MplayListUiClear( hWnd );
    return MplayListUiRefreshAppend(hWnd, List, bFullPath, 0);
}


int
MplayListUiSwapItems(HWND hWnd, LIST * List, DWORD dwSrcItem, DWORD dwDestItem, BOOL bFullPath)
{
    TCHAR szTemp[MAX_PATH];
    ASSERT( hWnd != NULL && hWnd != INVALID_HANDLE_VALUE );
    ASSERT( List != NULL );

    if (dwSrcItem == dwDestItem)
        return MPLAY_ERROR_SUCCESS;

    //
    //  We optimize the UI by trying to move items in the list control
    //  directly.  If we can't (for whatever reason), we can fall back
    //  to refreshing the entire list - expensive, but correct.
    //
    if (MplayListUiGetItemText( hWnd, dwSrcItem, szTemp ) == MPLAY_ERROR_SUCCESS) {

        MplayListUiDeleteItem( hWnd, dwSrcItem );

        MplayListUiInsertItem( hWnd, dwDestItem, szTemp );
        
    } else {

        ASSERT( !"Refreshing list to perform move" );
        return MplayListUiRefresh(hWnd,
            List,
            bFullPath);

    }
    return MPLAY_ERROR_SUCCESS;
}

int
MplayGetaFile(LPTSTR szFile, BOOL Save)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szCurrentWorkingDirectory[MAX_PATH];
    OPENFILENAME ofn;
    BOOL bResult;

    //
    //  The interface takes all file extensions twice with display string
    //  followed by the real string.  The size we need is twice the size
    //  of the extension string plus the overhead we're adding.  Note
    //  these sizeofs are always ANSI strings (since it's an element count)
    //  and they're also adding in a couple extra NULLs, which we'll need.
    //

    TCHAR szFileTypeString[2 * (sizeof(szTemp)/sizeof(szTemp[0])) + sizeof("Known files (*.m3u;*.msc)") + sizeof("*.m3u;*.msc")];

    ASSERT( szFile != NULL );
    _tcscpy(szTitle, _T(""));

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GethWndMain();
    ofn.hInstance = GethInst();
    ofn.hInstance = NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH-2;
    ofn.lpstrFileTitle = szTitle;
    ofn.nMaxFileTitle = sizeof(szTitle)/sizeof(szTitle[0]);

    //
    //  Note that some versions of Windows appear to ignore
    //  lpstrInitialDir completely.  For versions that don't, set it to
    //  the current directory so users can set working directories on
    //  their shortcuts.  If we already have a filename, use that path
    //  in preference.
    //

    if (szFile[0] == '\0') {
        GetCurrentDirectory( MAX_PATH, szCurrentWorkingDirectory );
        ofn.lpstrInitialDir = szCurrentWorkingDirectory;
    } else {
        ofn.lpstrInitialDir = NULL;
    }
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    if (!Save) {
        LPTSTR a;
        ofn.lpstrTitle = _T("Open");
        MplayEnumHandlerExtensions(szTemp, sizeof(szTemp)/sizeof(szTemp[0]), _T(";"));
        ofn.lpstrDefExt = _T("m3u");
        _stprintf(szFileTypeString, _T("Known files (*.m3u;*.msc%s)"), szTemp);
        a = szFileTypeString + _tcslen(szFileTypeString) + 1;
        _stprintf(a, _T("*.m3u;*.msc%s"), szTemp);
        a = a + _tcslen(a) + 1;
        a[0] = '\0';
        ofn.lpstrFilter = szFileTypeString;
    } else {
        ofn.lpstrTitle = _T("Save As");
        ofn.lpstrDefExt = _T("m3u");
        ofn.lpstrFilter = _T("Playlist files (*.m3u)\0*.m3u\0\0");
    }
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lCustData = 0;
    if (!Save) {
        bResult = GetOpenFileName(&ofn);
    } else {
        bResult = GetSaveFileName(&ofn);
    }
    if (bResult) {
        return MPLAY_ERROR_SUCCESS;
    } else {
        if (CommDlgExtendedError() == 0) {
            return MPLAY_ERROR_USER_CANCEL;
        }
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
}

int
MplayRefreshTitle(LPTSTR szTitle)
{
    TCHAR szTemp[MAX_PATH];
    size_t newTitleLength;

    newTitleLength = _tcslen(_T(MPLAY_NAME)) +
        _tcslen(szTitle) +
               6; // ' ', '-', ' ', '[', ']', '\0' == 6
    if (newTitleLength >= sizeof(szTemp)/sizeof(szTemp[0])) {
        return MPLAY_ERROR_OVERFLOW;
    }

    _stprintf(szTemp,
        _T("%s - [%s]"),
        _T(MPLAY_NAME),
        szTitle);

    SetWindowText(GethWndMain(), szTemp);
    return MPLAY_ERROR_SUCCESS;
}

int
MplayAddTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA tnd;
    memset( &tnd, 0, sizeof(tnd));
    tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uID = NOTIFYICON_ID;
    tnd.cbSize = sizeof(tnd);
    tnd.hWnd = hWnd;
    tnd.uCallbackMessage = TRAY_CALLBACK;

    tnd.hIcon = (HICON)LoadImage(GethInst(),
        MAKEINTRESOURCE(IDI_ICOBLACK),
        IMAGE_ICON,
           16,
           16,
           LR_DEFAULTCOLOR);

    ASSERT( tnd.hIcon != NULL && tnd.hIcon != INVALID_HANDLE_VALUE );

    ASSERT( _tcslen(_T(MPLAY_NAME)) < sizeof(tnd.szTip)/sizeof(tnd.szTip[0]) );
    _tcscpy(tnd.szTip, _T(MPLAY_NAME));

    if (!Shell_NotifyIcon(NIM_ADD, &tnd)) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    return MPLAY_ERROR_SUCCESS;
}

int
MplayDelTrayIcon(HWND hWnd)
{
    NOTIFYICONDATA tnd;
    memset( &tnd, 0, sizeof(tnd));
    tnd.uFlags = NIF_MESSAGE|NIF_ICON;
    tnd.uID = NOTIFYICON_ID;
    tnd.cbSize = sizeof(tnd);
    tnd.hWnd = hWnd;

    if (!Shell_NotifyIcon(NIM_DELETE, &tnd)) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    return MPLAY_ERROR_SUCCESS;
}

static int
CvtToTime(LPTSTR szTemp,DWORD dwVal)
{
    ASSERT(szTemp != NULL);
    if (dwVal>(1000*60*60)) {
        //
        // Hours:minutes:seconds
        //
        _stprintf(szTemp,
            _T("%02i:%02i:%02i"),
            (int)(dwVal/3600000),
            (int)(dwVal%3600000/60000),
            (int)(dwVal%60000/1000));
    } else {
        //
        // Minutes:seconds
        //
        _stprintf(szTemp,
            _T("%02i:%02i"),
            (int)(dwVal%3600000/60000),
            (int)(dwVal%60000/1000));
    }
    return MPLAY_ERROR_SUCCESS;
}

int MplaySetTrackerLength(DWORD dwValue)
{
    HWND hWnd;
    DWORD tf=1;
    TCHAR szTime[32];
    if (dwValue > 30000) tf = 5;
    if (dwValue > 60000) tf = 15;
    if (dwValue > 240000) tf = 30;
    if (dwValue > 600000) tf = 60;

    hWnd = GethWndPtr();
    ASSERT(hWnd != NULL);
    //
    // According to MSDN, these can't fail.
    //
    SendMessage(hWnd, TBM_SETRANGE, TRUE, MAKELPARAM(0,dwValue/1000));
    SendMessage(hWnd, TBM_SETTICFREQ, tf, 0);

    CvtToTime(szTime,dwValue);
    hWnd = GethWndStatus();
    ASSERT(hWnd != NULL);
    SendMessage(hWnd, SB_SETTEXT, 0, (LPARAM)szTime);

    return MPLAY_ERROR_SUCCESS;
}

int 
MplaySetTrackerPos(DWORD dwValue)
{
    TCHAR szTime[32];
    HWND hWnd;

    hWnd=GethWndPtr();
    ASSERT(hWnd != NULL);
    //
    // According to MSDN, this can't fail.
    //
    SendMessage(hWnd, TBM_SETPOS, TRUE, dwValue/1000);

    CvtToTime(szTime, dwValue);
    hWnd = GethWndStatus();
    ASSERT(hWnd != NULL);
    SendMessage(hWnd, SB_SETTEXT, 1, (LPARAM)szTime);
    return MPLAY_ERROR_SUCCESS;
}

int 
MplayGetTrackerPos()
{
    HWND hWnd;
    hWnd=GethWndPtr();
    ASSERT(hWnd != NULL);
    return (int)SendMessage(hWnd, TBM_GETPOS, 0, 0) * 1000;
}

int 
MplaySetOpenFile(LPTSTR szFile)
{
    HWND hWnd;
    hWnd = GethWndStatus();
    ASSERT(hWnd != NULL);
    if (SendMessage(hWnd, SB_SETTEXT, 2, (LPARAM)szFile)) {
        return MPLAY_ERROR_SUCCESS;
    }
    return MPLAY_ERROR_UNKNOWN_FAILURE;
} 

static LONG
MplayAutoComplete(HWND hWnd, WPARAM wParam, DWORD * elength)
{
    TCHAR szTemp[200];
    TCHAR szTemp2[200];
    DWORD ul;

    if (HIWORD(wParam) == CBN_EDITUPDATE) {

        //
        //  Amd64 this is longer - so we have a 4Gb
        //  string limit here.
        //
        ul = (DWORD)SendDlgItemMessage(hWnd,
            LOWORD(wParam),
            WM_GETTEXTLENGTH,
            0,
            0);

        if (ul >= sizeof(szTemp)/sizeof(szTemp[0])) {
            return MPLAY_ERROR_OVERFLOW;
        }

        if (ul > *elength) {
            *elength = ul;

            GetDlgItemText(hWnd,
                LOWORD(wParam),
                szTemp,
                sizeof(szTemp));
    
            //
            //  Amd64 this is longer - so we have a 4Gb
            //  string limit here.
            //
            ul = (DWORD)SendDlgItemMessage(hWnd,
                LOWORD(wParam),
                CB_FINDSTRING,
                (WPARAM)-1,
                (LPARAM)szTemp);

            if (ul != CB_ERR) {
                //
                //  In theory, Win16 won't allow more than 32k items in a list;
                //  Win32 still needs to ensure you don't have >2g & <4g.
                //
                ASSERT( ul <= INT_MAX );

                SendDlgItemMessage(hWnd,
                    LOWORD(wParam),
                    CB_GETLBTEXT,
                    (WPARAM)ul,
                    (LPARAM)szTemp2);

                //
                //  We've done a case insensitive find.
                //  Preserve the case of the original
                //  text in the new buffer.
                //
                memcpy(szTemp2,
                    szTemp,
                    _tcslen(szTemp) * sizeof(TCHAR));

                SetDlgItemText(hWnd,
                    LOWORD(wParam),
                    szTemp2);

                SendDlgItemMessage(hWnd,
                    LOWORD(wParam),
                    CB_SETEDITSEL,
                    0,
                    MAKELPARAM(_tcslen(szTemp), _tcslen(szTemp2)));
            }
        } else {
            *elength = ul;
        }
    }
    return MPLAY_ERROR_SUCCESS;
}

#pragma pack(push, 4)
struct FindParam {
    LPTSTR szText;
    DWORD TextLen; // Length of buffer, in characters
    LIST* RecentItems;
    DWORD dwFlags;
};
#pragma pack(pop)

LONG APIENTRY
MplayFindDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    static struct FindParam * fp;
    DWORD i;
    static DWORD textlen = 0;
    static PVOID resizeInfo;

    if (ResizeDialogProc(hWnd, message, wParam, lParam, &resizeInfo)) {
        return TRUE;
    }

    switch(message) {
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_FINDTEXT:
            MplayAutoComplete(hWnd, wParam, &textlen);
            break;
        case IDOK:

            ASSERT( fp->TextLen > 0 );
            ASSERT( fp->szText != NULL );
            //
            //  Ensure we don't inadvertently use a long buffer, because
            //  the below call is limited to 32k on Win16.
            //
            ASSERT( fp->TextLen < INT_MAX );

            //
            //  This will truncate on insufficient space, which is usually
            //  acceptable for a find dialog.
            //
            GetDlgItemText(hWnd,
                IDC_FINDTEXT,
                fp->szText,
                (int)fp->TextLen);

            fp->dwFlags = 0;

            if (IsDlgButtonChecked(hWnd, IDC_MATCHCASE))
                fp->dwFlags |= FIND_MATCHCASE;
            if (IsDlgButtonChecked(hWnd, IDC_STARTTOP))
                fp->dwFlags |= FIND_STARTTOP;
            if (!IsDlgButtonChecked(hWnd, IDC_SEARCHPATH))
                fp->dwFlags |= FIND_SEARCHDISP;
            if (IsDlgButtonChecked(hWnd, IDC_INVERT))
                fp->dwFlags |= FIND_INVERT;

            EndDialog(hWnd, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hWnd, FALSE);
            break;
        }
        return TRUE;
        break;
    case WM_INITDIALOG:
        {
            LPTSTR Entry;
            fp = (struct FindParam *)lParam;
            ASSERT( fp->TextLen > 0 );
            ASSERT( fp->szText != NULL );
            if (fp->szText) {
                SetDlgItemText(hWnd, IDC_FINDTEXT, fp->szText);
            }
            for (i = 0; i < GetListLength(fp->RecentItems); i++) {

                Entry = GetListItemU(fp->RecentItems, i);

                SendDlgItemMessage(hWnd,
                    IDC_FINDTEXT,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)Entry);

            }
            if (fp->dwFlags & FIND_STARTTOP)
                CheckDlgButton(hWnd, IDC_STARTTOP, TRUE);
            if (fp->dwFlags & FIND_MATCHCASE)
                CheckDlgButton(hWnd, IDC_MATCHCASE, TRUE);
            if (fp->dwFlags & FIND_INVERT)
                CheckDlgButton(hWnd, IDC_INVERT, TRUE);
            if ((fp->dwFlags & FIND_SEARCHMASK) == FIND_SEARCHPATH)
                CheckDlgButton(hWnd, IDC_SEARCHPATH, TRUE);

            textlen = 0;
        }
        return TRUE;
        break;
    default:
        return FALSE;
        break;
    }

}


int
MplayFindEntry(LIST * RecentItems,LPDWORD dwFlags)
{
    struct FindParam fp;
    TCHAR szTextBuffer[256];
    LPTSTR PreviousItem;
    int DialogResult;
    int i;

    ASSERT( RecentItems != NULL );

    //
    // We need to copy at some point to a stable buffer whose
    // contents can really change.  The list item can't do that.
    //
    PreviousItem = GetListItemU(RecentItems, 0);
    if (PreviousItem) {
        if (_tcslen(PreviousItem) >= sizeof(szTextBuffer)/sizeof(szTextBuffer[0])) {
            return MPLAY_ERROR_OVERFLOW;
        }
        _tcscpy(szTextBuffer, PreviousItem);
    } else {
        _tcscpy(szTextBuffer, _T(""));
    }
    fp.szText = szTextBuffer;
    fp.TextLen = sizeof(szTextBuffer)/sizeof(szTextBuffer[0]);
    fp.RecentItems = RecentItems;
    fp.dwFlags = *dwFlags;
    DialogResult = (int)DialogBoxParam(GethInst(),
        _T("IDDFIND"),
        GethWndMain(),
        (DLGPROC)MplayFindDlgProc,
        (LPARAM)&fp);

    if (DialogResult < 0) {
        
        //
        // The system couldn't create the dialog for us.
        //

        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }

    if (DialogResult == 0) {
        return MPLAY_ERROR_USER_CANCEL;
    }

    i = MplayFindNextText(RecentItems,
        0,
        szTextBuffer,
        fp.dwFlags & FIND_RECENTMASK);

    while (i >= 0) {

        //
        // This search term has been used recently before.  Remove
        // the previous recent search term and add the new one.
        //

        MplayDeleteListItem(RecentItems, i);

        i = MplayFindNextText(RecentItems,
            0,
            szTextBuffer,
            fp.dwFlags & FIND_RECENTMASK);
    }

    MplayInsertListItem(RecentItems, szTextBuffer);

    //
    // Don't let the recent items list grow forever.  Delete the
    // maximum item from the list.  Should be harmless if the list
    // isn't that big yet.
    //

    if (GetListLength(RecentItems) > MAX_FIND_SIZE) {
        MplayDeleteListItem(RecentItems, MAX_FIND_SIZE);
    }
    *dwFlags = fp.dwFlags;
    return MPLAY_ERROR_SUCCESS;
}

int
MplayDispWindow(HWND hWnd, BOOL bDisp)
{
    //
    // We swallow all errors in this path.
    //

    if (bDisp) {
        ShowWindow(hWnd, SW_SHOWNORMAL);
        if (MplayIsPlaying() && MplayIsTracker())
            SetTimer(hWnd, TIMER_ID, TIMER_TICK, NULL);
        SetForegroundWindow(hWnd);
    } else {
        ShowWindow(hWnd, SW_HIDE);
        KillTimer(hWnd, TIMER_ID);
    }
    return MPLAY_ERROR_SUCCESS;
}


static HMENU
MplayBuildPopupMenu()
{
    HMENU hTmp;
    BOOL bResumable;
    HWND hWnd;
    hTmp = CreatePopupMenu();
    if (hTmp == NULL) return NULL;

    bResumable = FALSE;
    hWnd = GethWndMain();

    if (GetMenuState(GetMenu(hWnd), IDM_PAUSE, MF_BYCOMMAND) & MF_GRAYED) {
        bResumable = TRUE;
    }

    AppendMenu(hTmp, MF_STRING, IDM_PLAY, _T("&Play"));
    AppendMenu(hTmp, MF_STRING, IDM_STOP, _T("&Stop"));
    if (bResumable) {
        AppendMenu(hTmp,
            MF_STRING | MF_GRAYED,
            IDM_PAUSE,
            _T("P&ause"));

        AppendMenu(hTmp,
            MF_STRING,
            IDM_RESUME,
            _T("&Resume"));
    } else {
        AppendMenu(hTmp,
            MF_STRING,
            IDM_PAUSE,
            _T("P&ause"));

        AppendMenu(hTmp,
            MF_STRING | MF_GRAYED,
            IDM_RESUME,
            _T("&Resume"));
    }
    AppendMenu(hTmp, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTmp, MF_STRING, IDM_NEXT, _T("&Next"));
    AppendMenu(hTmp, MF_STRING, IDM_PREVIOUS, _T("Pr&evious"));
    AppendMenu(hTmp, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTmp, MF_STRING, IDM_SAVEOPTIONS, _T("Sa&ve options"));
    AppendMenu(hTmp, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTmp, MF_STRING, IDM_EXIT, _T("E&xit"));

    return hTmp;
}

void
MplayTrayCallback(HWND hWnd, WPARAM wParam, LPARAM lParam, MPLAY_OPTIONS * Options)
{
    if (wParam == NOTIFYICON_ID) {
        if (lParam == WM_LBUTTONDBLCLK) {
            Options->bHide = FALSE;
            MplayDispWindow(hWnd, TRUE);
        } else if (lParam == WM_RBUTTONDOWN) {
            POINT cursorpos;
            HMENU hPopupMenu;
            hPopupMenu = MplayBuildPopupMenu();
            GetCursorPos(&cursorpos);
            TrackPopupMenu(hPopupMenu,
                           TPM_RIGHTALIGN|TPM_BOTTOMALIGN,
                           cursorpos.x, //X
                           cursorpos.y, //Y
                           0, //Reserved
                           hWnd,
                           NULL);
            DestroyMenu(hPopupMenu);
        }
    }
}

void
MplayResizeCallback(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);

#ifdef BROKEN_MINIMIZE
    if (wParam == SIZEICONIC) {
        DispWindow(hWnd, FALSE);
        return;
    }
#endif

    //
    // This can be called before Window creation.  Ensure that we don't
    // start doing client sizing if the clients don't exist yet.
    //

    if (MplayIsTracker() && GethWndStatus() && GethWndPtr() && GethWndCli()) {
        RECT Rect;
        BOOL Success = TRUE;
        int WinHeight;
        Success = GetWindowRect(GethWndStatus(), &Rect);
        ASSERT( Success );
        WinHeight = Rect.bottom - Rect.top;
        Success = GetWindowRect(GethWndPtr(), &Rect);
        ASSERT( Success );
        dwPtrHeight = Rect.bottom - Rect.top;
        ASSERT( dwPtrHeight != 0 );

        MplayListUiResize( GethWndCli(),
            LOWORD(lParam),
            HIWORD(lParam) - dwPtrHeight - WinHeight - 2 * MARGIN );

        MoveWindow(GethWndPtr(),
            0,
            (int)(HIWORD(lParam) - dwPtrHeight - WinHeight - 1 * MARGIN),
            (int)LOWORD(lParam),
            (int)dwPtrHeight,
            TRUE);

        MoveWindow(GethWndStatus(),
            0,
            HIWORD(lParam) - WinHeight,
            LOWORD(lParam),
            WinHeight,
            TRUE);

        RedrawWindow(GethWndStatus(),
            NULL,
            NULL,
            RDW_ERASE | RDW_INVALIDATE);
    } else {
        MoveWindow(GethWndCli(),
            0,
            0,
            LOWORD(lParam),
            HIWORD(lParam),
            TRUE);
    }
}

int
MplaySetOnTop(HWND hWnd, BOOL bOnTop)
{
    if (bOnTop) {
        SetWindowPos(hWnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        CheckMenuItem(GetMenu(hWnd), IDM_ONTOP, MF_CHECKED);
    } else {
        SetWindowPos(hWnd,HWND_NOTOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        CheckMenuItem(GetMenu(hWnd), IDM_ONTOP, MF_UNCHECKED);
    }
    return MPLAY_ERROR_SUCCESS;
}

int
MplaySetTracker(HWND hWnd, BOOL bDispTrack)
{
    RECT Rect;

    if (!bDispTrack) {

        dwPtrHeight = 0;
        ShowWindow(GethWndPtr(), SW_HIDE);
        ShowWindow(GethWndStatus(), SW_HIDE);
        CheckMenuItem(GetMenu(hWnd),
            IDM_DISPTRACK,
            MF_UNCHECKED);
        if (MplayIsPlaying()) {
            KillTimer(hWnd, TIMER_ID);
        }

    } else {

        HDC hDC;
        hDC = GetDC(hWndCli);
        dwPtrHeight = MulDiv(PTR_SIZE, GetDeviceCaps(hDC, LOGPIXELSY), USER_DEFAULT_SCREEN_DPI);
        ReleaseDC(hWndCli, hDC);
        ShowWindow(GethWndPtr(), SW_SHOW);
        ShowWindow(GethWndStatus(), SW_SHOW);
        CheckMenuItem(GetMenu(hWnd),
            IDM_DISPTRACK,
            MF_CHECKED);
        if (MplayIsPlaying()) {
            SetTimer(hWnd, TIMER_ID, TIMER_TICK, NULL);
        }
        
    }
    GetClientRect(hWnd, &Rect);
    SendMessage(hWnd,
        WM_SIZE,
        0,
        MAKELPARAM(Rect.right, Rect.bottom));

    return MPLAY_ERROR_SUCCESS;
}

int
MplayUpdatePauseStatus(HWND hWnd, BOOL bResumable)
{
    if (bResumable) {
        EnableMenuItem(GetMenu(hWnd),
            IDM_RESUME,
            MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(GetMenu(hWnd),
            IDM_PAUSE,
            MF_BYCOMMAND | MF_GRAYED);
    } else {
        EnableMenuItem(GetMenu(hWnd),
            IDM_RESUME,
            MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(GetMenu(hWnd),
            IDM_PAUSE,
            MF_BYCOMMAND | MF_ENABLED);
    }
    return MPLAY_ERROR_SUCCESS;
}


int
MplayCreateAppWindows(HINSTANCE_T hInstance, int nCmdShow)
{
    int aWidths[3]={54,108,-1};
    HFONT hFont;
    int ScrWidth, ScrHeight;
    int WinWidth, WinHeight;
    RECT Rect;
    HDC hDC;
    NONCLIENTMETRICS SystemFontInfo;
    LPCTSTR szFontName;
    int nFontSize;

    UNREFERENCED_PARAMETER(nCmdShow);

    InitCommonControls();

    hDC = GetDC(hWndCli);
    dwPtrHeight = MulDiv(PTR_SIZE, GetDeviceCaps(hDC, LOGPIXELSY), USER_DEFAULT_SCREEN_DPI);
    ReleaseDC(hWndCli, hDC);

    ScrWidth = GetSystemMetrics(SM_CXSCREEN);
    ScrHeight = GetSystemMetrics(SM_CYSCREEN);

    if (ScrWidth <= 800 || ScrHeight <= 600) {
        WinWidth = 170;
        WinHeight = 200;
    } else if (ScrWidth <= 1024 || ScrHeight <= 768) {
        WinWidth = 200;
        WinHeight = 250;
    } else {
        WinWidth = 230;
        WinHeight = 320;
    }


    hWndMain = CreateWindowEx(
        0,
        _T("SerenityAudioPlayer"),
        _T(MPLAY_NAME),
        WS_OVERLAPPEDWINDOW,
        ScrWidth - WinWidth,
        0,
        WinWidth,
        WinHeight,
        (HWND)NULL,
        (HMENU)NULL,
        (HINSTANCE_T)hInstance,
        (LPVOID)NULL
    );

    if (hWndMain == NULL || hWndMain == INVALID_HANDLE_VALUE) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }

    hWndCli = MplayListUiCreate( hWndMain, hInstance );

    if (hWndCli == NULL || hWndCli == INVALID_HANDLE_VALUE) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }

    hWndPtr = CreateWindowEx(
        0,
        TRACKBAR_CLASS,
        _T(""),
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOP,
        0,
        MARGIN,
        PTR_SIZE,
        dwPtrHeight,
        (HWND)hWndMain,
        (HMENU)NULL,
        (HINSTANCE_T)hInstance,
        (LPVOID)NULL);

    if (hWndPtr == NULL || hWndPtr == INVALID_HANDLE_VALUE) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }

    ASSERT( GetWindowRect( hWndPtr, &Rect ) );
    ASSERT( Rect.bottom - Rect.top > 0 );


    hWndStatus = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        _T(""),
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0,
        2*MARGIN,
        100,
        20,
        (HWND)hWndMain,
        (HMENU)NULL,
        (HINSTANCE_T)hInstance,
        (LPVOID)NULL);
    if (hWndStatus == NULL || hWndStatus == INVALID_HANDLE_VALUE) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    SendMessage(hWndStatus, SB_SETPARTS, 3, (LPARAM)aWidths);

    ZeroMemory( &SystemFontInfo, sizeof(SystemFontInfo ));
    SystemFontInfo.cbSize = sizeof(SystemFontInfo);

    if (SystemParametersInfo( SPI_GETNONCLIENTMETRICS,
            sizeof(SystemFontInfo),
            &SystemFontInfo,
            0)) {

        hFont = CreateFontIndirect( &SystemFontInfo.lfMessageFont );

    } else {

        //
        //  We failed to determine the system font.  On debug builds, do
        //  something stupid.  On release builds, fall back to a sensible
        //  default.
        //

#ifdef DEBUG_MPLAY
        szFontName = _T("Times New Roman");
        nFontSize = 16;
#else
        szFontName = _T("MS Sans Serif");
        nFontSize = 8;
#endif

        hDC = GetDC(hWndCli);
        hFont = CreateFont(-MulDiv(nFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                0,0,0,0,0,0,0,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                0,
                szFontName);
        ReleaseDC(hWndCli, hDC);
    }
    
    SendMessage(hWndCli,
        WM_SETFONT,
        (WPARAM)hFont,
        MAKELPARAM(TRUE,0));

    GetClientRect(hWndMain, &Rect);

    SendMessage(hWndMain,
        WM_SIZE,
        0,
        MAKELPARAM(Rect.right, Rect.bottom));

    return MPLAY_ERROR_SUCCESS;
}


#ifdef INTERNAL


LPARAM APIENTRY
MplayPropertiesDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDOK:
        case 2:
            EndDialog(hWnd, TRUE);
            break;
        }
        return TRUE;
        break;
    case WM_INITDIALOG:
        {
            LPTSTR szTrack      = NULL;
            LPTSTR szTrackNo    = NULL;
            LPTSTR szSource     = NULL;
            LPTSTR szSourceType = NULL;
            LPTSTR szArtist     = NULL;
            TCHAR lpString[MAX_PATH];
            LPTSTR a;

            a = (LPTSTR)lParam;
            if (_tcslen(a) >= MAX_PATH) {
                return FALSE;
            }
            _tcscpy(lpString, a);
            szTrack = _tcsrchr(lpString, '\\');
            if (szTrack) {
                szTrack[0] = '\0';
                szTrack++;
                if (szTrack[0] == '[') {
                    if (szTrack[1] == 'I' || szTrack[1] == 'C') {
                        szSourceType = &szTrack[1];
                    } else {
                        szTrackNo = &szTrack[1];
                    }
                    szTrack = _tcschr(szTrack, ']');
                    if (szTrack) {
                        szTrack[0] = '\0';
                        szTrack++;
                    }
                }
                a = _tcsrchr(szTrack, '.');
                if (a) a[0] = '\0';
                a = _tcsrchr(lpString, '\\');
                if (a) {
                    a[0] = '\0';
                    a++;
                    if (a[0] == '[') {
                        szSourceType = &a[1];
                        szSource = _tcschr(a, ']');
                        if (szSource) {
                            szSource[0] = '\0';
                            szSource++;
                        }
                        a = _tcsrchr(lpString, '\\');
                        if (a) {
                            a++;
                            szArtist = a;
                        }
                    } else {
                        szArtist = a;
                    }
                }

            }

            SetDlgItemText(hWnd, IDC_TRACKNAME, szTrack);
            SetDlgItemText(hWnd, IDC_ARTIST, szArtist);
            SetDlgItemText(hWnd, IDC_SOURCE, szSource);
            SetDlgItemText(hWnd, IDC_TRACK, szTrackNo);
            SetDlgItemText(hWnd, IDC_SOURCETYPE, szSourceType);
            return TRUE;
        }
        break;
    default:
        return FALSE;
        break;
    }

}

#endif
