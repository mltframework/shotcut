; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "Shotcut"

; The file to write
OutFile "shotcut-setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Shotcut

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Shotcut" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

XPStyle on

SetCompressor lzma

;--------------------------------

; Pages

;Page components
Page license
Page directory
Page instfiles

LicenseData Shotcut\COPYING.txt

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Program Files"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File /r Shotcut\*
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Shotcut "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "DisplayName" "Shotcut"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  ;CreateDirectory "$SMPROGRAMS\Shotcut"
  ;CreateShortCut "$SMPROGRAMS\Shotcut\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Shotcut.lnk" "$INSTDIR\shotcut.exe" "" "$INSTDIR\shotcut.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut"
  DeleteRegKey HKLM SOFTWARE\Shotcut

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Shotcut.lnk"

  ; Remove program files and directories
  RMDir /r "$INSTDIR"

SectionEnd
