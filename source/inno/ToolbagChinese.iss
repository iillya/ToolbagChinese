; Inno Setup is the sole supported installer. Runtime payload is allowlisted.
#ifndef PayloadInclude
  #error Build using source\build_inno.bat
#endif
#define ProductId "ToolbagChinese.Inno"
#define ProductName "Toolbag 中文补丁"
#define AssocId "ToolbagChinese.Inno.casc"
#ifdef TestMode
  #define ProductId "ToolbagChinese.Inno.IsolatedTests"
  #define ProductName "Toolbag Inno Isolated Tests"
  #define AssocId "ToolbagChinese.Inno.IsolatedTests.casc"
#endif

[Setup]
AppId={#ProductId}
AppName={#ProductName}
AppVersion={#PackageVersion}
AppPublisher=神说要凑数
AppPublisherURL=https://space.bilibili.com/281243426
AppSupportURL=https://github.com/iillya/ToolbagChinese
DefaultDirName={autopf}\Marmoset\Toolbag 5
AppendDefaultDirName=no
DisableDirPage=no
DirExistsWarning=no
DisableProgramGroupPage=yes
UsePreviousTasks=no
UninstallFilesDir={app}\ChineseLauncher\.inno
UninstallDisplayIcon={app}\ChineseLauncher\ToolbagChineseLauncher.exe
OutputDir={#PackageOutput}
OutputBaseFilename=ToolbagChineseInstaller
SetupIconFile=..\..\icon\toolbag.ico
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
WizardStyle=modern light windows11
WizardSizePercent=110
WizardImageFile=
WizardSmallImageFile=
Compression=lzma2
SolidCompression=yes
CloseApplications=no
RestartApplications=no
RestartIfNeededByRun=no
ChangesAssociations=yes
SetupLogging=yes
UninstallLogging=no
#ifdef TestMode
PrivilegesRequired=lowest
#else
PrivilegesRequired=admin
#endif

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[LangOptions]
DialogFontName=Microsoft YaHei UI
DialogFontSize=10

[Messages]
SelectDirLabel3=请选择包含 toolbag.exe 的软件目录。补丁只写入其下的 ChineseLauncher 文件夹。
FinishedLabel=中文补丁已安装。请通过“Toolbag 中文版”快捷方式启动软件。%n%n卸载请使用 Windows“已安装的应用”。卸载会完整删除 ChineseLauncher 文件夹及其中的全部文件，不保留词典、设置或备份。

[Tasks]
Name: "desktopicon"; Description: "创建公共桌面快捷方式"
Name: "startmenuicon"; Description: "创建开始菜单快捷方式（所有用户）"
Name: "fileassoc"; Description: "通过中文启动器打开工程（不更改默认应用选择）"

[Files]
Source: "{#SupportDll}"; DestDir: "{app}\ChineseLauncher\.inno"; DestName: "support.dll"; Flags: ignoreversion
#include PayloadInclude

[Icons]
#ifndef TestMode
Name: "{autodesktop}\Toolbag 中文版"; Filename: "{app}\ChineseLauncher\ToolbagChineseLauncher.exe"; WorkingDir: "{app}"; IconFilename: "{app}\ChineseLauncher\ToolbagChineseLauncher.exe"; IconIndex: 0; Tasks: desktopicon
Name: "{autoprograms}\Toolbag 中文版"; Filename: "{app}\ChineseLauncher\ToolbagChineseLauncher.exe"; WorkingDir: "{app}"; IconFilename: "{app}\ChineseLauncher\ToolbagChineseLauncher.exe"; IconIndex: 0; Tasks: startmenuicon
#endif

; Associations are handled by the native, ownership-checked proxy journal.
[UninstallDelete]
Type: filesandordirs; Name: "{app}\ChineseLauncher"

; Do not create a new default ProgID or write UserChoice.

[Code]
var
  LegacySid: String;
  ProxySid: String;
  PayloadNames: TArrayOfString;

function CheckTarget(Directory: String; CheckVersion: Boolean; Message: String; Capacity: Cardinal): Boolean;
external 'CheckTarget@files:support.dll stdcall setuponly';
function LegacyOwner(Launcher: String; Owner: String; Capacity: Cardinal): Boolean;
external 'LegacyOwner@files:support.dll stdcall setuponly';
function CheckTargetUninstall(Directory: String; CheckVersion: Boolean; Message: String; Capacity: Cardinal): Boolean;
external 'CheckTarget@{app}\ChineseLauncher\.inno\support.dll stdcall uninstallonly delayload';
function RestoreLegacy(Sid, Launcher: String): Boolean;
external 'RestoreLegacy@{app}\ChineseLauncher\.inno\support.dll stdcall uninstallonly delayload';
function PlanProxy(Directory, Owner: String; OwnerCapacity: Cardinal; Message: String; Capacity: Cardinal): Boolean;
external 'PlanProxy@files:support.dll stdcall setuponly';
function ApplyProxy(Directory, Owner, Message: String; Capacity: Cardinal): Boolean;
external 'ApplyProxy@files:support.dll stdcall setuponly';
function RemoveProxy(Directory, Owner, Message: String; Capacity: Cardinal): Boolean;
external 'RemoveProxy@{app}\ChineseLauncher\.inno\support.dll stdcall uninstallonly delayload';

procedure InitPayload;
begin
  { Generated from the same explicit payload allowlist as the native packer. }
  #include PayloadCode
end;

function InstallRoot: String;
begin
  Result := AddBackslash(ExpandConstant('{app}')) + 'ChineseLauncher';
end;

function StateFile: String;
begin
  Result := InstallRoot + '\.inno\install-state.ini';
end;

function LauncherCommand: String;
begin
  Result := '"' + InstallRoot + '\ToolbagChineseLauncher.exe" "%1"';
end;

function BufferText(Buffer: String): String;
var
  Terminator: Integer;
begin
  Terminator := Pos(#0, Buffer);
  if Terminator > 0 then Result := Copy(Buffer, 1, Terminator - 1)
  else Result := Buffer;
end;

function ValidateDirectory(Directory: String): String;
var
  Message: String;
begin
  SetLength(Message, 1024);
  if CheckTarget(Directory, True, Message, 1024) then Result := ''
  else Result := BufferText(Message);
end;

#include "product_options.iss"

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Error: String;
begin
  Result := True;
  if CurPageID <> wpSelectDir then Exit;
  Error := ValidateDirectory(WizardDirValue);
  Result := Error = '';
  if not Result then SuppressibleMsgBox(Error, mbError, MB_OK, IDOK);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  PreviousDir, ExistingOwner: String;
  #ifndef TestMode
  Owner, Message: String;
  #endif
  RegistryRoot: Integer;
begin
  Result := ValidateDirectory(ExpandConstant('{app}'));
  if Result <> '' then Exit;
  ExistingOwner := GetIniString('Install', 'Owner', '', StateFile);
  if (ExistingOwner <> '') and (ExistingOwner <> '{#ProductId}') and
       (Pos('ToolbagChinese.Inno', ExistingOwner) <> 1) then begin
    Result := 'ChineseLauncher 的安装记录属于另一安装器，已停止覆盖。';
    Exit;
  end;
  RegistryRoot := HKLM64;
  #ifdef TestMode
  RegistryRoot := HKCU;
  #endif
  if RegQueryStringValue(RegistryRoot,
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#ProductId}_is1',
    'InstallLocation', PreviousDir) and
    (CompareText(RemoveBackslashUnlessRoot(PreviousDir), RemoveBackslashUnlessRoot(ExpandConstant('{app}'))) <> 0) then begin
    Result := '当前安装器一次只管理一个 Toolbag 目录。请先卸载旧目录中的中文补丁，再更换安装位置。';
    Exit;
  end;
  LegacySid := GetIniString('Install', 'LegacyOwnerSid', '', StateFile);
  ProxySid := GetIniString('Install', 'ProxyOwnerSid', '', StateFile);
  #ifndef TestMode
  if WizardIsTaskSelected('fileassoc') then begin
    SetLength(Owner, 185);
    SetLength(Message, 1024);
    if not PlanProxy(ExpandConstant('{app}'), Owner, 185, Message, 1024) then begin
      Result := BufferText(Message);
      Exit;
    end;
    if (ProxySid <> '') and (ProxySid <> BufferText(Owner)) then begin
      Result := '工程关联属于另一安装用户。请由原用户升级，或取消工程关联选项。';
      Exit;
    end;
    ProxySid := BufferText(Owner);
  end;
  if LegacySid = '' then begin
    SetLength(Owner, 185);
    if not LegacyOwner(InstallRoot + '\ToolbagChineseLauncher.exe', Owner, 185) then begin
      Result := '无法读取旧版工程关联的归属，已停止迁移。';
      Exit;
    end;
    LegacySid := BufferText(Owner);
  end;
  #endif
end;

procedure BackupExistingPayload;
var
  I, Suffix: Integer;
  Source, Target, Base, Backup: String;
begin
  Backup := '';
  for I := 0 to GetArrayLength(PayloadNames) - 1 do begin
    Source := InstallRoot + '\' + PayloadNames[I];
    if FileExists(Source) then begin
      if Backup = '' then begin
        Base := InstallRoot + '\.inno\backups\' + GetDateTimeString('yyyymmdd-hhnnss', '-', ':');
        Backup := Base;
        Suffix := 0;
        while DirExists(Backup) do begin
          Suffix := Suffix + 1;
          Backup := Base + '-' + IntToStr(Suffix);
        end;
      end;
      Target := Backup + '\' + PayloadNames[I];
      if not ForceDirectories(ExtractFileDir(Target)) or not CopyFile(Source, Target, True) or
        (GetSHA256OfFile(Source) <> GetSHA256OfFile(Target)) then
        RaiseException('无法完成升级前备份，未开始替换汉化文件：' + Source);
    end;
  end;
  if (Backup <> '') and FileExists(StateFile) then
    if not CopyFile(StateFile, Backup + '\install-state.ini', True) then
      RaiseException('无法备份旧安装记录，未开始替换汉化文件。');
  if Backup <> '' then Log('Pre-install recovery snapshot: ' + Backup);
end;

#include "cleanup.iss"

procedure CurStepChanged(CurStep: TSetupStep);
var
  Message: String;
begin
  if CurStep = ssInstall then begin
    #ifndef TestMode
    if FileExists(ExpandConstant('{userdesktop}\Toolbag 中文版.lnk')) and
      not DeleteFile(ExpandConstant('{userdesktop}\Toolbag 中文版.lnk')) then
      RaiseException('无法清理旧版当前用户桌面快捷方式，请关闭占用程序后重试。');
    if FileExists(ExpandConstant('{userprograms}\Toolbag 中文版.lnk')) and
      not DeleteFile(ExpandConstant('{userprograms}\Toolbag 中文版.lnk')) then
      RaiseException('无法清理旧版当前用户开始菜单快捷方式，请关闭占用程序后重试。');
    #endif
    BackupExistingPayload;
  end;
  if CurStep = ssPostInstall then begin
    DeleteIniSection('Hashes', StateFile);
    if not SetIniString('Install', 'Owner', '{#ProductId}', StateFile) or
      not SetIniString('Install', 'LegacyOwnerSid', LegacySid, StateFile) or
      not SetIniString('Install', 'ProxyOwnerSid', ProxySid, StateFile) then
      RaiseException('文件已安装，但安装记录无法保存；请保留日志并重新运行安装器。');
      #ifndef TestMode
      { Remove alternate uninstall records from other installer IDs for the
        same product so they cannot later delete this shared ChineseLauncher. }
      if CompareText('{#ProductId}', 'ToolbagChinese.Inno') <> 0 then
        RegDeleteKeyIncludingSubkeys(HKLM64,
          'Software\Microsoft\Windows\CurrentVersion\Uninstall\ToolbagChinese.Inno_is1');
      RegDeleteKeyIncludingSubkeys(HKLM64,
        'Software\Microsoft\Windows\CurrentVersion\Uninstall\ToolbagChinese.Inno.Experimental_is1');
      #endif
    ApplyProductOptions;
    #ifndef TestMode
    if WizardIsTaskSelected('fileassoc') then begin
      SetLength(Message, 1024);
      if not ApplyProxy(ExpandConstant('{app}'), ProxySid, Message, 1024) then
        RaiseException(BufferText(Message) + #13#10 + '汉化文件已安装；请保留安装目录和日志，请勿删除安装目录。');
      Log('Verified official command proxy installed for user: ' + ProxySid);
    end;
    #endif
    CleanupInstallerBackups;
  end;
end;

procedure InitializeWizard;
begin
  WizardForm.WelcomeLabel1.Alignment := taLeftJustify;
  WizardForm.WelcomeLabel2.Alignment := taLeftJustify;
  WizardForm.FinishedHeadingLabel.Alignment := taLeftJustify;
  WizardForm.FinishedLabel.Alignment := taLeftJustify;
  WizardForm.WelcomeLabel1.Left := ScaleX(24);
  WizardForm.WelcomeLabel1.Width := WizardForm.WelcomePage.ClientWidth - ScaleX(48);
  WizardForm.WelcomeLabel2.Left := ScaleX(24);
  WizardForm.WelcomeLabel2.Width := WizardForm.WelcomePage.ClientWidth - ScaleX(48);
  WizardForm.FinishedHeadingLabel.Left := ScaleX(24);
  WizardForm.FinishedHeadingLabel.Width := WizardForm.FinishedPage.ClientWidth - ScaleX(48);
  WizardForm.FinishedLabel.Left := ScaleX(24);
  WizardForm.FinishedLabel.Width := WizardForm.FinishedPage.ClientWidth - ScaleX(48);
  InitPayload;
end;

function CheckUninstallTarget: Boolean;
var
  Message: String;
begin
  SetLength(Message, 1024);
  Result := CheckTargetUninstall(ExpandConstant('{app}'), False, Message, 1024);
  if not Result then SuppressibleMsgBox(BufferText(Message), mbError, MB_OK, IDOK);
end;

function InitializeUninstall: Boolean;
begin
  InitPayload;
  try
    Result := CheckUninstallTarget;
  finally
    UnloadDLL(InstallRoot + '\.inno\support.dll');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Sid, Message: String;
begin
  if CurUninstallStep = usUninstall then begin
    try
      if not CheckUninstallTarget then RaiseException('请先正常关闭 Toolbag 后重试。');
      RestoreProductOptions;
      SetLength(Message, 1024);
      Sid := GetIniString('Install', 'ProxyOwnerSid', '', StateFile);
      if not RemoveProxy(ExpandConstant('{app}'), Sid, Message, 1024) then
        RaiseException(BufferText(Message));
      Sid := GetIniString('Install', 'LegacyOwnerSid', '', StateFile);
      if not RestoreLegacy(Sid, InstallRoot + '\ToolbagChineseLauncher.exe') then
        RaiseException('无法恢复旧版工程关联。请登录原安装用户后重试；汉化文件尚未删除。');
    finally
      UnloadDLL(InstallRoot + '\.inno\support.dll');
    end;

    CleanupInstallerBackups;
    CleanupProductBackups;

  end;
end;
