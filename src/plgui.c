/*
 * Plgui.cpp:
 *
 * This file is a port of my playlist generator, and is responsible for
 * manipulating playlists.
 *
 * Copyright (c) Malcolm Smith 2001-2017.  No warranty is provided.
 */

#include "pch.h"

static int
MplayExtendListIfNecessary(LIST * List)
{
    ASSERT( List != NULL );

    if (List->dwItemsInList == List->dwItemsAllocated) {
        List->dwItemsAllocated += GROWTH_INCREMENT;
        GlobalUnlock( List->hAlloc );
        List->hAlloc = GlobalReAlloc(List->hAlloc,
            (size_t)(List->dwItemsAllocated)*sizeof(LISTENTRY),
            GMEM_MOVEABLE);
        List->entrylist = (LISTENTRY*)GlobalLock(List->hAlloc);
        if (List->hAlloc == NULL) {
            return MPLAY_ERROR_NO_MEMORY;
        }
    }
    return MPLAY_ERROR_SUCCESS;
}

static int
MplaySetListItem(LIST * List, DWORD dwItem, LPTSTR szItem)
{
    ASSERT( List != NULL );
    ASSERT( szItem != NULL );
    ASSERT( dwItem < List->dwItemsInList );
    if (List->entrylist[dwItem].hAlloc != NULL) {
        GlobalFree(List->entrylist[dwItem].hAlloc);
    }
    List->entrylist[dwItem].hAlloc = GlobalAlloc(GMEM_MOVEABLE,
            (_tcslen(szItem)+1)*sizeof(TCHAR));
    if (List->entrylist[dwItem].hAlloc == NULL) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    List->entrylist[dwItem].data = (LPTSTR)GlobalLock(List->entrylist[dwItem].hAlloc);
    _tcscpy(List->entrylist[dwItem].data,szItem);
    return MPLAY_ERROR_SUCCESS;
}


static int
MplayAddListItem(LIST * List, LPTSTR szItem)
{
    int err;
    ASSERT( List != NULL );
    ASSERT( szItem != NULL );
    err = MplayExtendListIfNecessary(List);
    if (err != MPLAY_ERROR_SUCCESS) {
        return err;
    }
    List->entrylist[List->dwItemsInList].hAlloc = NULL;
    List->dwItemsInList++;
    err = MplaySetListItem(List, List->dwItemsInList-1, szItem);
    if (err != MPLAY_ERROR_SUCCESS) {
        List->dwItemsInList--;
    }
    return err;
}

int
MplayInsertListItem(LIST * List, LPTSTR szItem)
{
    int err;
    ASSERT( List != NULL );
    ASSERT( szItem != NULL );
    err = MplayExtendListIfNecessary(List);
    if (err != MPLAY_ERROR_SUCCESS) {
        return err;
    }
    if (List->dwItemsInList > 0) {
        memmove(&List->entrylist[1],
            &List->entrylist[0],
            sizeof(LISTENTRY) * (size_t)(List->dwItemsInList));
    }
    List->entrylist[0].hAlloc = NULL;
    List->dwItemsInList++;
    err = MplaySetListItem(List, 0, szItem);
    if (err != MPLAY_ERROR_SUCCESS) {
        List->dwItemsInList--;
        memmove(&List->entrylist[0],
            &List->entrylist[1],
            sizeof(LISTENTRY) * (size_t)(List->dwItemsInList));
    }
    return err;
}

int
MplaySwapListItems(LIST * List,
    DWORD dwItem1,
    DWORD dwItem2)
{
    LISTENTRY temp;

    ASSERT( List != NULL );

    ASSERT( dwItem1 < List->dwItemsInList );
    ASSERT( dwItem2 < List->dwItemsInList );
    ASSERT( dwItem1 != dwItem2 );

    temp=List->entrylist[dwItem1];
    List->entrylist[dwItem1]=List->entrylist[dwItem2];
    List->entrylist[dwItem2]=temp;
    return MPLAY_ERROR_SUCCESS;
}

static int
MplayDeleteListEntry(LIST * List, DWORD dwItem)
{
    ASSERT( List != NULL );
    ASSERT( dwItem < List->dwItemsInList );
    ASSERT(List->entrylist[dwItem].hAlloc != NULL);

    GlobalUnlock(List->entrylist[dwItem].hAlloc);
    GlobalFree(List->entrylist[dwItem].hAlloc);

    List->entrylist[dwItem].hAlloc = NULL;
    return MPLAY_ERROR_SUCCESS;
}

int
MplayDeleteListItem(LIST * List,
    DWORD dwItem)
{
    int err;
    ASSERT( List != NULL );
    ASSERT( dwItem < List->dwItemsInList );

    if (dwItem>=List->dwItemsInList)
        return MPLAY_ERROR_TRACK_OUT_OF_RANGE;

    err = MplayDeleteListEntry( List, dwItem );
    //
    // We shouldn't really fail this class of operation.
    //
    ASSERT( err == MPLAY_ERROR_SUCCESS );
    if (err != MPLAY_ERROR_SUCCESS) {
        return err;
    }

    List->dwItemsInList--;
    if (List->dwItemsInList>0) {
        memmove(&List->entrylist[dwItem],
            &List->entrylist[dwItem+1],
            sizeof(LISTENTRY)*(size_t)(List->dwItemsInList-dwItem));
    }
    return MPLAY_ERROR_SUCCESS;
}


int
MplayGenerateTree(LIST * List, LPTSTR szPath)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    HANDLE FindHandle;
    pi_find FindData;
    int err;

    ASSERT( List != NULL );
    ASSERT( szPath != NULL );

    _stprintf(szTemp, _T("%s\\*.*"), szPath);
    FindHandle = MplayFindFirst(szTemp, &FindData);
    if (FindHandle != INVALID_FIND_VALUE) {
        do {
            _stprintf(szTemp,
                _T("%s\\%s"),
                szPath,
                MplayGetName(&FindData));
            if (MplayGetAttr(&FindData) & FA_SUBDIR) {
                if (MplayGetName(&FindData)[0] != '.') {
                    err = MplayGenerateTree(List,
                        szTemp);
                    if (err != MPLAY_ERROR_SUCCESS) {
                        return err;
                    }
                }
            } else {  
                GetFullPathName(szTemp,
                    sizeof(szTemp2)/sizeof(TCHAR),
                    szTemp2,
                    NULL);
                err = MplayInputFile(List, szTemp2);
                if (err != MPLAY_ERROR_SUCCESS) {
                    return err;
                }
            }
        } while (MplayFindNext(FindHandle, &FindData) != INVALID_FIND_VALUE);
        MplayFindClose(FindHandle);
    }
    return MPLAY_ERROR_SUCCESS;
}

int MplayShuffleList(LIST * List)
{
    DWORD i,j;
    if (List->dwItemsInList == 0) return MPLAY_ERROR_TRACK_OUT_OF_RANGE;

    for (i=0; i < List->dwItemsInList; i++) {
        j = ((DWORD)List->dwItemsInList - 1) * rand() / RAND_MAX;
        ASSERT( j < List->dwItemsInList );
        if (j != i) {
            MplaySwapListItems(List,i,j);
        }
    }

    return MPLAY_ERROR_SUCCESS;
}

#pragma pack(push, 4)
struct SearchData {
    LPTSTR szFileName;
    DWORD dwItem;
};
#pragma pack(pop)

int
MplaySortList(LIST * List, int iCriteria)
{
    DWORD BestItem;
    DWORD i,j;
    LPTSTR a;
    DWORD li;
    struct SearchData * lpSD;
    LIST NewList;
    HANDLE hSD;

    struct SearchData sdTmp;
    
    li=List->dwItemsInList;

    if (li<2) return MPLAY_ERROR_TRACK_OUT_OF_RANGE;

    hSD = GlobalAlloc(GMEM_MOVEABLE,
        li*sizeof(struct SearchData));

    if (hSD == NULL) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    lpSD = (struct SearchData *)GlobalLock(hSD);
    if (lpSD == NULL) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    for (i=0;i<li;i++) {
        a=GetListItemU(List, i);
        ASSERT( a != NULL );
        a = MplayGetPtr(a,iCriteria);
        ASSERT( a != NULL );

        lpSD[i].szFileName=a;
        lpSD[i].dwItem=i;
    }

    for (i=0;i<(li-1);i++) {
        BestItem=i;
        for (j = (i+1); j < li; j++) 
            if (_tcsicmp(lpSD[j].szFileName,lpSD[BestItem].szFileName)<0)
                BestItem=j;
        
        if (BestItem != i) {
            memcpy(&sdTmp, &lpSD[BestItem], sizeof(struct SearchData));
            memcpy(&lpSD[BestItem], &lpSD[i], sizeof(struct SearchData));
            memcpy(&lpSD[i], &sdTmp, sizeof(struct SearchData));
        }
    }    

    NewList.hAlloc = GlobalAlloc(GMEM_MOVEABLE, sizeof(LISTENTRY)*li);
    if (NewList.hAlloc == NULL) {
        GlobalUnlock(hSD);
        GlobalFree(hSD);
        return MPLAY_ERROR_NO_MEMORY;
    }
    NewList.entrylist = (LISTENTRY*)GlobalLock(NewList.hAlloc);    

    for (i=0;i<li;i++) {
        NewList.entrylist[i].hAlloc = List->entrylist[lpSD[i].dwItem].hAlloc;
        NewList.entrylist[i].data = List->entrylist[lpSD[i].dwItem].data;
    }

    GlobalUnlock(List->hAlloc);
    GlobalFree(List->hAlloc);

    List->hAlloc = NewList.hAlloc;
    List->entrylist = NewList.entrylist;
    List->dwItemsAllocated = li;
    List->dwItemsInList = li;

    GlobalUnlock(hSD);
    GlobalFree(hSD);
    return MPLAY_ERROR_SUCCESS;
}



int
MplayWriteList(LIST * List, LPTSTR szFile)
{
    HANDLE hFile;
    DWORD i;
    DWORD dwBytesWritten;
    LPSTR szUtf8ListItem = NULL;
    DWORD dwCurrentItemLength;
#ifdef UNICODE
    DWORD dwAllocatedItemLength = 0;
#endif

    hFile = CreateFile(szFile,
                       GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_DELETE,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return MPLAY_ERROR_BAD_PATH;
    }
    for (i=0;i<List->dwItemsInList;i++) {
#ifdef UNICODE
        //
        //  On Unicode builds, we want to preserve extended
        //  characters when saving.  We do this by using UTF8
        //  as an output format for playlists.  This preserves
        //  compatibility with UTF7, while allowing us to
        //  store more information in the file.  (Apologies
        //  to all users of Latin-1 M3U files.)  It'd be
        //  great if we could record what encoding we are
        //  using when writing these.
        //
        dwCurrentItemLength = WideCharToMultiByte( CP_UTF8,
            0,
            GetListItemU( List, i ),
            -1,
            NULL,
            0,
            NULL,
            NULL );

        //
        //  If the above function fails, it returns zero.
        //
        //  Add one for NULL.
        //
        ASSERT( dwCurrentItemLength != 0 );
        dwCurrentItemLength++;

        //
        //  Allocate a new buffer if necessary.
        //
        if (dwCurrentItemLength > dwAllocatedItemLength) {
            dwAllocatedItemLength = dwCurrentItemLength;
            if (szUtf8ListItem != NULL) {
                free( szUtf8ListItem );
            }
            szUtf8ListItem = malloc( dwAllocatedItemLength );
        }

        if (szUtf8ListItem == NULL) {
            return MPLAY_ERROR_NO_MEMORY;
        }

        //
        //  Convert to UTF8.
        //
        dwCurrentItemLength = WideCharToMultiByte( CP_UTF8,
            0,
            GetListItemU( List, i ),
            -1,
            szUtf8ListItem,
            dwCurrentItemLength,
            NULL,
            NULL );
        ASSERT( dwCurrentItemLength != 0 );
        szUtf8ListItem[dwCurrentItemLength] = '\0';

        WriteFile(hFile, szUtf8ListItem, dwCurrentItemLength, &dwBytesWritten, NULL);
        WriteFile(hFile, "\n", 1, &dwBytesWritten, NULL);
#else
        szUtf8ListItem = GetListItemU(List, i);
        WriteFile(hFile, szUtf8ListItem, _tcslen(szUtf8ListItem), &dwBytesWritten, NULL);
        WriteFile(hFile, "\n", 1, &dwBytesWritten, NULL);
        szUtf8ListItem = NULL;
#endif
    }
#ifdef UNICODE
    if (szUtf8ListItem != NULL) {
        free( szUtf8ListItem );
    }
#endif
    
    CloseHandle(hFile);
    return MPLAY_ERROR_SUCCESS;
}

int
MplayReadList(LIST * List, LPTSTR szFile)
{
#ifdef UNICODE
    TCHAR szUniTemp[MAX_PATH * sizeof(WCHAR)];
#endif
    CHAR szTemp[MAX_PATH * sizeof(WCHAR)];
    int err;
    LPSTR a;
    FILE * fp;

    fp = _tfopen(szFile, _T("r"));
    if (fp == NULL) {
        return MPLAY_ERROR_BAD_PATH;
    }

    while (fgets( szTemp, sizeof(szTemp), fp )) {
        if (szTemp[0] != '\0') {

            // 
            //  If we see a newline, remove it.
            //
            a = strchr(szTemp, '\r');
            if (a) a[0] = '\0';
            a = strchr(szTemp, '\n');
            if (a) a[0] = '\0';
            
#ifdef UNICODE
            //
            //  On Unicode builds, we treat the playlist
            //  as being in UTF8 encoding.  Translate this
            //  back into Unicode so we never need to worry
            //  about how big each character is ever again
            //  (until the user saves it, of course.)
            //
            err = MultiByteToWideChar( CP_UTF8,
                0,
                szTemp,
                -1,
                szUniTemp,
                sizeof(szUniTemp)/sizeof(szUniTemp[0]));

            //
            //  Since szTemp is x chars and szUniTemp is x
            //  wchars, we should never be able to overflow
            //  here.  Note that since a character may use
            //  one or two bytes in the source, it is not
            //  guaranteed that the same size buffer is
            //  needed.
            //
            ASSERT( err > 0 );
            if (err == 0) {
                fclose( fp );
                return MPLAY_ERROR_OVERFLOW;
            }

            //
            //  Note that we may still have a string
            //  longer than MAX_PATH at this point.  We'll
            //  still add it to the list, but we won't be
            //  able to play it.
            //
            err = MplayAddListItem(List, szUniTemp);
#else
            err = MplayAddListItem(List, szTemp);
#endif
            if (err != MPLAY_ERROR_SUCCESS) {
                return err;
            }
        }
    }
    fclose(fp);
    return MPLAY_ERROR_SUCCESS;
}

int 
MplayInitList(LIST * List)
{
    List->hAlloc = GlobalAlloc(GMEM_MOVEABLE,
        GROWTH_INCREMENT * sizeof(LISTENTRY));
    if (List->hAlloc == NULL) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    List->entrylist=(LISTENTRY*)GlobalLock(List->hAlloc);
    if (List->entrylist == NULL) {
        return MPLAY_ERROR_NO_MEMORY;
    }
    List->dwItemsAllocated = GROWTH_INCREMENT;
    List->dwItemsInList = 0;
    return MPLAY_ERROR_SUCCESS;
}

int
MplayFreeList(LIST * List)
{
    ASSERT(List->hAlloc != NULL);
    GlobalUnlock(List->hAlloc);
    GlobalFree(List->hAlloc);
    List->hAlloc = NULL;
    List->entrylist = NULL;
    List->dwItemsInList = 0;
    List->dwItemsAllocated = 0;
    return MPLAY_ERROR_SUCCESS;
}


int
MplayInputFile(LIST * List, LPTSTR szFile)
{
    TCHAR szExt[MAX_PATH];
    LPTSTR a;
    ASSERT( List != NULL );
    ASSERT( szFile != NULL );
    if (szFile[0] == '\0') {
        return MPLAY_ERROR_BAD_PATH;
    }

    //
    //  The system can't handle anything >MAX_PATH, so check for it here and
    //  fail rather than overflow our own internal buffers
    //

    if (_tcslen(szFile) >= MAX_PATH) {
        return MPLAY_ERROR_OVERFLOW;
    }

    a=_tcsrchr(szFile,'.');
    if (a) {
        _tcscpy(szExt,a+1);
        _tcsupr(szExt);
    } else {
        _tcscpy(szExt, _T(""));
    }

#ifdef SCRIPTING
    if (!_tcscmp(szExt, _T("MSC"))) {
        return MplayPlayScript(List,szFile);
    }
#endif
    if (!_tcscmp(szExt, _T("M3U"))) {
        return MplayReadList(List, szFile);
    }
    return MplayAddListItem(List,szFile);
}

int
MplayClearList(LIST * List)
{
    DWORD i;
    int err;
    ASSERT( List != NULL );

    for (i=0;i<List->dwItemsInList;i++) {
        err = MplayDeleteListEntry( List, i );
        ASSERT( err == MPLAY_ERROR_SUCCESS );
        if (err != MPLAY_ERROR_SUCCESS) {
            //
            // Panic.  We're kinda screwed now.  We clean up the list as
            // best we can and get out.  Fortunately, DeleteListEntry
            // should not fail, and neither should ClearList.
            //
            if (i != 0) {
                memmove(&List->entrylist[0],
                    &List->entrylist[i],
                    sizeof(LISTENTRY)*(size_t)(List->dwItemsInList - i));
                List->dwItemsInList -= i;
            }
            return err;
        }
    }

    //
    // FIXME: Move this?
    //
    MplayCloseStream(GethWndMain());
    List->dwItemsInList=0;
    return MPLAY_ERROR_SUCCESS;
}

LPTSTR
MplayGetPtr(LPTSTR szPath, DWORD dwTo)
{
    LPTSTR a,b;
    a=szPath;

    if ((dwTo == SORT_FILE) || (dwTo == SORT_DISPLAY)) {
        if ((b = _tcsrchr(a,'\\')) != NULL) {
            a = b + 1;
        }
        if (dwTo == SORT_DISPLAY) {
            if (a[0]=='[') {
                if ((b = _tcschr(a,']')) != NULL) {
                    a = b + 1;
                }
            }
        }
    }

    ASSERT(a != NULL);
    return(a);
}

int
MplayFindNextText(LIST * List,
    DWORD dwStart,
    LPTSTR szText,
    DWORD dwFlags)
{
    int dwFound=-1;
    TCHAR szTemp[MAX_PATH];
    LPTSTR a;
    DWORD i;
    DWORD dwPtr;

    ASSERT( List != NULL );
    ASSERT( szText != NULL );
    dwPtr = (dwFlags & FIND_SEARCHMASK) / FIND_SEARCHDIV;
    if (dwFlags & FIND_STARTTOP) dwStart=0;
    if (!(dwFlags & FIND_MATCHCASE)) {
        _tcslwr(szText);
    }
    if (dwStart >= List->dwItemsInList) return(-1);
    for (i=dwStart;
        ((dwFound < 0) && (i < List->dwItemsInList));
        (dwFlags & FIND_BACKWARDS)?i--:i++) {
        
        a=GetListItemU(List,i);
        a=MplayGetPtr(a,dwPtr);
        
        if (!(dwFlags&FIND_MATCHCASE)) {
            _tcscpy(szTemp,a);
            a=szTemp;
            _tcslwr(a);
        }
        if (!(dwFlags&FIND_INVERT)) {
            if (_tcsstr(a,szText)) dwFound=(int)i;
        } else {
            if (!_tcsstr(a,szText)) dwFound=(int)i;
        }
    }
    if (dwFound>=0) {
        return(dwFound);
    } else {
        return(-1);
    }
}

