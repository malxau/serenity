
#ifdef DEBUG_MPLAY

#include "pch.h"

#if defined(_WIN32) && !defined(_WIN64) && defined(DEBUG_STACKTRACE)
#define DBG_STACK_SUPPORT
#ifdef IMGHLP
#include <imagehlp.h>
#else
#include <dbghelp.h>
#endif

static VOID
DbgGenerateStack(LPTSTR szStack)
{
    BOOL bRet;
    STACKFRAME StackFrame;
    union {
        IMAGEHLP_SYMBOL i;
        CHAR Padding[256];
    } HumanFrame;
    TCHAR szLine[300];
    DWORD FrameNumber = 0;
    DWORD FnDisp = 0;
    CONTEXT c;
#ifndef IMGHLP
    DWORD LnDisp;
    IMAGEHLP_LINE HumanLocation;
#endif
    CHAR szModule[MAX_PATH];

    memset(&StackFrame, 0, sizeof(StackFrame));

    memset( &c, 0, sizeof(c) );

    c.ContextFlags = CONTEXT_FULL;
    GetThreadContext( GetCurrentThread(), &c );
    
    StackFrame.AddrPC.Offset = c.Eip;
    StackFrame.AddrFrame.Offset = c.Ebp;
    StackFrame.AddrPC.Mode = AddrModeFlat;
    StackFrame.AddrFrame.Mode = AddrModeFlat;

    bRet = SymInitialize( GetCurrentProcess(),
        NULL,
        FALSE );
    if (!bRet) DebugBreak();

    bRet = GetModuleFileNameA( NULL,
        szModule,
        sizeof(szModule));
    if (!bRet) DebugBreak();
    
    bRet = SymLoadModule( GetCurrentProcess(),
        0,
        szModule,
        NULL,
        0,
        0 );
    if (!bRet) DebugBreak();

    bRet = StackWalk( IMAGE_FILE_MACHINE_I386,
        GetCurrentProcess(),
        GetCurrentThread(),
        &StackFrame,
        &c,
        NULL,
        SymFunctionTableAccess,
        SymGetModuleBase,
        NULL);

    if (!bRet) {
        SymCleanup(GetCurrentProcess());
        return;
    }

    while (bRet) {

        //
        // Don't display the first two stack frames; they're used to
        // generate the assert itself.
        //
        if (FrameNumber++ >= 0) {
            memset(&HumanFrame, 0, sizeof(HumanFrame));

            HumanFrame.i.SizeOfStruct = sizeof(HumanFrame.i);
            HumanFrame.i.MaxNameLength = sizeof(HumanFrame.Padding) - sizeof(HumanFrame.i);
            bRet = SymGetSymFromAddr(GetCurrentProcess(),
                StackFrame.AddrPC.Offset,
                &FnDisp,
                &HumanFrame.i);
            if (!bRet) {
                strcpy(HumanFrame.i.Name, "<unknown>");
                FnDisp = 0;
            }

#ifndef IMGHLP
            HumanLocation.SizeOfStruct = sizeof(HumanLocation);

            bRet = SymGetLineFromAddr(GetCurrentProcess(),
                StackFrame.AddrPC.Offset,
                &LnDisp,
                &HumanLocation);

            if (!bRet) {
                HumanLocation.FileName = "<unknown>";
                HumanLocation.LineNumber = 0;
            }
#endif

            _stprintf(szLine, _T("%08x %08x %08x %hs+0x%x")
#ifndef IMGHLP
                _T(" [%hs @ %i]")
#endif
                _T("\n"),
                StackFrame.AddrPC.Offset,
                StackFrame.AddrFrame.Offset,
                StackFrame.AddrReturn.Offset,
                HumanFrame.i.Name,
                FnDisp
#ifndef IMGHLP
                ,HumanLocation.FileName
                ,HumanLocation.LineNumber
#endif
                );
            _tcscat(szStack, szLine);

        }

        bRet = StackWalk( IMAGE_FILE_MACHINE_I386,
            GetCurrentProcess(),
            GetCurrentThread(),
            &StackFrame,
            &c,
            NULL,
            SymFunctionTableAccess,
            SymGetModuleBase,
            NULL);
    }

    SymCleanup(GetCurrentProcess());
}
#endif

//
//  We output to three places: the debugger, stdout, and UI.  This is
//  because we can't know what type of application is running here:
//  this code may be in a DLL used by different types of applications.
//

VOID DbgRealAssert( LPCTSTR szCondition, LPCTSTR szFunction, LPCTSTR szFile, DWORD dwLine ) {
    TCHAR szTemp[1024];  // A stack trace can be big
    DWORD oldGLE = 0;

    //
    // Save away LE.  MessageBox et al will mess with it.
    //
    oldGLE = GetLastError();
    if (szFunction[0]!='\0') {
        _stprintf(szTemp,
            _T("ASSERTION FAILURE: %s\n%s %s:%i\nGLE: %i\n\n"),
            szCondition,
            szFunction,
            szFile,
            dwLine,
            oldGLE);
    } else {
        _stprintf(szTemp,
            _T("ASSERTION FAILURE: %s\n%s:%i\nGLE: %i\n\n"),
            szCondition,
            szFile,
            dwLine,
            oldGLE);
    }
#ifdef DBG_STACK_SUPPORT
    DbgGenerateStack(szTemp);
#endif
    OutputDebugString(szTemp);
    _ftprintf(stderr, szTemp);

    //
    // IsDebuggerPresent requires NT4 or Win98 or above.  The below
    // line should be IsDebuggerPresent, but isn't until I feel game
    // without Win95 support.
    //
#if defined(WINVER) && (WINVER >= 0x0401) && 0
    if (IsDebuggerPresent()) {
#else
    if (FALSE) {
#endif
        DebugBreak();
    } else {
        TCHAR szTemp2[2048];
        int mbresult;
        _stprintf(szTemp2,
            _T("%s\n\nWould you like to break into the debugger?"),
            szTemp);
        mbresult = MessageBox(NULL,
            szTemp2,
            _T("Assertion Failure"),
            MB_YESNOCANCEL);
        switch(mbresult) {
            case IDYES:
                DebugBreak();
                break;
            case IDNO:
                break;
            case IDCANCEL:
                ExitProcess(0);
                break;
        }
    }
    SetLastError(oldGLE);
}
#endif
