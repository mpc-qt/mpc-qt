
;Derived from https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

  !pragma warning error all

;Smaller installer (83 MB vs 110 MB) but much slower installation
;SetCompressor /SOLID lzma

;--------------------------------
;Include Modern UI
  !include "MUI.nsh"

  Var INSTALLED_SIZE

;--------------------------------
;General

  ;Properly display all languages (Installer will not work on Windows 95, 98 or ME!)
  Unicode true

  ;Make sure text isn't blurry
  ManifestDPIAware true

  ;Name and file
  Name "MPC-QT"
  OutFile "mpc-qt-win-x64-installer.exe"
  BrandingText " "

  ;Default installation folder
  InstallDir "$PROGRAMFILES\MPC-QT"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\MPC-QT" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

  ;Show all languages, despite user's codepage
  !define MUI_LANGDLL_ALLLANGUAGES

  !define MUI_ICON "..\..\..\mpc-qt.ico"
  !define MUI_UNICON "..\..\..\mpc-qt.ico"

  !define MUI_WELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\nsis3-metro.bmp
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP ${NSISDIR}\Contrib\Graphics\Wizard\nsis3-metro.bmp

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKLM"
  !define MUI_LANGDLL_REGISTRY_KEY "Software\MPC-QT"
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "..\..\..\LICENSE"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  ;Var STARTMENU_FOLDER
  ;!insertmacro MUI_PAGE_STARTMENU "Application" $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  ;!insertmacro MUI_UNPAGE_COMPONENTS
  ;!insertmacro MUI_UNPAGE_DIRECTORY
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" ; The first language is the default language
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SerbianLatin"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Uzbek"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Afrikaans"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Esperanto"
  !insertmacro MUI_LANGUAGE "Asturian"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Pashto"
  !insertmacro MUI_LANGUAGE "ScotsGaelic"
  !insertmacro MUI_LANGUAGE "Georgian"
  !insertmacro MUI_LANGUAGE "Vietnamese"
  !insertmacro MUI_LANGUAGE "Welsh"
  !insertmacro MUI_LANGUAGE "Armenian"
  !insertmacro MUI_LANGUAGE "Corsican"
  !insertmacro MUI_LANGUAGE "Tatar"
  !insertmacro MUI_LANGUAGE "Hindi"

;--------------------------------
;Reserve Files

  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.

  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Sections

Section "Install"

  SetShellVarContext all
  SetOutPath "$INSTDIR"

  Delete "$INSTDIR\*.*"
  Delete "$INSTDIR\doc\*.*"
  Delete "$INSTDIR\iconengines\*.*"
  Delete "$INSTDIR\imageformats\*.*"
  Delete "$INSTDIR\platforms\*.*"
  Delete "$INSTDIR\styles\*.*"

  RMDir "$INSTDIR\doc"
  RMDir "$INSTDIR\iconengines"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR\styles"

  File "..\..\..\LICENSE"
  File /r "..\..\..\mpc-qt-win-x64\*.*"

  ;Store installation folder
  WriteRegStr HKLM "Software\MPC-QT" "" $INSTDIR

  ;Create uninstaller

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  SectionGetSize 0 $INSTALLED_SIZE
  ;Add the size of the uninstaller
  IntOp $INSTALLED_SIZE $INSTALLED_SIZE + 367

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "DisplayName" "MPC-QT"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                     "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                     "NoRepair" "1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "Publisher" "The MPC-QT developers"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "HelpLink" "https://github.com/mpc-qt/mpc-qt"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "URLUpdateInfo" "https://github.com/mpc-qt/mpc-qt/releases"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "URLInfoAbout" "https://mpc-qt.github.io/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "DisplayIcon" "$INSTDIR\mpc-qt.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT" \
                   "EstimatedSize" "$INSTALLED_SIZE"

  ;Create Start Menu shortcut
  ;!insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  CreateDirectory "$SMPROGRAMS\MPC-QT"
  CreateShortCut "$SMPROGRAMS\MPC-QT\MPC-QT.lnk" "$INSTDIR\mpc-qt.exe"

  ;!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  SetShellVarContext all

  # Remove Start Menu launcher
  Delete "$SMPROGRAMS\MPC-QT\MPC-QT.lnk"
  # Try to remove the Start Menu folder - this will only happen if it is empty
  RMDir "$SMPROGRAMS\MPC-QT"

  Delete "$INSTDIR\*.*"
  Delete "$INSTDIR\doc\*.*"
  Delete "$INSTDIR\iconengines\*.*"
  Delete "$INSTDIR\imageformats\*.*"
  Delete "$INSTDIR\platforms\*.*"
  Delete "$INSTDIR\styles\*.*"

  RMDir "$INSTDIR\doc"
  RMDir "$INSTDIR\iconengines"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR\styles"
  RMDir "$INSTDIR"

  DeleteRegKey HKLM "Software\MPC-QT"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MPC-QT"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd

