# Serenity Audio Player
Serenity Audio Player is a simple, clean and lightweight audio player. It uses Windows codecs without a hefty interface.

## License

This software is licensed under the terms of the GPL Version 2 license.

## System Requirements

To run Serenity binaries, you need Windows NT 4 or newer. Note that Windows 95/98/Me are not supported by precompiled binaries.

To compile from source, you need Visual C++, version 4 or newer. Free versions of Visual C++ are included in the Visual C++ Build Tools, Windows SDK 7.1 (2010), or Windows SDK 7.0 (2008).

## Build instructions

1. Open a command prompt to your version of Visual C++ and set up your environment.
2. Unpack and enter the serenity source tree.
3. Most of the time, just run "nmake".

### Compilation options

Compilation options can be used by passing arguments to NMAKE.

| Option | Definition |
| ------ | ---------- |
| DEBUG= | Enable debug code. Valid values are 0 (disabled) or 1 (enabled.) Default is 0. |
| MINICRT= | Compile against Minicrt rather than Msvcrt. Valid values are 0 (disabled) or 1 (enabled.) If 1, minicrt.h and minicrt.lib must be in %INCLUDE% and %LIB% respectively. An alternative way to use Minicrt is to extract it into a crt subdirectory within the serenity directory, and it will be used automatically. Default is 0.
| MSVCRT_DLL= | Compile against the shared, DLL C runtime library, or the static version. Valid values are 0 (static C runtime), or 1 (shared C runtime.) Default is 1. |
| UNICODE= | Compile a Unicode or ANSI version of the binary. Unicode is more useful on NT, but ANSI is required to run on Windows 95. Valid values are 1 (Unicode) or 0 (ANSI.) Default is 1. |

