; -- shotcut.iss --
; Copyright (c) 2023 Meltytech, LLC

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

[Setup]
AppVersion=YY.MM.DD
AppPublisher=Meltytech
AppName=Shotcut
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
ArchitecturesAllowed=x64
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64
AppMutex="Meltytech Shotcut Running Mutex"
OutputBaseFilename=shotcut-setup
UninstallDisplayName=Shotcut
VersionInfoCopyright="Copyright (c) 2012-2023 Meltytech, LLC"

[Tasks]
Name: startMenu; Description: "Create Start Menu Shortcut"
Name: associateExtension; Description: "Associate *.mlt files with Shotcut"; Check: IsAdminInstallMode
Name: desktopIcon; Description: "Create Desktop Shortcut (Icon)"; Flags: unchecked
Name: removeSettings; Description: "Remove Shotcut Settings From Registry"; Flags: unchecked

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
Filename: "{app}\shotcut.exe"; Description: "Start Shotcut After Install"; Flags: postinstall nowait skipifsilent

[Code]
procedure RemoveShotcutSettings();
begin
  RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Meltytech');
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := IsAdminInstallMode and (PageID = wpUserInfo);
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
  CustomStatusLabel.Caption := 'Installing Shotcut...';
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
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut');
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Shotcut');
  end;
end;
