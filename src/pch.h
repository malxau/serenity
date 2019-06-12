/*
 * pch.h:
 *
 * Precompiled header accelerator.
 *
 * Copyright (c) Malcolm Smith 2001-2010.  No warranty is provided.
 */

#include "build.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RC_INVOKED
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <mmsystem.h>
#ifdef MINICRT
#include <tchar.h>
#include <minicrt.h>
#if !defined(MINICRT_VER) || MINICRT_VER < 0x0001000a
#error "Building with minicrt requires minicrt 1.10 or newer"
#endif
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <tchar.h>
#endif
#endif

#include "ResizeDialog.h"
#include "msupd.h"

//
//  Visual C++ 4 doesn't define this, but Visual C++ 5 does.  Since
//  we're only expecting Unicode to function correctly on NT, this
//  implies that NT4 should work. I haven't confirmed this though.
//
#ifdef UNICODE
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#endif

#ifndef DWORD_PTR
#ifndef _WIN64
typedef DWORD DWORD_PTR;
#endif
#endif
#include "mpassert.h"
#include "import.h"

#pragma warning( error: 4701 4189 4706 ) // 4127

