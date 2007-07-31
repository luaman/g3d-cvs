
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

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${INST_TO_ROOT}\build\${PRODUCT_INSTALLER}"
; default install directory
InstallDir "$PROGRAMFILES\G3D"
ShowInstDetails show

Section "G3D Library" SEC01
  SetOutPath "$INSTDIR\win-i386-vc8.0"
  SetOverwrite try
  File /r "${INST_TO_ROOT}\build\win-i386-vc8.0\*"
  SetOutPath "$INSTDIR\html"
  File /r "${INST_TO_ROOT}\build\html\*"
  SetOutPath "$INSTDIR\demos"
  File /r "${INST_TO_ROOT}\build\demos\*"
  SetOutPath "$INSTDIR\data"
  File /r "${INST_TO_ROOT}\build\data\*"
SectionEnd

Section -Post
SectionEnd