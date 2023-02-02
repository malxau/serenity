// Build.h

#define MPLAY_STRING_INT(str) #str
#define MPLAY_STRING(str) MPLAY_STRING_INT(str) 

#define MPLAY_VER_STRING \
	MPLAY_STRING(MPLAY_VER_MAJOR) "." \
	MPLAY_STRING(MPLAY_VER_MINOR) "." \
	MPLAY_STRING(MPLAY_VER_MICRO)

#ifdef BROKEN_MINIMIZE
#define MPLAY_NAME "Serenity Audio Player (BROKEN_MINIMIZE)"
#else
#define MPLAY_NAME "Serenity Audio Player"
#endif

#define MPLAY_COPYRIGHT "Copyright (c) Malcolm Smith 2001-2023"

#if MPLAY_BUILD_ID!=0
#define PRERELEASE
#endif

#ifdef _CYGWIN
#define _WIN32
#endif

#define GROWTH_INCREMENT 1000
#define MAX_FIND_SIZE    0x10

//
// Suppress warning building Unicode on VS2005+.
//
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _CRT_SECURE_NO_WARNINGS 1



