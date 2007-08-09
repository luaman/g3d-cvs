
; ----
; G3D Windows Installer
; ----


; ----
; product and version defines
; ----

; relative path to root G3D directory (prefix for installation file locations)
!define INST_TO_ROOT "."

; ----
; installer attributes
; ----

; installer window title
Name "G3D 7.0"

; installer build path
OutFile "${INST_TO_ROOT}\build\g3d-7_0_win32.exe"

; default install directory
InstallDir "C:\libraries\G3D"

ShowInstDetails show
SetCompressor lzma

; ----
; modern ui settings
; ----

!include "MUI.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"

; turn off per-component descriptions on right-side
!define MUI_COMPONENTSPAGE_NODESC

; Add directory selection page
!insertmacro MUI_PAGE_DIRECTORY

; Add component selection page
!insertmacro MUI_PAGE_COMPONENTS

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS



; ----
; User helper macros
; ----


; ----
; Component Sections
; ----

;;
;; Main installation component (copy files and create links)
;;
Section "!G3D Library for Visual C++ 8.0"
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
Section /o "Add include/library paths to Visual C++ 8"
  ; Check for Visual Studio 2005 version
  StrCpy $R0 "$PROFILE\Local Settings\Application Data\Microsoft\VisualStudio\8.0\VCComponents.dat"
  IfFileExists $R0 foundfile 0
  ; Check for Visual Studio 2005 Express
  ClearErrors
  StrCpy $R0 "$PROFILE\Local Settings\Application Data\Microsoft\VCExpress\8.0\VCComponents.dat"
  IfFileExists $R0 0 nofile

foundfile:
  nsis_vscomp_plugin::InsertVCComponentDirectories $R0 "$INSTDIR\win-i386-vc8.0\include" "$INSTDIR\win-i386-vc8.0\lib"

nofile:
SectionEnd
