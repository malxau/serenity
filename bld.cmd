@echo off

if %MINICRT%. NEQ 1. (
    echo MINICRT is not defined, bad idea for a release
    exit /b
)

set BLDID=%1
if %BLDID%.==. (
    set /p BLDID=Enter the date to use for the build: 
)

if %BLDID%.==. (
    echo Preparing for release build.  Ctrl+C if not intended.
    pause
    set RELDIR=out-rel
    set BLDID=0
) else (
    set RELDIR=out-%BLDID%
)

mkdir %RELDIR%
erase /q %RELDIR%\* >NUL

nmake /nologo clean >NUL
nmake /nologo PDB=1 MINICRT=1 UNICODE=1 MPLAY_BUILD_ID=%BLDID% distribution
mkdir %RELDIR%\win32-unicode
copy serenity.exe %RELDIR%\win32-unicode\serenity.exe
for %%i in (serenity-win32-installer*.exe) do copy %%i %RELDIR%\win32-unicode\serenity-installer.exe
copy serenity.pdb %RELDIR%\win32-unicode\serenity.pdb

:done

zip %RELDIR%\serenity-source.zip COPYING Makefile *.nsi src\*

nmake /nologo clean >NUL
