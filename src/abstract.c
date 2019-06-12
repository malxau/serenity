/*
 * Abstract.cpp:
 *
 * This file is historic.  It used to abstract Win32/Win16 when Win16 was
 * supported.  Now it contains Win32 code only.
 *
 * Copyright (c) Malcolm Smith 2001-2008.  No warranty is provided.
 */

#include "pch.h"

HANDLE
MplayFindFirst(LPCTSTR szFile, pi_find * fileinfo)
{
	HANDLE ret;
	ret = FindFirstFile(szFile, fileinfo);
	if (ret == INVALID_HANDLE_VALUE) {
		return INVALID_FIND_VALUE;
	} else {
		return ret;
	}
}

HANDLE
MplayFindNext(HANDLE hFind, pi_find * fileinfo)
{
	BOOL ret;
	ret = FindNextFile(hFind, fileinfo);
	if (!ret) {
		return INVALID_FIND_VALUE;
	} else {
		return NULL;
	}
}

BOOL
MplayFindClose(HANDLE hFind)
{
	return FindClose(hFind);
}

unsigned int
MplayGetAttr(pi_find * fileinfo)
{
	return fileinfo->dwFileAttributes;
}

LPTSTR
MplayGetName(pi_find * fileinfo)
{
	return fileinfo->cFileName;
}

