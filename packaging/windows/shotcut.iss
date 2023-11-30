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
WizardSmallImageFile="shotcut-logo-64.bmp"
WizardImageStretch=yes
ShowLanguageDialog=auto

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "fr"; MessagesFile: "French.isl"
Name: "pt"; MessagesFile: "BrazilianPortuguese.isl"
Name: "de"; MessagesFile: "German.isl"
Name: "ja"; MessagesFile: "Japanese.isl"
Name: "it"; MessagesFile: "Italian.isl"

[CustomMessages]
en.InstallOpera=Install Opera One Web Browser
en.StartMenu=Create Start Menu Shortcut
en.AssociateMltXml=Associate *.mlt files with Shotcut
en.DesktopIcon=Create Desktop Shortcut (Icon)
en.ClearShotcutSettings=Remove Shotcut Settings From Registry
en.InstallingShotcut=Installing Shotcut...
en.StartShotcut=Start Shotcut

pt.InstallOpera=Instale o Navegador Opera One
pt.StartMenu=Crie um atalho no menu Iniciar
pt.AssociateMltXml=Associe arquivos *.mlt ao Shotcut
pt.DesktopIcon=Crie um atalho na área de trabalho (ícone)
pt.ClearShotcutSettings=Remova as configurações do Shotcut do registro
pt.InstallingShotcut=Instalando o Shotcut...
pt.StartShotcut=Inicie o Shotcut

de.InstallOpera=Opera One Webbrowser installieren
de.StartMenu=Startmenü-Verknüpfung erstellen
de.AssociateMltXml=*.mlt-Dateien Shotcut zuordnen
de.DesktopIcon=Desktop-Verknüpfung (Icon) erstellen
de.ClearShotcutSettings=Shotcut-Einstellungen von der Datenbank entfernen
de.InstallingShotcut=Shotcut wird installiert...
de.StartShotcut=Starten Sie Shotcut

fr.InstallOpera=Installer le navigateur Internet Opera One
fr.StartMenu=Créer un raccourci pour le menu Démarrer
fr.AssociateMltXml=Associer les fichiers *.mlt à Shotcut
fr.DesktopIcon=Créer un raccourci sur le bureau (icône)
fr.ClearShotcutSettings=Enlever les réglages de Shotcut du registre
fr.InstallingShotcut=Installation de Shotcut...
fr.StartShotcut=Lancer Shotcut

it.InstallOpera=Installa Opera One Browser
it.StartMenu=Crea un'icona nel menu d'inizio
it.AssociateMltXml=Associa i file .mlt con Shotcut
it.DesktopIcon=Crea un'icona sul Desktop
it.ClearShotcutSettings=Elimina i paramentri Shotcut dai registri
it.InstallingShotcut=Installando Shotcut...
it.StartShotcut=Avvia Shotcut

ja.InstallOpera=Opera Oneウェブブラウザをインストールする
ja.StartMenu=スタートメニューのショートカットを作成する
ja.AssociateMltXml=.mltファイルをShotcutに関連付ける
ja.DesktopIcon=デスクトップショートカット（アイコン）の作成
ja.ClearShotcutSettings=レジストリからShotcutの設定を削除する
ja.InstallingShotcut=Shotcutのインストール...
ja.StartShotcut=Shotcutを起動する

[Tasks]
Name: startMenu; Description: "{cm:StartMenu}"
Name: associateExtension; Description: "{cm:AssociateMltXml}"; Check: IsAdminInstallMode
Name: desktopIcon; Description: "{cm:DesktopIcon}"; Flags: unchecked
Name: removeSettings; Description: "{cm:ClearShotcutSettings}"; Flags: unchecked

[Files]
Source: "opera-one-en-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-en-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-de-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-de-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-fr-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-fr-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-it-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-it-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-ja-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-ja-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-pt-1x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
Source: "opera-one-pt-2x.bmp"; Flags: dontcopy; Check: ShouldOperaBeOffered()
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
Filename: "{tmp}\opera-installer.exe"; StatusMsg: "Installing Opera"; Parameters: "--allusers=0 --silent --installfolder=""{autopf}\Opera"" --launchbrowser=0"; Check: not IsAdminInstallMode and DownloadOpera(); Flags: skipifsilent skipifdoesntexist
Filename: "{tmp}\opera-installer.exe"; StatusMsg: "Installing Opera"; Parameters: "--allusers=1 --silent --installfolder=""{autopf}\Opera"" --launchbrowser=0"; Check: IsAdminInstallMode and DownloadOpera(); Flags: skipifsilent skipifdoesntexist
Filename: "{app}\shotcut.exe"; Description: "{cm:StartShotcut}"; Flags: postinstall nowait skipifsilent

[Code]
var
  OperaPage: TWizardPage;
  OperaPageID: Integer;
  OperaImage: TBitmapImage;
  OperaCheckbox: TNewCheckBox;
  DownloadPage: TDownloadWizardPage;
  OperaDownloadSuccess: Boolean;
  OperaOfferSuccess: Boolean;

procedure RemoveShotcutSettings();
begin
  RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\Meltytech');
end;

function ThereIsAnOperaRegistryKey(const RootKey: Integer; const SubKeyName: String): BOolean;
var
  Names: TArrayOfString;
  i: Integer;
  s: String;
begin
  if RegGetSubkeyNames(RootKey, SubKeyName, Names) then
  begin
    Result := False;
    for i := 0 to GetArrayLength(Names)-1 do
    begin
      s := Names[i];
      if WildCardMatch(Names[i], 'Opera*') then
        Result := True;
    end;
  end
end;

function IsOperaInstalled(): Boolean;
begin
  Result := False;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Opera Software') then
    Result := True
  else if RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Wow6432Node\Opera Software') then
    Result := True
  else if ThereIsAnOperaRegistryKey(HKEY_LOCAL_MACHINE, 'Software\Microsoft\Windows\CurrentVersion\Uninstall') then
    Result := True
  else if ThereIsAnOperaRegistryKey(HKEY_LOCAL_MACHINE, 'Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall') then
    Result := True
  else if ThereIsAnOperaRegistryKey(HKEY_CURRENT_USER, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall') then
    Result := True
  else if ThereIsAnOperaRegistryKey(HKEY_CURRENT_USER, 'SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall') then
    Result := True;
end;

function ShouldOperaBeOffered(): Boolean;
begin
  Result := OperaOfferSuccess;
end;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  if Progress = ProgressMax then
    Log(Format('Successfully downloaded file to {tmp}: %s', [FileName]));
  Result := True;
end;

function DownloadOpera(): Boolean;
begin
  Result := False;
  if ShouldOperaBeOffered() and OperaCheckbox.Checked then
  begin;
    DownloadPage.Clear;
    DownloadPage.Add('https://net.geo.opera.com/opera/stable/windows?utm_source=Meltytech&utm_medium=apb', 'opera-installer.exe', '');
    DownloadPage.Show;
    try
      try
        DownloadPage.Download; // This downloads the files to {tmp}
        OperaDownloadSuccess := True;
        Result := True;
      except
        if DownloadPage.AbortedByUser then
          Log('Aborted by user.')
        else
          SuppressibleMsgBox(AddPeriod(GetExceptionMessage), mbCriticalError, MB_OK, IDOK);
      end;
    finally
      DownloadPage.Hide;
    end;
  end;
end;

procedure ImageOnClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  OperaCheckbox.Checked := True;
  //ShellExec('', 'https://opera.com', '', '', SW_SHOW, ewNoWait, ErrorCode);
end;

procedure InitializeWizard();
var
  CustomStatusLabel: TNewStaticText;
  DPI: Integer;
  OperaImageName: String;
begin
  Log(Format('ActiveLanguage %s ScaleX %d', [ActiveLanguage(), ScaleX(1)]));
  OperaOfferSuccess := not IsOperaInstalled;
  if OperaOfferSuccess then
  begin;
    try
      OperaOfferSuccess := DownloadTemporaryFileSize('https://check.shotcut.org/opera.json') > 0
    except
      OperaOfferSuccess := False;
    end;
  end;

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

  if ShouldOperaBeOffered() then
  begin;
    // The Opera Web browser blocks ads on the Shotcut web site when you visit for help and updates.
    OperaPage := CreateCustomPage(wpSelectDir, '', '');
    OperaPageID := OperaPage.ID;
    OperaImage := TBitmapImage.Create(WizardForm);
    DPI := ScaleX(1);
    if (DPI > 2) then
      DPI := 2;
    OperaImageName := Format('opera-one-%s-%dx.bmp', [ActiveLanguage(), DPI]);
    ExtractTemporaryFile(OperaImageName);
    with OperaImage do
    begin
      Bitmap.LoadFromFile(Format(ExpandConstant('{tmp}\%s'), [OperaImageName]));
      Parent := WizardForm.InnerPage;
      Top := 0;
      Left := 0;
      Stretch := True;
      Height := WizardForm.Height - ScaleY(40);
      Width := Round(1.6 * Height);
      Visible := False;
      Cursor := crHand;
      OnClick := @ImageOnClick;
    end;
    OperaCheckbox := TNewCheckBox.Create(OperaPage);
    with OperaCheckbox do
    begin
      Parent := WizardForm.InnerPage;
      Top := WizardForm.Height - ScaleY(38);
      Left := ScaleX(200);
      Width := WizardForm.Width;
      Height := Height + ScaleY(10);
      Font.Size := Font.Size + 3;
      Font.Style := [fsBold];
      Caption := ExpandConstant('{cm:InstallOpera}');
    end;
    DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), SetupMessage(msgPreparingDesc), @OnDownloadProgress);
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpInstalling then
  begin;
    Log('Removing old installer registry keys');
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Shotcut');
    RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'SOFTWARE\Shotcut');
  end;
  if ShouldOperaBeOffered() then
  begin;
    OperaImage.Visible := CurPageID = OperaPageID;
    OperaCheckbox.Visible := CurPageID = OperaPageID;
    WizardForm.Bevel1.Visible := CurPageID <> OperaPageID;
    WizardForm.MainPanel.Visible := CurPageID <> OperaPageID;
    WizardForm.InnerNotebook.Visible := CurPageID <> OperaPageID;
  end;
end;
