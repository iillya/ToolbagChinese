{ Font changes are an explicit Toolbag adapter, never part of the Qt display core. }
function ReplaceFont(Directory: String; Index: Cardinal; Restore: Boolean; Message: String; Capacity: Cardinal): Boolean;
external 'ReplaceFont@files:support.dll stdcall setuponly';
function RestoreFont(Directory: String; Index: Cardinal; Restore: Boolean; Message: String; Capacity: Cardinal): Boolean;
external 'ReplaceFont@{app}\ChineseLauncher\.inno\support.dll stdcall uninstallonly delayload';
function FontName(Index: Integer): String;
begin
  case Index of
    0: Result := 'notosans_chinese.slug';
    1: Result := 'segoeui.slug';
    2: Result := 'selawik.slug';
  end;
end;

procedure PrepareFonts;
var
  I: Integer;
  Name, Target, Backup, Legacy, OriginalHash, InstalledHash, CurrentHash, NewHash: String;
begin
  NewHash := GetSHA256OfFile(InstallRoot + '\ToolbagChineseFont.slug');
  { All original backups and metadata are committed before the first host write. }
  for I := 0 to 2 do begin
    Name := FontName(I);
    Target := ExpandConstant('{app}\data\gui\font\') + Name;
    if FileExists(Target) then begin
      CurrentHash := GetSHA256OfFile(Target);
      Backup := InstallRoot + '\.inno\font-backups\' + Name;
      Legacy := Target + '.ChineseLocalizer.backup';
      OriginalHash := GetIniString('FontOriginal', Name, '', StateFile);
      InstalledHash := GetIniString('FontInstalled', Name, '', StateFile);
      if OriginalHash <> '' then begin
        if not FileExists(Backup) or (GetSHA256OfFile(Backup) <> OriginalHash) then
          RaiseException('原字体备份缺失或校验失败，未替换字体：' + Name);
        if (CurrentHash <> InstalledHash) and (CurrentHash <> OriginalHash) then
          Log('Preserving externally modified font: ' + Target);
      end else begin
        if not ForceDirectories(ExtractFileDir(Backup)) then
          RaiseException('无法创建字体备份目录。');
        if FileExists(Backup) then
          RaiseException('存在未完成的字体备份。请保留备份并检查后重试：' + Backup);
        if FileExists(Legacy) then begin
          if CurrentHash <> NewHash then
            RaiseException('旧版字体状态不明确，未覆盖字体：' + Name);
          if GetSHA256OfFile(Legacy) = NewHash then
            RaiseException('旧版备份不是原始字体，未覆盖字体：' + Name);
          if not CopyFile(Legacy, Backup, True) or (GetSHA256OfFile(Backup) <> GetSHA256OfFile(Legacy)) then
            RaiseException('无法验证旧版字体备份：' + Name);
        end else begin
          if CurrentHash = NewHash then RaiseException('中文字体已存在，但缺少原字体备份：' + Name);
          if not CopyFile(Target, Backup, True) or (GetSHA256OfFile(Backup) <> CurrentHash) then
            RaiseException('无法备份原字体：' + Name);
        end;
        if not SetIniString('FontOriginal', Name, GetSHA256OfFile(Backup), StateFile) or
          not SetIniString('FontInstalled', Name, NewHash, StateFile) then
          RaiseException('无法保存字体恢复记录，未替换字体。');
      end;
    end;
  end;
end;

procedure ApplyProductOptions;
var
  I: Integer;
  Name, Target, CurrentHash, OriginalHash, InstalledHash, NewHash, Message: String;
begin
  PrepareFonts;
  NewHash := GetSHA256OfFile(InstallRoot + '\ToolbagChineseFont.slug');
  for I := 0 to 2 do begin
    Name := FontName(I);
    Target := ExpandConstant('{app}\data\gui\font\') + Name;
    OriginalHash := GetIniString('FontOriginal', Name, '', StateFile);
    InstalledHash := GetIniString('FontInstalled', Name, '', StateFile);
    if FileExists(Target) and (OriginalHash <> '') then begin
      CurrentHash := GetSHA256OfFile(Target);
      if (CurrentHash = OriginalHash) or (CurrentHash = InstalledHash) then begin
        { Record the intended hash before replacement, so an interrupted install can restore. }
        if not SetIniString('FontInstalled', Name, NewHash, StateFile) then
          RaiseException('无法保存字体恢复记录。');
        SetLength(Message, 1024);
        if not ReplaceFont(ExpandConstant('{app}'), I, False, Message, 1024) or
          (GetSHA256OfFile(Target) <> NewHash) then
          RaiseException('字体替换失败。请保留备份，修复或卸载中文补丁：' + Name);
      end;
    end;
  end;
end;

procedure RestoreProductOptions;
var
  I: Integer;
  Name, Target, Backup, OriginalHash, InstalledHash, Message: String;
begin
  { Validate every recovery file before changing any font. }
  for I := 0 to 2 do begin
    Name := FontName(I);
    OriginalHash := GetIniString('FontOriginal', Name, '', StateFile);
    Backup := InstallRoot + '\.inno\font-backups\' + Name;
    Target := ExpandConstant('{app}\data\gui\font\') + Name;
    InstalledHash := GetIniString('FontInstalled', Name, '', StateFile);
    { A completed restore can be retried even after its backup was cleaned. }
    if (OriginalHash = '') <> (InstalledHash = '') then
      RaiseException('字体恢复记录不完整，已停止卸载：' + Name);
    if (OriginalHash <> '') and FileExists(Target) and
      (GetSHA256OfFile(Target) = InstalledHash) and
      (not FileExists(Backup) or (GetSHA256OfFile(Backup) <> OriginalHash)) then
      RaiseException('原字体备份缺失或校验失败，已停止卸载：' + Name);
  end;
  for I := 0 to 2 do begin
    Name := FontName(I);
    Target := ExpandConstant('{app}\data\gui\font\') + Name;
    Backup := InstallRoot + '\.inno\font-backups\' + Name;
    InstalledHash := GetIniString('FontInstalled', Name, '', StateFile);
    if (InstalledHash <> '') and FileExists(Target) and (GetSHA256OfFile(Target) = InstalledHash) then begin
      SetLength(Message, 1024);
      if not RestoreFont(ExpandConstant('{app}'), I, True, Message, 1024) or (GetSHA256OfFile(Target) <> GetSHA256OfFile(Backup)) then
        RaiseException('原字体恢复失败，已停止卸载：' + Name);
    end else if InstalledHash <> '' then Log('Preserving externally modified/deleted font: ' + Target);
  end;
  { Cleanup is deferred until association restoration also succeeds. }
end;

procedure CleanupProductBackups;
var
  I: Integer;
  Name, Backup, Legacy, OriginalHash: String;
begin
  for I := 0 to 2 do begin
    Name := FontName(I);
    OriginalHash := GetIniString('FontOriginal', Name, '', StateFile);
    Backup := InstallRoot + '\.inno\font-backups\' + Name;
    Legacy := ExpandConstant('{app}\data\gui\font\') + Name + '.ChineseLocalizer.backup';
    if OriginalHash <> '' then begin
      if FileExists(Backup) and (GetSHA256OfFile(Backup) = OriginalHash) and not DeleteFile(Backup) then
        RaiseException('无法清理已完成恢复的字体备份：' + Backup);
      if FileExists(Legacy) and (GetSHA256OfFile(Legacy) = OriginalHash) and not DeleteFile(Legacy) then
        RaiseException('无法清理旧版字体备份：' + Legacy);
    end;
  end;
  RemoveDir(InstallRoot + '\.inno\font-backups');
end;
