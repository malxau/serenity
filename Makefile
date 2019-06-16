

MPLAY_VER_MAJOR=3
MPLAY_VER_MINOR=4
MPLAY_VER_MICRO=1
MPLAY_BUILD_ID=0

ROOTNAME=serenity

all: $(ROOTNAME).exe

DEBUG=0
MSVCRT_DLL=1
PDB=0
UNICODE=1
BROKEN_MINIMIZE=0
WITH_INTERNAL=0
WITH_SCRIPTING=1

LINK=link.exe
MAKE=nmake.exe -nologo

CFLAGS=-nologo -W3 -Gs99999 -I.
LDFLAGS=-fixed -release
OBJS=abstract.obj app.obj debug.obj inter.obj media.obj msupd.obj plgui.obj script.obj ResizeDialog.obj
LIBS=advapi32.lib comctl32.lib comdlg32.lib gdi32.lib kernel32.lib shell32.lib user32.lib winmm.lib

#
# If minicrt.h exists, do something which will fail, and
# if it fails, that means the file is there and we should
# use minicrt.  Note that we don't do the inverse so
# setting MINICRT=1 in the environment also works.
#

!IF [if exist crt\minicrt.h echo yes|find "no" >NUL]>0
MINICRT=1
!ENDIF

!IF $(DEBUG)==1
CFLAGS=$(CFLAGS) -Zi -Od -DDEBUG_MPLAY
LDFLAGS=$(LDFLAGS) -DEBUG
RCFLAGS=$(RCFLAGS) -DDEBUG
!ELSE
CFLAGS=$(CFLAGS) -GFy -O1syi
!ENDIF

!IF $(PDB)==1
CFLAGS=$(CFLAGS) -Zi -Fdserenity.pdb
LDFLAGS=$(LDFLAGS) /DEBUG /OPT:REF /Pdb:serenity.pdb
!ENDIF

#
# Set the correct entrypoint depending on whether we're
# ANSI or Unicode.
#

!IF $(UNICODE)==1
CFLAGS=$(CFLAGS) -DUNICODE -D_UNICODE
ENTRY=wmainCRTStartup
!ELSE
ENTRY=mainCRTStartup
!ENDIF

#
# Include and link to the desired CRT.
#

!IF "$(MINICRT)"=="1"
CFLAGS=$(CFLAGS) -DMINICRT
LDFLAGS=$(LDFLAGS) -nodefaultlib
CRTLIB=minicrt.lib
DEPS=minicrt.lib
!ELSE
!IF $(MSVCRT_DLL)==1
CFLAGS=$(CFLAGS) -MD
CRTLIB=msvcrt.lib
!ELSE
CFLAGS=$(CFLAGS) -MT
CRTLIB=libcmt.lib
!ENDIF
!ENDIF

!IF "$(BROKEN_MINIMIZE)" == "1"
CFLAGS=$(CFLAGS) -DBROKEN_MINIMIZE
!ENDIF

!IF "$(WITH_SCRIPTING)" == "1"
CFLAGS=$(CFLAGS) -DSCRIPTING
!ENDIF

!IF "$(WITH_INTERNAL)" == "1"
CFLAGS=$(CFLAGS) -DINTERNAL
RCFLAGS=$(RCFLAGS) /d INTERNAL
!ENDIF

#
# Probe for more esoteric aspects of compiler behavior.
# Apparently pragma doesn't cut it for -GS-, and this is
# the most critical one for Minicrt.  MP is a perf
# optimization that only exists on newer compilers, so
# skip the probe on old ones.
#

!IF [$(CC) -GS- 2>&1 | find "unknown" >NUL]>0
CFLAGS=$(CFLAGS) -GS-
CFLAGS=$(CFLAGS) -GF
!IF [$(CC) -? 2>&1 | find "/MP" >NUL]==0
CFLAGS=$(CFLAGS) -MP
!ENDIF
!ELSE
!IF [$(CC) -GF 2>&1 | find "unknown" >NUL]>0
CFLAGS=$(CFLAGS) -GF
!ELSE
CFLAGS=$(CFLAGS) -Gf
!ENDIF
!ENDIF

#
# Probe for stdcall support.  This should exist on all x86
# compilers, but not x64 compilers.
#

!IF [$(CC) -? 2>&1 | find "Gz __stdcall" >NUL]==0
CFLAGS=$(CFLAGS) -Gz
!ENDIF

#
# Really old compiler support, when link.exe was
# link32.exe and didn't support -nologo
#

!IF [$(LINK) 2>&1 | find "Microsoft" >NUL]>0
!IF [link32.exe 2>&1 | find "Microsoft" >NUL]==0
LINK=link32.exe
!IF "$(MINICRT)"!="1"
!IF $(MSVCRT_DLL)==1
CRTLIB=crtdll.lib
!ENDIF
!ENDIF
!ENDIF
!ELSE
LINK=$(LINK) -nologo -incremental:no

#
# Visual C 2003 really wants this and isn't satisfied
# with a pragma, so yet another probe
#

!IF [$(LINK) -OPT:NOWIN98 2>&1 | find "NOWIN98" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:NOWIN98
!ENDIF
!ENDIF

!IF "$(MINICRT)"=="1"

#
# Try to detect which architecture the compiler is generating.  This is done
# to help the subsystem version detection, below.  If we don't guess it's
# not fatal, the linker will figure it out in the end.
#

ARCH=win32
!IF [$(CC) 2>&1 | find "for x86" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X86
!ELSE
!IF [$(CC) 2>&1 | find "for x64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X64
ARCH=amd64
!ELSE
!IF [$(CC) 2>&1 | find "for AMD64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:AMD64
ARCH=amd64
!ENDIF
!ENDIF
!ENDIF

#
# Look for the oldest subsystem version the linker is willing to generate
# for us.  This app with minicrt will run back to NT 3.1, but requires OS
# support added in 4.0, but typically the primary limit is the linker.
# Without minicrt we're at the mercy of whatever CRT is there, so just
# trust the defaults.  For execution efficiency, these checks are
# structured as a tree:
#
#                             5.0?
#                           /      \
#                         /          \
#                       /              \
#                   4.0?                5.2?
#                 /      \            /      \
#              4.0         5.0    5.1?         6.0?
#                                /    \       /    \
#                              5.1    5.2   6.0   default
#

!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:WINDOWS,5.0 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:WINDOWS,4.0 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS,4.0
!ELSE  # !4.0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS,5.0
!ENDIF # 4.0
!ELSE  # !5.0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:WINDOWS,5.2 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:WINDOWS,5.1 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS,5.1
!ELSE  # !5.1
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS,5.2
!ENDIF # 5.1
!ELSE  # !5.2
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:WINDOWS,6.0 2>&1 | find "default subsystem" >NUL]>0
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS
!ENDIF # 6.0
!ENDIF # 5.2
!ENDIF # 5.0
!ELSE  # MINICRT
LDFLAGS=$(LDFLAGS) -SUBSYSTEM:WINDOWS
!ENDIF

#
# We could also probe for -GF and -incremental:no,
# although they typically are only warnings and
# probes are expensive, so we don't.
#

minicrt.lib:
	@if exist crt ( cd crt & $(MAKE) & cd .. & copy crt\minicrt.lib . & copy crt\minicrt.h . )

clean:
	if exist *.cab erase *.cab
	if exist *.exe erase *.exe
	if exist *.obj erase *.obj
	if exist *.pdb erase *.pdb
	if exist *.lib erase *.lib
	if exist *.res erase *.res
	if exist *.aps erase *.aps
	if exist *.pch erase *.pch
	if exist *.ilk erase *.ilk
	if exist *.map erase *.map
	if exist *~ erase *~
	if exist src\*~ erase src\*~
	if exist *.exe.manifest erase *.exe.manifest
	if exist minicrt.h erase minicrt.h
	if exist crt ( cd crt & $(MAKE) clean & cd .. )

.SUFFIXES: .c .rc .res .obj .exe

!IFDEF _NMAKE_VER
{src}.c.obj::
!ELSE
{src}.c.obj:
!ENDIF
	@$(CC) $(CFLAGS) -DMPLAY_VER_MAJOR=$(MPLAY_VER_MAJOR) -DMPLAY_VER_MINOR=$(MPLAY_VER_MINOR) -DMPLAY_VER_MICRO=$(MPLAY_VER_MICRO) -DMPLAY_BUILD_ID=$(MPLAY_BUILD_ID) -DWIN32_NO_CONFIG_H -c $<

{src}.rc.obj:
	@echo resource.rc
	@$(RC) /d MPLAY_VER_MAJOR=$(MPLAY_VER_MAJOR) /d MPLAY_VER_MINOR=$(MPLAY_VER_MINOR) /d MPLAY_VER_MICRO=$(MPLAY_VER_MICRO) /d MPLAY_BUILD_ID=$(MPLAY_BUILD_ID) -r $(RCFLAGS) $<
	@if exist resource.obj erase resource.obj
	@if not exist resource.obj move src\resource.res resource.obj

$(ROOTNAME).exe: $(OBJS) resource.obj $(DEPS)
	@echo $(ROOTNAME).exe
	@$(LINK) $(LDFLAGS) $(OBJS) resource.obj /out:$(ROOTNAME).exe $(LIBS) $(CRTLIB)

SHIPPDB=
!IF $(DEBUG)>0
SHIPPDB=/DSHIPPDB
!ENDIF

PACKARCH=/DPACKARCH=-win32

MNUNICODE=
!IF $(UNICODE)==1
MNUNICODE=/DUNICODE
!ENDIF

distribution: all
	@makensis /V1 $(PACKARCH) $(SHIPPDB) $(MNUNICODE) install.nsi
	@yori -c ypm -c serenity-$(ARCH).cab serenity $(MPLAY_VER_MAJOR).$(MPLAY_VER_MINOR).$(MPLAY_VER_MICRO) $(ARCH) -filelist pkg\serenity.lst -minimumosbuild 1381 -upgradepath http://www.malsmith.net/download/?obj=serenity/latest-stable/serenity-$(ARCH).cab -symbolpath http://www.malsmith.net/download/?obj=serenity/latest-stable/serenity-pdb-$(ARCH).cab -sourcepath http://www.malsmith.net/download/?obj=serenity/latest-stable/serenity-source.cab
	@yori -c ypm -c serenity-pdb-$(ARCH).cab serenity-pdb $(MPLAY_VER_MAJOR).$(MPLAY_VER_MINOR).$(MPLAY_VER_MICRO) $(ARCH) -filelist pkg\serenity-pdb.lst -upgradepath http://www.malsmith.net/download/?obj=serenity/latest-stable/serenity-pdb-$(ARCH).cab -sourcepath http://www.malsmith.net/download/?obj=serenity/latest-stable/serenity-source.cab
	@yori -c ypm -cs serenity-source.cab serenity-source $(MPLAY_VER_MAJOR).$(MPLAY_VER_MINOR).$(MPLAY_VER_MICRO) -filepath .

