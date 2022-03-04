; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

!define APPDIR "\Shotcut"

; The name of the installer
Name "Shotcut"

; The file to write
OutFile "shotcut-setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES64\Shotcut

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Shotcut" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

XPStyle on

SetCompressor lzma

;--------------------------------

; Pages

Page license
Page directory
Page components
Page instfiles

LicenseData Shotcut\COPYING.txt
ComponentText "" "" 'If Shotcut crashes at launch "Remove Shotcut Settings From Registry" might fix it.'

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

Section "Remove Old Program Files"

  ; Require that install dir ends with app name.
  StrLen $R0 "${APPDIR}"
  StrCpy $R1 $INSTDIR "" -$R0
  StrCmp $R1 "${APPDIR}" +2
    StrCpy $INSTDIR "$INSTDIR${APPDIR}"

  ; Remove program files and directories
  RMDir /r "$INSTDIR"

SectionEnd

; The stuff to install
Section "Install Program Files"

  SectionIn RO

  ; Require that install dir ends with app name.
  StrLen $R0 "${APPDIR}"
  StrCpy $R1 $INSTDIR "" -$R0
  StrCmp $R1 "${APPDIR}" +2
    StrCpy $INSTDIR "$INSTDIR${APPDIR}"

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File /r Shotcut\*
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Shotcut "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "DisplayName" "Shotcut"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "DisplayVersion" "YY.MM.DD"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "DisplayIcon" '"$INSTDIR\shotcut.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "Publisher" "Meltytech, LLC"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Create Start Menu Shortcut"

  ;CreateDirectory "$SMPROGRAMS\Shotcut"
  ;CreateShortCut "$SMPROGRAMS\Shotcut\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\Shotcut.lnk" "$INSTDIR\shotcut.exe" "" "$INSTDIR\shotcut.exe" 0

SectionEnd

; Optional section (can be disabled by the user)
Section "Associate *.mlt files with Shotcut" 

 ;register file extensions
 WriteRegStr HKCR ".mlt" ""  "Shotcut"
 WriteRegStr HKCR "Shotcut\shell\open\command" "" "$\"$INSTDIR\shotcut.exe$\" $\"%1$\""
  
SectionEnd

; Opt in section (can be enabled by the user)
Section /o "Create Desktop Shortcut (Icon)"

  SetShellVarContext all
  CreateShortCut "$DESKTOP\Shotcut.lnk" "$INSTDIR\shotcut.exe" "" "$INSTDIR\shotcut.exe" 0

SectionEnd

; Opt-in section (can be enabled by the user)
Section /o "Remove Shotcut Settings From Registry"

  DeleteRegKey HKCU "Software\Meltytech\Shotcut"

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Make sure the uninstaller is in APPDIR
  StrLen $R0 "${APPDIR}"
  StrCpy $R1 $INSTDIR "" -$R0
  StrCmp $R1 "${APPDIR}" +3
    MessageBox MB_OK|MB_ICONSTOP "The uninstall path is invalid!"
    Abort "Uninstall failed"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut"
  DeleteRegKey HKLM SOFTWARE\Shotcut
  DeleteRegStr HKCR ".mlt"
  DeleteRegStr HKCR "Shotcut\shell\open\command"

  ; Remove shortcuts, if any
  SetShellVarContext all
  Delete "$SMPROGRAMS\Shotcut.lnk"

  ; Remove program files and directories
  RMDir /r "$INSTDIR"

SectionEnd
