/*
 * Script.cpp:
 *
 * This file is responsible for interpreting scripts.
 *
 * Copyright (c) Malcolm Smith 2001-2017.  No warranty is provided.
 */

#include "pch.h"

#ifdef SCRIPTING

#define MAX_PARAM 16

static int
scrAddHandler(LIST * List, LPTSTR * szParam)
{
    LPTSTR a;
    UNREFERENCED_PARAMETER(List);
    a = _tcsrchr(szParam[0], '.');
    if (a) {
        a = a + 1;
    } else {
        a = szParam[0];
    }
    return MplayAddHandler(a, szParam[1]);
}


static int
scrClear(LIST * List, LPTSTR * szParam)
{
    UNREFERENCED_PARAMETER(szParam);
    
    return MplayClearList(List);
}

static int
scrClearHandlers(LIST * List, LPTSTR * szParam)
{
    UNREFERENCED_PARAMETER(List);
    UNREFERENCED_PARAMETER(szParam);

    return MplayResetHandlers();
}


static int
scrHide(LIST * List, LPTSTR * szParam)
{
    UNREFERENCED_PARAMETER(List);
    UNREFERENCED_PARAMETER(szParam);

    oOptions.bHide = TRUE;
    return MplayDispWindow(GethWndMain(), FALSE);
}


static int
scrImport(LIST * List, LPTSTR * szParam)
{
    HANDLE hFind;
    pi_find fd;
    if ((hFind = MplayFindFirst(szParam[0], &fd)) != INVALID_FIND_VALUE) {
        MplayFindClose(hFind);
        if (MplayGetAttr(&fd) & FA_SUBDIR) {
            return MplayGenerateTree(List, szParam[0]);
        } else {
            return MplayInputFile(List, szParam[0]);
        }
    }
    return MPLAY_ERROR_BAD_PATH;
}

static int
scrOntop(LIST * List, LPTSTR * szParam)
{
    BOOL onTopState = TRUE;

    UNREFERENCED_PARAMETER(List);

    if (szParam[0]) {
        if (!_tcsicmp(szParam[0], _T("false")))
            onTopState = FALSE;
    }
    oOptions.bOnTop = onTopState;
    return MplaySetOnTop(GethWndMain(), onTopState);
}


static int
scrPlay(LIST * List, LPTSTR * szParam)
{
    unsigned long ul = 0;

    if (szParam[0])
        ul = _ttoi(szParam[0]);
    return MplayPlayMediaFile(GethWndMain(), List, (int)ul);
}

static int
scrShow(LIST * List, LPTSTR * szParam)
{
    UNREFERENCED_PARAMETER(List);
    UNREFERENCED_PARAMETER(szParam);
    oOptions.bHide = FALSE;
    return MplayDispWindow(GethWndMain(), TRUE);
}


static int
scrShuffle(LIST * List, LPTSTR * szParam)
{
    UNREFERENCED_PARAMETER(szParam);
    return MplayShuffleList(List);
}

static int
scrSort(LIST * List, LPTSTR * szParam)
{
    int sorttype;
    if (!_tcscmp(szParam[0], _T("path"))) {
        sorttype = SORT_PATH;
    } else {
        if (!_tcscmp(szParam[0], _T("file"))) {
            sorttype = SORT_FILE;
        } else {
            sorttype = SORT_DISPLAY;
        }
    }
    return MplaySortList(List, sorttype);
}

static int
scrTracker(LIST * List, LPTSTR * szParam)
{
    BOOL trackerState = TRUE;

    UNREFERENCED_PARAMETER(List);

    if (szParam[0]) {  
        if (!_tcscmp(szParam[0],_T("false")))
            trackerState = FALSE;
    }

    oOptions.bDisplayTracker = trackerState;
    return MplaySetTracker(GethWndMain(), trackerState);
}

#define MAX_LINE_LENGTH 1024

static int
MplayScrReadLine( FILE * fp, TCHAR * szLine, DWORD NumChars )
{
#ifdef UNICODE
    CHAR szRawLine[MAX_LINE_LENGTH];
    int err;
#endif
    LPTSTR a;

    ASSERT( NumChars >= MAX_LINE_LENGTH );

    //
    //  Always read with fgets; Unicode needs to manually
    //  translate, non-Unicode is passthrough
    //
#ifdef UNICODE
    if (fgets(szRawLine, sizeof(szRawLine), fp) == NULL) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
    //
    //  On Unicode builds, we treat the script
    //  as being in UTF8 encoding.  Translate this
    //  back into Unicode so we never need to worry
    //  about how big each character is ever again
    //  (until the user saves it, of course.)
    //
    err = MultiByteToWideChar( CP_UTF8,
        0,
        szRawLine,
        -1,
        szLine,
        NumChars);

    //
    //  Since szRawLine is x chars and szLine is x
    //  wchars, we should never be able to overflow
    //  here.  Note that since a character may use
    //  one or two bytes in the source, it is not
    //  guaranteed that the same size buffer is
    //  needed.
    //
    ASSERT( err > 0 );
    if (err == 0) {
        return MPLAY_ERROR_OVERFLOW;
    }
#else
    if (fgets(szLine, NumChars, fp) == NULL) {
        return MPLAY_ERROR_UNKNOWN_FAILURE;
    }
#endif
    a = _tcschr(szLine, '\r');
    if (a) a[0] = '\0';
    a = _tcschr(szLine, '\n');
    if (a) a[0] = '\0';
    return MPLAY_ERROR_SUCCESS;
}

int
MplayPlayScript(LIST * List, LPTSTR szFile)
{
    FILE * fp;
    TCHAR szLine[MAX_LINE_LENGTH];
    LPTSTR szParam[MAX_PARAM];
    LPTSTR a;
    int i;
    int Error;

    fp = _tfopen(szFile, _T("r"));
    if (!fp) {
        return MPLAY_ERROR_BAD_PATH;
    }

    Error = MplayScrReadLine( fp,
        szLine,
        sizeof( szLine )/sizeof( szLine[0] ));


    while (Error == MPLAY_ERROR_SUCCESS) {
        a = _tcschr(szLine,'#');
        if (a) a[0] = '\0';
        if (_tcslen(szLine)) {
            memset(szParam, 0, sizeof(szParam));
            a = _tcschr(szLine, ' ');
            if (a) {
                a[0] = '\0';
                szParam[0] = a + 1;
                i = 0;
                a = _tcschr(szParam[i], ',');
                while (a && (i < MAX_PARAM)) {
                    a[0] = '\0';
                    i++;
                    szParam[i] = a + 1;
                    a = _tcschr(szParam[i], ',');
                }

            }
            
            if (!_tcscmp(szLine, _T("AddHandler"))) scrAddHandler(List, szParam);
            if (!_tcscmp(szLine, _T("Clear"))) scrClear(List, szParam);
            if (!_tcscmp(szLine, _T("ClearHandlers"))) scrClearHandlers(List, szParam);
            if (!_tcscmp(szLine, _T("Hide"))) scrHide(List, szParam);
            if (!_tcscmp(szLine, _T("Import"))) scrImport(List, szParam);
            if (!_tcscmp(szLine, _T("OnTop"))) scrOntop(List, szParam);
            if (!_tcscmp(szLine, _T("Play"))) scrPlay(List, szParam);
            if (!_tcscmp(szLine, _T("Show"))) scrShow(List, szParam);
            if (!_tcscmp(szLine, _T("Shuffle"))) scrShuffle(List, szParam);
            if (!_tcscmp(szLine, _T("Sort"))) scrSort(List, szParam);
            if (!_tcscmp(szLine, _T("Tracker"))) scrTracker(List, szParam);
        }

        Error = MplayScrReadLine( fp,
            szLine,
            sizeof( szLine )/sizeof( szLine[0] ));

    }
    fclose(fp);
    return MPLAY_ERROR_SUCCESS;
}

#endif


