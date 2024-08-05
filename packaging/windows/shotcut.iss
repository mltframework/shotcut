; -- shotcut.iss --
; Copyright (c) 2024 Meltytech, LLC

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

#define CurrentYear        GetDateTimeString('yyyy','','')

[Setup]
AppName=Shotcut
AppVersion=YY.MM.DD
AppPublisher=Meltytech
WizardStyle=modern
DefaultDirName={autopf}\Shotcut
; Since no icons will be created in "{group}", we don't need the wizard
; to ask for a Start Menu folder name:
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\shotcut.exe
LicenseFile=Shotcut\COPYING.txt
Compression=lzma2
SolidCompression=no
OutputDir=userdocs:Inno Setup Examples Output
ChangesAssociations=yes
PrivilegesRequiredOverridesAllowed=commandline dialog
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64compatible
AppMutex="Meltytech Shotcut Running Mutex"
OutputBaseFilename=shotcut-setup
UninstallDisplayName=Shotcut
VersionInfoDescription=Shotcut installer
VersionInfoCopyright="(c) 2012-{CurrentYear} Meltytech, LLC"
WizardSmallImageFile="shotcut-logo-64.bmp"
WizardImageStretch=yes
ShowLanguageDialog=yes
UsePreviousLanguage=no
LanguageDetectionMethod=uilanguage

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"

[CustomMessages]
en.StartMenu=Create Start Menu Shortcut
en.AssociateMltXml=Associate *.mlt files with Shotcut
en.DesktopIcon=Create Desktop Shortcut (Icon)
en.ClearShotcutSettings=Remove Shotcut Settings From Registry
en.InstallingShotcut=Installing Shotcut...
en.StartShotcut=Start Shotcut

it.StartMenu=Crea collegamenti nel menu Start
it.AssociateMltXml=Associa file *.mlt con Shotcut
it.DesktopIcon=Crea collegamento desktop (icona)
it.ClearShotcutSettings=Rimuovi impostazioni Shotcut dal registro
it.InstallingShotcut=Installazione Shotcut...
it.StartShotcut=Esegui Shotcut

[Tasks]
Name: startMenu; Description: "{cm:StartMenu}"
Name: associateExtension; Description: "{cm:AssociateMltXml}"; Check: IsAdminInstallMode
Name: desktopIcon; Description: "{cm:DesktopIcon}"; Flags: unchecked
Name: removeSettings; Description: "{cm:ClearShotcutSettings}"; Flags: unchecked

[Files]
Source: "Shotcut\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs uninsremovereadonly

[Icons]
Name: "{autoprograms}\Shotcut"; Filename: "{app}\shotcut.exe"; Tasks: startMenu
Name: "{autodesktop}\Shotcut"; Filename: "{app}\shotcut.exe"; Tasks: desktopIcon

[Registry]
; Associate .mlt files (requires ChangesAssociations=yes)
Root: HKCR; Subkey: ".mlt"; ValueType: string; ValueName: ""; ValueData: "Shotcut.mlt"; Flags: uninsdeletekey; Check: IsAdminInstallMode; Tasks: associateExtension
Root: HKCR; Subkey: "Shotcut.mlt"; ValueType: string; ValueName: ""; ValueData: ""; Flags: uninsdeletekey; Check: IsAdminInstallMode; Tasks: associateExtension
Root: HKCR; Subkey: "Shotcut.mlt\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\shotcut.exe"" ""%1"""; Check: IsAdminInstallMode; Tasks: associateExtension
Root: HKCU; Subkey: "Software\Meltytech\Shotcut"; Tasks: removeSettings; AfterInstall: RemoveShotcutSettings()

[Run]
Filename: "{app}\shotcut.exe"; Description: "{cm:StartShotcut}"; Flags: postinstall nowait skipifsilent

[Code]
var
  DownloadPage: TDownloadWizardPage;

procedure RemoveShotcutSettings();
begin
  RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Meltytech\Shotcut');
end;

procedure InitializeWizard();
var
  CustomStatusLabel: TNewStaticText;
begin
  // Hide radio buttons on License page and pre-select "accept" to enable "next" button
  WizardForm.LicenseAcceptedRadio.Checked := True;
  WizardForm.LicenseAcceptedRadio.Visible := False;
  WizardForm.LicenseNotAcceptedRadio.Visible := False;
  WizardForm.LicenseLabel1.Visible := False;
  WizardForm.LicenseMemo.Top := WizardForm.LicenseLabel1.Top;
  WizardForm.LicenseMemo.Height :=
    WizardForm.LicenseNotAcceptedRadio.Top +
    WizardForm.LicenseNotAcceptedRadio.Height -
    WizardForm.LicenseMemo.Top - ScaleY(5);

  WizardForm.FilenameLabel.Visible := False;
  WizardForm.StatusLabel.Visible := False;

  WizardForm.ProgressGauge.Top := WizardForm.InstallingPage.Height - ScaleY(60);

  CustomStatusLabel := TNewStaticText.Create(WizardForm);
  CustomStatusLabel.Parent := WizardForm.InstallingPage;
  CustomStatusLabel.Caption := ExpandConstant('{cm:InstallingShotcut}');
  CustomStatusLabel.Font.Size := CustomStatusLabel.Font.Size + 4;
  CustomStatusLabel.Font.Style := [fsBold];
  CustomStatusLabel.AutoSize := True;
  CustomStatusLabel.Top :=
    WizardForm.ProgressGauge.Top - CustomStatusLabel.Height - ScaleY(8);
  CustomStatusLabel.Left :=
    WizardForm.ProgressGauge.Left +
    ((WizardForm.ProgressGauge.Width - CustomStatusLabel.Width) div 2);
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpInstalling then
  begin;
    Log('Removing old installer registry keys');
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut');
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Shotcut');
  end;
end;
