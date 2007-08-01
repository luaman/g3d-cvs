
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "G3D"
!define PRODUCT_VERSION "7.0"
!define PRODUCT_WEB_SITE "http://g3d-cpp.sourceforge.net/"
!define PRODUCT_INSTALLER "g3d-7_0_win32.exe"

; relative path to root G3D directory (prefix for installation file locations)
!define INST_TO_ROOT "."

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"

!define MUI_COMPONENTSPAGE_NODESC

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Select components page
!insertmacro MUI_PAGE_COMPONENTS
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

InstType "Complete"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${INST_TO_ROOT}\build\${PRODUCT_INSTALLER}"

; default install directory
InstallDir "C:\libraries\G3D"

ShowInstDetails show


;;
;; Copy G3D files to installation directory
;;
Section "!G3D Library for Visual C++ 8.0"
  SectionIn 1
  
  SetOutPath "$INSTDIR\win-i386-vc8.0"
  SetOverwrite try
  File /r "${INST_TO_ROOT}\build\win-i386-vc8.0\*"
  SetOutPath "$INSTDIR\html"
  File /r "${INST_TO_ROOT}\build\html\*"
  SetOutPath "$INSTDIR\demos"
  File /r "${INST_TO_ROOT}\build\demos\*"
  SetOutPath "$INSTDIR\data"
  File /r "${INST_TO_ROOT}\build\data\*"

  ; Create shortcut to document index, gfxmeter and viewer
  CreateShortCut "$DESKTOP\G3D Documentation.lnk" "$INSTDIR\html\index.html"
SectionEnd

;;
;; Add the include/library G3D directories to the MSVC8/Express paths if they don't already exist
;;
Section "Add include/library paths to Visual C++ 8"
  SectionIn 1
  
  ; Create temp file for one line at a time read/write
  FileOpen $1 "$TEMP\g3d_vccomponents.dat" w
  IfErrors Abort_inclib_install

  ; put 8.0 location into $R0
  StrCpy $R0 "$PROFILE\Local Settings\Application Data\Microsoft\VisualStudio\8.0\VCComponents.dat"
  FileOpen $0 $R0 r
  IfErrors No_80_dat Read_vccomponents_loop
No_80_dat:
  ; put 8.0 Express location into $R0
  StrCpy $R0 "$PROFILE\Local Settings\Application Data\Microsoft\VCExpress\8.0\VCComponents.dat"
  FileOpen $0 $R0 r
  IfErrors Abort_inclib_install
Read_vccomponents_loop:    
  ; Start reading in one line
  FileRead $0 $R1
  IfErrors Finished_vccomponents_read
  Push $R1
  Push "Include Dirs="
  ; Find "Include Dirs=" in line
  Call StrStr
  Pop $R2
  StrLen $R3 $R2
  IntCmpU 0 $R3 Check_library_dirs Add_dir_to_include

Add_dir_to_include:
  Push $R1
  Push "$INSTDIR\win-i386-vc8.0\include"
  ; Make sure this exact include directory is unique
  Call StrStr
  Pop $R2
  StrLen $R3 $R2
  IntCmpU 0 $R3 Finish_adding_include Add_line_to_temp
Finish_adding_include:
  ; Write new include path into line
  FileWrite $1 "Include Dirs="
  StrCpy $R2 $R1 1024 13
  FileWrite $1 "$INSTDIR\win-i386-vc8.0\include;"
  FileWrite $1 $R2
  Goto Read_vccomponents_loop

Check_library_dirs:
  Push $R1
  Push "Library Dirs="
  Call StrStr
  Pop $R2
  StrLen $R3 $R2
  IntCmpU 0 $R3 Add_line_to_temp
  Push $R1
  Push "$INSTDIR\win-i386-vc8.0\lib"
  Call StrStr
  Pop $R2
  StrLen $R3 $R2
  IntCmpU 0 $R3 Finish_adding_lib Add_line_to_temp
Finish_adding_lib:
  FileWrite $1 "Library Dirs="
  StrCpy $R2 $R1 1024 13
  FileWrite $1 "$INSTDIR\win-i386-vc8.0\lib;"
  FileWrite $1 $R2
  Goto Read_vccomponents_loop
  
Add_line_to_temp:
  FileWrite $1 $R1
  Goto Read_vccomponents_loop


Finished_vccomponents_read:
  FileClose $0
  Fileclose $1
  CopyFiles /SILENT "$TEMP\g3d_vccomponents.dat" $R0
  Goto Clean_end

Abort_inclib_install:
  Fileclose $1
Clean_end:
SectionEnd

Section -Post
SectionEnd


;; External Functions

; StrStr
 ; input, top of stack = string to search for
 ;        top of stack-1 = string to search in
 ; output, top of stack (replaces with the portion of the string remaining)
 ; modifies no other variables.
 ;
 ; Usage:
 ;   Push "this is a very long string"
 ;   Push "long"
 ;   Call StrStr
 ;   Pop $R0
 ;  ($R0 at this point is "long string")

 Function StrStr
   Exch $R1 ; st=haystack,old$R1, $R1=needle
   Exch    ; st=old$R1,haystack
   Exch $R2 ; st=old$R1,old$R2, $R2=haystack
   Push $R3
   Push $R4
   Push $R5
   StrLen $R3 $R1
   StrCpy $R4 0
   ; $R1=needle
   ; $R2=haystack
   ; $R3=len(needle)
   ; $R4=cnt
   ; $R5=tmp
   loop:
     StrCpy $R5 $R2 $R3 $R4
     StrCmp $R5 $R1 done
     StrCmp $R5 "" done
     IntOp $R4 $R4 + 1
     Goto loop
 done:
   StrCpy $R1 $R2 "" $R4
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Exch $R1
 FunctionEnd
