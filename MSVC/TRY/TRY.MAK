# Microsoft Developer Studio Generated NMAKE File, Based on try.dsp
!IF "$(CFG)" == ""
CFG=try - Win32 Debug
!MESSAGE No configuration specified. Defaulting to try - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "try - Win32 Release" && "$(CFG)" != "try - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "try.mak" CFG="try - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "try - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "try - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "try - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\try.exe" "$(OUTDIR)\try.bsc"


CLEAN :
	-@erase "$(INTDIR)\Mxt.obj"
	-@erase "$(INTDIR)\Mxt.sbr"
	-@erase "$(INTDIR)\Try.obj"
	-@erase "$(INTDIR)\Try.sbr"
	-@erase "$(INTDIR)\Vars-standalone.obj"
	-@erase "$(INTDIR)\Vars-standalone.sbr"
	-@erase "$(INTDIR)\Varsa.obj"
	-@erase "$(INTDIR)\Varsa.sbr"
	-@erase "$(INTDIR)\Varsp.obj"
	-@erase "$(INTDIR)\Varsp.sbr"
	-@erase "$(INTDIR)\Varss.obj"
	-@erase "$(INTDIR)\Varss.sbr"
	-@erase "$(INTDIR)\Varsv.obj"
	-@erase "$(INTDIR)\Varsv.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\Win32.obj"
	-@erase "$(OUTDIR)\try.bsc"
	-@erase "$(OUTDIR)\try.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\try.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Mxt.sbr" \
	"$(INTDIR)\Try.sbr" \
	"$(INTDIR)\Vars-standalone.sbr" \
	"$(INTDIR)\Varsa.sbr" \
	"$(INTDIR)\Varsp.sbr" \
	"$(INTDIR)\Varss.sbr" \
	"$(INTDIR)\Varsv.sbr"

"$(OUTDIR)\try.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\try.pdb" /machine:I386 /out:"$(OUTDIR)\try.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Mxt.obj" \
	"$(INTDIR)\Try.obj" \
	"$(INTDIR)\Vars-standalone.obj" \
	"$(INTDIR)\Varsa.obj" \
	"$(INTDIR)\Varsp.obj" \
	"$(INTDIR)\Varss.obj" \
	"$(INTDIR)\Varsv.obj" \
	"$(INTDIR)\Win32.obj"

"$(OUTDIR)\try.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "try - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\try.exe" "$(OUTDIR)\try.bsc"


CLEAN :
	-@erase "$(INTDIR)\Mxt.obj"
	-@erase "$(INTDIR)\Mxt.sbr"
	-@erase "$(INTDIR)\Try.obj"
	-@erase "$(INTDIR)\Try.sbr"
	-@erase "$(INTDIR)\Vars-standalone.obj"
	-@erase "$(INTDIR)\Vars-standalone.sbr"
	-@erase "$(INTDIR)\Varsa.obj"
	-@erase "$(INTDIR)\Varsa.sbr"
	-@erase "$(INTDIR)\Varsp.obj"
	-@erase "$(INTDIR)\Varsp.sbr"
	-@erase "$(INTDIR)\Varss.obj"
	-@erase "$(INTDIR)\Varss.sbr"
	-@erase "$(INTDIR)\Varsv.obj"
	-@erase "$(INTDIR)\Varsv.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\Win32.obj"
	-@erase "$(INTDIR)\Win32.sbr"
	-@erase "$(OUTDIR)\try.bsc"
	-@erase "$(OUTDIR)\try.exe"
	-@erase "$(OUTDIR)\try.ilk"
	-@erase "$(OUTDIR)\try.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /GX /Zi /Od /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\try.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Mxt.sbr" \
	"$(INTDIR)\Try.sbr" \
	"$(INTDIR)\Vars-standalone.sbr" \
	"$(INTDIR)\Varsa.sbr" \
	"$(INTDIR)\Varsp.sbr" \
	"$(INTDIR)\Varss.sbr" \
	"$(INTDIR)\Varsv.sbr" \
	"$(INTDIR)\Win32.sbr"

"$(OUTDIR)\try.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcd.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\try.pdb" /debug /machine:I386 /out:"$(OUTDIR)\try.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Mxt.obj" \
	"$(INTDIR)\Try.obj" \
	"$(INTDIR)\Vars-standalone.obj" \
	"$(INTDIR)\Varsa.obj" \
	"$(INTDIR)\Varsp.obj" \
	"$(INTDIR)\Varss.obj" \
	"$(INTDIR)\Varsv.obj" \
	"$(INTDIR)\Win32.obj"

"$(OUTDIR)\try.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("try.dep")
!INCLUDE "try.dep"
!ELSE 
!MESSAGE Warning: cannot find "try.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "try - Win32 Release" || "$(CFG)" == "try - Win32 Debug"
SOURCE=..\Mxt.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Mxt.obj"	"$(INTDIR)\Mxt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Mxt.obj"	"$(INTDIR)\Mxt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Try.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Try.obj"	"$(INTDIR)\Try.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Try.obj"	"$(INTDIR)\Try.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE="..\Vars-standalone.c"

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Vars-standalone.obj"	"$(INTDIR)\Vars-standalone.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Vars-standalone.obj"	"$(INTDIR)\Vars-standalone.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Varsa.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Varsa.obj"	"$(INTDIR)\Varsa.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Varsa.obj"	"$(INTDIR)\Varsa.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Varsp.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Varsp.obj"	"$(INTDIR)\Varsp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Varsp.obj"	"$(INTDIR)\Varsp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Varss.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Varss.obj"	"$(INTDIR)\Varss.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Varss.obj"	"$(INTDIR)\Varss.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Varsv.c

!IF  "$(CFG)" == "try - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /Gi /GX /O2 /Ob2 /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /D "SYMBOLTABLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\Varsv.obj"	"$(INTDIR)\Varsv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "try - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gi /GX /Zi /Od /Ob2 /I "." /D "_DEBUG" /D "SYMBOLTABLE" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "VARS_STANDALONE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\try.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\Varsv.obj"	"$(INTDIR)\Varsv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Win32.c

!IF  "$(CFG)" == "try - Win32 Release"


"$(INTDIR)\Win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "try - Win32 Debug"


"$(INTDIR)\Win32.obj"	"$(INTDIR)\Win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

