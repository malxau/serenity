/*
 * MSUPD.C
 *
 * Code to update a file from the internet including the running
 * executable.
 *
 * Copyright (c) 2016 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <tchar.h>
#ifdef MINICRT
#include <minicrt.h>
#else
#include <stdio.h>
#endif

#include "msupd.h"

#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 4
#endif

#ifndef _stprintf_s
#define _stprintf_s _sntprintf
#endif

LPCTSTR
UpdErrorStrings[] = {
    _T("Success"),
    _T("Could not initialize WinInet"),
    _T("Could not connect to server"),
    _T("Could not read data from server"),
    _T("Data read from server is incorrect"),
    _T("Could not write data to local file"),
    _T("Could not replace existing file with new file")
};

BOOL
WINAPI
UpdateBinaryFromFile(
    LPTSTR ExistingPath,
    LPTSTR NewPath
    )
{
    TCHAR MyPath[MAX_PATH];
    TCHAR OldPath[MAX_PATH];
    LPTSTR PathToReplace;
    HANDLE hMyBinary;

    if (ExistingPath == NULL) {

        //
        //  If the file name to replace is NULL, replace the currently
        //  existing binary.
        //
    
        if (GetModuleFileName(NULL, MyPath, sizeof(MyPath)/sizeof(MyPath[0])) == 0) {
            return FALSE;
        }

        PathToReplace = MyPath;
    } else {
        LPTSTR FinalBackslash;

        //
        //  If the file name to replace is a full path, defined as containing
        //  a backslash, replace that file path.
        //

        FinalBackslash = _tcsrchr(ExistingPath, '\\');

        if (FinalBackslash != NULL) {
            PathToReplace = ExistingPath;
        } else {

            //
            //  If it's a file name only, assume that it refers to a file in
            //  the same path as the existing binary.
            //

            if (GetModuleFileName(NULL, MyPath, sizeof(MyPath)/sizeof(MyPath[0])) == 0) {
                return FALSE;
            }

            FinalBackslash = _tcsrchr(MyPath, '\\');
            if (FinalBackslash != NULL) {
                DWORD RemainingLength = (DWORD)(sizeof(MyPath)/sizeof(MyPath[0]) - (FinalBackslash - MyPath + 1));
                if ((DWORD)_tcslen(ExistingPath) >= RemainingLength) {
                    return FALSE;
                }
                _stprintf_s(FinalBackslash + 1, RemainingLength, _T("%s"), ExistingPath);
                PathToReplace = MyPath;
            } else {
                PathToReplace = ExistingPath;
            }
        }
    }

    //
    //  Create a temporary name to hold the existing binary.
    //

    if (_tcslen(PathToReplace) + 4 >= sizeof(OldPath) / sizeof(OldPath[0])) {
        return FALSE;
    }
    _stprintf_s(OldPath, sizeof(OldPath)/sizeof(OldPath[0]), _T("%s.old"), PathToReplace);

    //
    //  Rename the existing binary to temp.  If this process has
    //  been performed before and is incomplete the temp may exist,
    //  but we can just clobber it with the current version.
    //

    if (!MoveFileEx(PathToReplace, OldPath, MOVEFILE_REPLACE_EXISTING)) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            return FALSE;
        }
    }

    //
    //  Rename the new binary to where the old binary was.
    //  If it fails, try to move the old binary back.  If
    //  that fails, there's not much we can do.
    //

    if (!MoveFileEx(NewPath, PathToReplace, MOVEFILE_COPY_ALLOWED)) {
        MoveFile(OldPath, PathToReplace);
        return FALSE;
    }

    //
    //  Try to delete the old binary.  Do this by opening a delete
    //  on close handle and not closing it.  The close will occur
    //  when the process terminates, which hopefully means it won't
    //  conflict with the open that's running the program right now.
    //
    //  If this fails just leave the old binary around.  Note next
    //  time this process is run it is overwritten.
    //

    hMyBinary = CreateFile(OldPath,
                           DELETE,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_DELETE_ON_CLOSE,
                           NULL);


    return TRUE;
}

typedef
LPVOID
WINAPI
INTERNET_OPEN(
    LPCTSTR szAgent,
    DWORD dwAccessType,
    LPCSTR szProxyName,
    LPCSTR szProxyBypass,
    DWORD dwFlags);

typedef INTERNET_OPEN *PINTERNET_OPEN;

typedef
LPVOID
WINAPI
INTERNET_OPEN_URL(
    LPVOID hInternet,
    LPCTSTR szUrl,
    LPCTSTR szHeaders,
    DWORD dwHeaderLength,
    DWORD dwFlags,
    DWORD dwContext);

typedef INTERNET_OPEN_URL *PINTERNET_OPEN_URL;

typedef
BOOL
WINAPI
HTTP_QUERY_INFO(
    LPVOID hInternet,
    DWORD dwLevel,
    LPVOID Buffer,
    LPDWORD BufferLength,
    LPDWORD Index
    );

typedef HTTP_QUERY_INFO *PHTTP_QUERY_INFO;

typedef
BOOL
WINAPI
INTERNET_READ_FILE(
    LPVOID hInternet,
    LPVOID pBuffer,
    DWORD BytesToRead,
    LPDWORD BytesRead
    );

typedef INTERNET_READ_FILE *PINTERNET_READ_FILE;

typedef
BOOL
WINAPI
INTERNET_CLOSE_HANDLE(
    LPVOID hInternet
    );

typedef INTERNET_CLOSE_HANDLE *PINTERNET_CLOSE_HANDLE;

#define UPDATE_READ_SIZE (1024 * 1024)

UpdError
WINAPI
UpdateBinaryFromUrl(
    LPTSTR Url,
    LPTSTR TargetName,
    LPTSTR Agent
    )
{
    PVOID hInternet = NULL;
    PVOID NewBinary = NULL;
    PUCHAR NewBinaryData = NULL;
    DWORD ErrorBufferSize = 0;
    DWORD ActualBinarySize;
    TCHAR TempName[MAX_PATH];
    TCHAR TempPath[MAX_PATH];
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    BOOL SuccessfullyComplete = FALSE;
    HINSTANCE hWinInet = NULL;
    DWORD dwError;
    UpdError Return = UpdErrorSuccess;

    PINTERNET_OPEN InetOpen;
    PINTERNET_OPEN_URL InetOpenUrl;
    PINTERNET_READ_FILE InetReadFile;
    PINTERNET_CLOSE_HANDLE InetCloseHandle;
    PHTTP_QUERY_INFO HttpQueryInfo;

    //
    //  Dynamically load WinInet.  This means we don't have to resolve
    //  imports unless we're really using it for something, and we can
    //  degrade gracefully if it's not there (original 95/NT.)
    //

    hWinInet = LoadLibrary(_T("WinInet.dll"));
    if (hWinInet == NULL) {
        Return = UpdErrorInetInit;
        return FALSE;
    }

#ifdef _UNICODE
    InetOpen = (PINTERNET_OPEN)GetProcAddress(hWinInet, "InternetOpenW");
    InetOpenUrl = (PINTERNET_OPEN_URL)GetProcAddress(hWinInet, "InternetOpenUrlW");
    HttpQueryInfo = (PHTTP_QUERY_INFO)GetProcAddress(hWinInet, "HttpQueryInfoW");
#else
    InetOpen = (PINTERNET_OPEN)GetProcAddress(hWinInet, "InternetOpenA");
    InetOpenUrl = (PINTERNET_OPEN_URL)GetProcAddress(hWinInet, "InternetOpenUrlA");
    HttpQueryInfo = (PHTTP_QUERY_INFO)GetProcAddress(hWinInet, "HttpQueryInfoA");
#endif
    InetReadFile = (PINTERNET_READ_FILE)GetProcAddress(hWinInet, "InternetReadFile");
    InetCloseHandle = (PINTERNET_CLOSE_HANDLE)GetProcAddress(hWinInet, "InternetCloseHandle");

    if (InetOpen == NULL ||
        InetOpenUrl == NULL ||
        HttpQueryInfo == NULL ||
        InetReadFile == NULL ||
        InetCloseHandle == NULL) {

        Return = UpdErrorInetInit;
        goto Exit;
    }

    //
    //  Open an internet connection with default proxy settings.
    //

    hInternet = InetOpen(Agent,
                         0,
                         NULL,
                         NULL,
                         0);

    if (hInternet == NULL) {
        Return = UpdErrorInetInit;
        goto Exit;
    }

    //
    //  Request the desired URL and check the status is HTTP success.
    //

    NewBinary = InetOpenUrl(hInternet, Url, NULL, 0, 0, 0);
    if (NewBinary == NULL) {
        Return = UpdErrorInetConnect;
        goto Exit;
    }

    ErrorBufferSize = sizeof(dwError);
    ActualBinarySize = 0;
    dwError = 0;
    if (!HttpQueryInfo(NewBinary, 0x20000013, &dwError, &ErrorBufferSize, &ActualBinarySize)) {
        Return = UpdErrorInetConnect;
        goto Exit;
    }

    if (dwError != 200) {
        Return = UpdErrorInetConnect;
        goto Exit;
    }

    //
    //  Create a temporary file to hold the contents.
    //

    if (GetTempPath(sizeof(TempPath)/sizeof(TempPath[0]), TempPath) == 0) {
        Return = UpdErrorFileWrite;
        goto Exit;
    }

    if (GetTempFileName(TempPath, _T("UPD"), 0, TempName) == 0) {
        Return = UpdErrorFileWrite;
        goto Exit;
    }

    hTempFile = CreateFile(TempName,
                           FILE_WRITE_DATA|FILE_READ_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (hTempFile == INVALID_HANDLE_VALUE) {
        Return = UpdErrorFileWrite;
        goto Exit;
    }

    NewBinaryData = HeapAlloc(GetProcessHeap(), 0, UPDATE_READ_SIZE);
    if (NewBinaryData == NULL) {
        Return = UpdErrorFileWrite;
        goto Exit;
    }

    //
    //  Read from the internet location and save to the temporary file.
    //

    while (InetReadFile(NewBinary, NewBinaryData, 1024 * 1024, &ActualBinarySize)) {

        DWORD DataWritten;

        if (ActualBinarySize == 0) {
            SuccessfullyComplete = TRUE;
            break;
        }

        if (!WriteFile(hTempFile, NewBinaryData, ActualBinarySize, &DataWritten, NULL) ||
            DataWritten != ActualBinarySize) {

            Return = UpdErrorFileWrite;
            goto Exit;
        }
    }

    //
    //  The only acceptable reason to fail is all the data has been
    //  received.
    //

    if (!SuccessfullyComplete) {
        Return = UpdErrorInetRead;
        goto Exit;
    }

    //
    //  For validation, if the request is to modify the current executable
    //  check that the result is an executable.
    //

    if (TargetName == NULL) {
        SetFilePointer(hTempFile, 0, NULL, FILE_BEGIN);
        if (!ReadFile(hTempFile, NewBinaryData, 2, &ActualBinarySize, NULL) ||
            ActualBinarySize != 2 ||
            NewBinaryData[0] != 'M' ||
            NewBinaryData[1] != 'Z' ) {

            Return = UpdErrorInetContents;
            goto Exit;
        }
    }

    //
    //  Now update the binary with the local file.
    //

    CloseHandle(hTempFile);
    HeapFree(GetProcessHeap(), 0, NewBinaryData);

    if (UpdateBinaryFromFile(TargetName, TempName)) {
        return UpdErrorSuccess;
    } else {
        return UpdErrorFileReplace;
    }

Exit:

    if (NewBinaryData != NULL) {
        HeapFree(GetProcessHeap(), 0, NewBinaryData);
    }

    if (hTempFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hTempFile);
        DeleteFile(TempName);
    }

    if (NewBinary != NULL) {
        InetCloseHandle(NewBinary);
    }

    if (hInternet != NULL) {
        InetCloseHandle(hInternet);
    }

    if (hWinInet != NULL) {
        FreeLibrary(hWinInet);
    }

    return Return;
}

LPCTSTR
WINAPI
UpdateErrorString(
    UpdError Error
    )
{
    if (Error < UpdErrorMax) {
        return UpdErrorStrings[Error];
    }
    return _T("Not an update error");
}
