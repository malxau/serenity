!include "Sections.nsh"

; The name of the installer
Name "Serenity Audio Player"
SetCompressor LZMA

!define /date BUILDDATE "%Y%m%d"

!IFNDEF PACKARCH
!define PACKARCH "-win32"
!ENDIF

!IFDEF SHIPPDB
!define DEBUGTAG "-debug"
!ELSE
!define DEBUGTAG ""
!ENDIF

!IFDEF UNICODE
!define UNICODETAG "-unicode"
!ELSE
!define UNICODETAG ""
!ENDIF

; The file to write
OutFile "serenity${PACKARCH}-installer${UNICODETAG}${DEBUGTAG}-${BUILDDATE}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Serenity

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Serenity Audio Player" "Install_Dir"

;--------------------------------

; Pages

Page license
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

LicenseText "This software is licensed under the GPL, which attaches some conditions to modification and distribution (but not to use.)  Please read the full text for these conditions."
LicenseData "COPYING"

;--------------------------------

RequestExecutionLevel user

; The stuff to install
Section "Serenity (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Install to per-user start menu so we don't require admin privs
  ; Note admin is still required for add/remove programs support
  SetShellVarContext current

  ; Put file there
  File "serenity.exe"
!IFDEF SHIPPDB
  File "serenity.pdb"
!ENDIF
  
  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\Serenity Audio Player" "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SerenityAudio" "DisplayName" "Serenity Audio Player"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SerenityAudio" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SerenityAudio" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SerenityAudio" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  CreateDirectory "$SMPROGRAMS\Serenity Audio Player"
  CreateShortCut "$SMPROGRAMS\Serenity Audio Player\Serenity Audio Player.lnk" "$INSTDIR\serenity.exe" "" "$INSTDIR\serenity.exe" 0

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Install to per-user start menu so we don't require admin privs
  ; Note admin is still required for add/remove programs support
  SetShellVarContext current

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SerenityAudio"
  DeleteRegKey HKLM "SOFTWARE\Serenity Audio Player"

  ; Remove files and uninstaller
  Delete $INSTDIR\serenity.exe
!IFDEF SHIPPDB
  Delete $INSTDIR\serenity.pdb
!ENDIF
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Serenity Audio Player\Serenity Audio Player.lnk"
  Delete "$SMPROGRAMS\Serenity Audio Player\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Serenity Audio Player"
  RMDir "$INSTDIR"

SectionEnd

