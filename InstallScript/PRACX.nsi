# TODO
# custom image on launch -> AdvSplash
# banner image

!include "MUI2.nsh"
!include StrRep.nsh
!include ReplaceInFile.nsh

!define APP_NAME "PRACX"
#!define WEB_SITE "http://www.civgaming.net/forums/"
!define VERSION "01.10.00.00"
!define VERSION_SHRT "1.10"
!define REG_ROOT "HKLM"
!define REG_APP_PATH "Software\Microsoft\DirectPlay\Applications"
#!define REG_GOG_PATH "Software\GOG.com"
!define PATCH_FILES_PATH "..\bin"
!define RESOURCE_FILES_PATH "..\resources"

######################################################################

VIProductVersion  "${VERSION}"
VIAddVersionKey "ProductName"  "${APP_NAME}"
VIAddVersionKey "FileDescription"  "${APP_NAME}"
VIAddVersionKey "FileVersion"  "${VERSION}"

######################################################################

CRCCheck on
XPStyle on
SetCompressor /SOLID /FINAL LZMA 
RequestExecutionLevel admin
Name "${APP_NAME}"
Caption "${APP_NAME} v${VERSION_SHRT} Installer"
OutFile "..\bin\PRACX.v${VERSION_SHRT}.exe"

######################################################################

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

#!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
#!insertmacro MUI_LANGUAGE "French"
#!insertmacro MUI_LANGUAGE "German"

!insertmacro MUI_RESERVEFILE_LANGDLL

######################################################################

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY

	# try to obtain install path
	ReadRegStr $0 ${REG_ROOT} "${REG_APP_PATH}\Sid Meier's Alpha Centauri" "UnofficialPath"
	StrCmp $0 "" 0 default_path
	ReadRegStr $0 ${REG_ROOT} "${REG_APP_PATH}\Sid Meier's Planetary Pack" "Path"
	StrCmp $0 "" 0 default_path
	ReadRegStr $0 ${REG_ROOT} "${REG_APP_PATH}\Sid Meier's Alpha Centauri" "Path"
	StrCmp $0 "" 0 default_path
	ReadRegStr $0 ${REG_ROOT} "${REG_APP_PATH}\Sid Meier's Alien Crossfire" "Path"
	StrCmp $0 "" 0 default_path
	#ReadRegStr $0 ${REG_ROOT} "${REG_GOG_PATH}\GOGSIDMEIERSALPHACENTAURI" "Path"
	#StrCmp $0 "" 0 default_path
	#ReadRegStr $0 ${REG_ROOT} "${REG_GOG_PATH}\GOGSIDMEIERSALIENCROSSFIRE" "Path"
	#StrCmp $0 "" 0 default_path
	StrCpy $0 "$PROGRAMFILES\Sid Meier's Alpha Centauri" ; default path if can't read from registry
	default_path:
	StrCpy $INSTDIR $0
FunctionEnd

######################################################################

!define BCKPATH "$INSTDIR\_backup_v${VERSION_SHRT}"

Section -MainProgram
	SetShellVarContext current

	# SMAC base : back up original files
	IfFileExists "${BCKPATH}\*.*" +2
		CreateDirectory "${BCKPATH}"
	CopyFiles /SILENT "$INSTDIR\terran.exe" "${BCKPATH}\terran.exe"
	CopyFiles /SILENT "$INSTDIR\terranx.exe" "${BCKPATH}\terranx.exe"
	CopyFiles /SILENT "$INSTDIR\Icons.pcx" "${BCKPATH}\Icons.pcx"
	CopyFiles /SILENT "$INSTDIR\prax.dll" "${BCKPATH}\prax.dll"
	CopyFiles /SILENT "$INSTDIR\prac.dll" "${BCKPATH}\prac.dll"
	CopyFiles /SILENT "$INSTDIR\pracxpatch.exe" "${BCKPATH}\pracxpatch.exe"
	
	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\PRACX.v${VERSION_SHRT}.Uninstaller.exe"

	File "${PATCH_FILES_PATH}\prax.dll"
	File "${PATCH_FILES_PATH}\prac.dll"
	File "${PATCH_FILES_PATH}\pracxpatch.exe"
	File "${RESOURCE_FILES_PATH}\PRACX Change Log.txt"
	File "${RESOURCE_FILES_PATH}\Icons.pcx"
	
	ExecWait '"$INSTDIR\pracxpatch.exe"'

	IfFileExists "$INSTDIR\game.sdb" 0 +2
		ExecWait 'sdbinst -u "$INSTDIR\game.sdb"'

	IfFileExists "$INSTDIR\game_add.sdb" 0 +2
		ExecWait 'sdbinst -u "$INSTDIR\game_add.sdb"'

	# Yitzi's Alpha Centauri.Ini disables pracx, but if a user installs this, they probably want it on.
	IfFileExists "$INSTDIR\Alpha Centauri.Ini" 0 +2
		!insertmacro _ReplaceInFile "$INSTDIR\Alpha Centauri.Ini" "Disabled=1" "Disabled=<DEFAULT>"
		!insertmacro _ReplaceInFile "$INSTDIR\Alpha Centauri.Ini" "ScreenWidth=1024" "ScreenWidth=<DEFAULT>"

	# ExecShell open '"$INSTDIR\PRACX Change Log.txt"' SW_SHOWNORMAL

SectionEnd

######################################################################


######################################################################


Section "Uninstall"
 Delete "$INSTDIR\PRACX.v${VERSION_SHRT}_Uninstaller.exe"
 
 CopyFiles /SILENT "${BCKPATH}\*.*" "$INSTDIR"

 RMDir /r "${BCKPATH}"
 
SectionEnd

######################################################################
