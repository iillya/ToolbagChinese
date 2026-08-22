# install.ps1 — 八猴 Toolbag 5 汉化插件 · 傻瓜式一键安装


#


# 设计目标：面向任何人，尽量零环境依赖。


#   - 默认直接使用 dist\ 里的预编译产物，**不需要安装 Visual Studio**；


#   - 自动以管理员身份运行；


#   - 自动定位 Toolbag（常见路径 + 注册表，找不到会弹出文件选择框让你选）；


#   - 若 Toolbag 正在运行，先提醒/帮你关闭，避免文件被占用；


#   - 每次安装前自动备份旧文件，可随时还原；


#   - 安装后自检并给出中文使用说明。


#


# 用法（直接双击仓库根目录的“一键安装汉化.bat”即可）：


#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1


#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1 -ToolbagDir "D:\Marmoset\Toolbag 5"


#   powershell -ExecutionPolicy Bypass -File scripts\install.ps1 -Build   # 开发者：自动重新编译（需要 VS C++ 工具）


param(


    [string]$ToolbagDir = '',


    [switch]$Build,        # 开发者用：自动调用 build.bat 重新编译


    [switch]$Uninstall,    # 卸载模式：移除已安装的汉化并还原字体


    [switch]$Quiet,


    [string]$OriginalUserSid = '',


    [string]$OriginalUserProfile = '',


    [string]$OriginalUserAppData = ''


)


$ErrorActionPreference = 'Stop'


Add-Type -AssemblyName System.Windows.Forms


$script:ProjectRoot = Split-Path -Parent $PSScriptRoot


$script:Dist = Join-Path $script:ProjectRoot 'dist'


function Write-Step([string]$msg) {


    if (-not $Quiet) { Write-Host "`n==> $msg" -ForegroundColor Cyan }


}


function Write-Info([string]$msg) {


    if (-not $Quiet) { Write-Host "    $msg" -ForegroundColor Gray }


}


function Show-Error([string]$title, [string]$msg) {


    Write-Host "`n[错误] $msg" -ForegroundColor Red


    try { [System.Windows.Forms.MessageBox]::Show($msg, $title, 'OK', 'Error') | Out-Null } catch {}


}


function Ask-YesNo([string]$title, [string]$msg) {


    try {


        $r = [System.Windows.Forms.MessageBox]::Show($msg, $title, 'YesNo', 'Question')


        return ($r -eq 'Yes')


    } catch {


        Write-Host "`n$msg (Y/N)" -ForegroundColor Yellow


        return ((Read-Host).Trim().ToUpper() -eq 'Y')


    }


}


# ---------- 1. 管理员提权 ----------


$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(


    [Security.Principal.WindowsBuiltInRole]::Administrator)


if (-not $isAdmin) {


    if (-not $OriginalUserSid) { $OriginalUserSid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value }


    if (-not $OriginalUserProfile) { $OriginalUserProfile = $env:USERPROFILE }


    if (-not $OriginalUserAppData) { $OriginalUserAppData = $env:APPDATA }


    Write-Host '需要管理员权限，正在重新以管理员身份启动...' -ForegroundColor Yellow


    $argList = '-NoProfile -ExecutionPolicy Bypass -File "' + $MyInvocation.MyCommand.Path + '"'


    if ($ToolbagDir) { $argList += ' -ToolbagDir "' + $ToolbagDir + '"' }


    if ($Build) { $argList += ' -Build' }


    if ($Uninstall) { $argList += ' -Uninstall' }


    if ($Quiet) { $argList += ' -Quiet' }


    $argList += ' -OriginalUserSid "' + $OriginalUserSid + '"'


    $argList += ' -OriginalUserProfile "' + $OriginalUserProfile + '"'


    $argList += ' -OriginalUserAppData "' + $OriginalUserAppData + '"'


    try {


        $elevated = Start-Process powershell.exe -Verb RunAs -ArgumentList $argList -Wait -PassThru


        exit $elevated.ExitCode


    } catch {


        Show-Error '汉化安装' "无法获取管理员权限，安装已取消。`n请右键“一键安装汉化.bat”选择“以管理员身份运行”。"


        exit 1


    }


}


# ---------- 2. 定位 Toolbag 安装目录 ----------


function Find-ToolbagDirectory([string]$Override) {


    if ($Override) {


        $cand = Join-Path $Override 'toolbag.exe'


        if (Test-Path -LiteralPath $cand) { return (Resolve-Path -LiteralPath $Override).Path }


        return $null


    }


    $stateKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer'
    if (Test-Path -LiteralPath $stateKey) {
        $savedToolbag = (Get-ItemProperty -LiteralPath $stateKey -ErrorAction SilentlyContinue).ToolbagDir
        if ($savedToolbag -and (Test-Path -LiteralPath (Join-Path $savedToolbag 'toolbag.exe'))) {
            return (Resolve-Path -LiteralPath $savedToolbag).Path
        }
    }


    $originalLocalAppData = Join-Path (Split-Path -Parent $OriginalUserAppData) 'Local'


    $candidates = @(


        "$env:ProgramFiles\Marmoset\Toolbag 5",


        "${env:ProgramFiles(x86)}\Marmoset\Toolbag 5",


        "$originalLocalAppData\Marmoset Toolbag 5",


        'C:\Program Files\Marmoset\Toolbag 5'


    )


    foreach ($c in $candidates) {


        if ($c -and (Test-Path -LiteralPath (Join-Path $c 'toolbag.exe'))) {


            return (Resolve-Path -LiteralPath $c).Path


        }


    }


    # 注册表卸载项里找 InstallLocation / DisplayIcon


    $roots = @(


        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*',


        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*',


        ((Get-UserRegistryPath 'Software\Microsoft\Windows\CurrentVersion\Uninstall') + '\*')


    )


    foreach ($root in $roots) {


        foreach ($key in Get-ItemProperty $root -ErrorAction SilentlyContinue) {


            if ([string]$key.DisplayName -notmatch '(?i)(Marmoset\s*)?Toolbag\s*5') { continue }


            $loc = $key.InstallLocation


            if (-not $loc -and $key.DisplayIcon) {
                $displayIcon = ([string]$key.DisplayIcon -replace ',\s*-?\d+$', '').Trim('"')
                $loc = Split-Path $displayIcon -Parent
            }


            if ($loc -and (Test-Path -LiteralPath (Join-Path $loc 'toolbag.exe'))) {


                return (Resolve-Path -LiteralPath $loc).Path


            }


        }


    }


    return $null


}


# ---- 快捷方式（中文启动入口，图标取自 toolbag.exe）----

function New-Shortcuts {

    param([string]$pluginDir, [string]$toolbag)

    try {

        $ws = New-Object -ComObject WScript.Shell

        $launcher = Join-Path $pluginDir 'ToolbagChineseLauncher.exe'

        $icon = Join-Path $toolbag 'toolbag.exe'

        $name = '八猴5汉化版'

        $lnks = @()

        $desktop = Join-Path $OriginalUserProfile 'Desktop'

        if (Test-Path -LiteralPath $desktop) { $lnks += (Join-Path $desktop ($name + '.lnk')) }

        $startMenu = Join-Path $OriginalUserAppData 'Microsoft\Windows\Start Menu\Programs'

        if (Test-Path -LiteralPath $startMenu) { $lnks += (Join-Path $startMenu ($name + '.lnk')) }

        foreach ($lnk in $lnks) {

            $sc = $ws.CreateShortcut($lnk)

            $sc.TargetPath = $launcher

            $sc.WorkingDirectory = $pluginDir

            $sc.IconLocation = "$icon, 0"

            $sc.Description = '八猴5汉化版'

            $sc.Save()

            Write-Info "已创建快捷方式: $lnk"

        }

    } catch {

        Write-Info ('创建快捷方式失败: ' + $_.Exception.Message)

    }

}

function Remove-Shortcuts {

    $names = @('八猴5汉化版', '八猴Toolbag5汉化')

    foreach ($d in @((Join-Path $OriginalUserProfile 'Desktop'), (Join-Path $OriginalUserAppData 'Microsoft\Windows\Start Menu\Programs'))) {

        foreach ($name in $names) {
            $lnk = Join-Path $d ($name + '.lnk')
            if (Test-Path -LiteralPath $lnk) { Remove-Item -LiteralPath $lnk -Force; Write-Info "已删除快捷方式: $lnk" }
        }

    }

}

function Get-ToolbagRelatedProcesses([string]$toolbagRoot) {

    $paths = @(
        (Join-Path $toolbagRoot 'toolbag.exe'),
        (Join-Path $toolbagRoot 'data\ChineseLocalizer\ToolbagChineseLauncher.exe'),
        (Join-Path $toolbagRoot 'data\plugin\ChineseLocalizer\ToolbagChineseLauncher.exe')
    ) | ForEach-Object {
        try { [IO.Path]::GetFullPath($_).TrimEnd('\') } catch { $_ }
    }

    return @(Get-Process -Name 'toolbag','ToolbagChineseLauncher' -ErrorAction SilentlyContinue | Where-Object {
        try {
            $processPath = [IO.Path]::GetFullPath($_.Path).TrimEnd('\')
            $paths -contains $processPath
        } catch { $false }
    })
}

function Stop-ToolbagRelatedProcesses([string]$toolbagRoot) {

    $running = @(Get-ToolbagRelatedProcesses $toolbagRoot)
    if (-not $running) { return }

    foreach ($process in $running) {
        try { $null = $process.CloseMainWindow() } catch {}
    }
    Start-Sleep -Milliseconds 500

    foreach ($process in $running) {
        try {
            if (-not $process.HasExited) { $process.Kill() }
            if (-not $process.WaitForExit(5000)) {
                throw "进程未能退出: $($process.ProcessName) (PID $($process.Id))"
            }
        } catch {
            throw "无法关闭占用汉化文件的进程 $($process.ProcessName) (PID $($process.Id))：$($_.Exception.Message)"
        }
    }

    # 进程退出后，给系统外壳、杀毒软件和映像映射一点释放时间。
    Start-Sleep -Milliseconds 500
}

function Remove-DirectoryWithRetry([string]$path, [string]$label) {

    if (-not (Test-Path -LiteralPath $path)) { return }
    $lastError = $null
    for ($attempt = 1; $attempt -le 12; $attempt++) {
        try {
            Get-ChildItem -LiteralPath $path -Force -Recurse -ErrorAction SilentlyContinue |
                ForEach-Object { try { $_.Attributes = 'Normal' } catch {} }
            Remove-Item -LiteralPath $path -Recurse -Force -ErrorAction Stop
            if (-not (Test-Path -LiteralPath $path)) { return }
        } catch {
            $lastError = $_
        }
        Start-Sleep -Milliseconds 250
    }
    $reason = if ($lastError) { $lastError.Exception.Message } else { '目录仍然存在' }
    throw "$label 无法删除，文件仍被其他程序占用。请关闭 Toolbag、旧汉化启动器或安全软件后重试。`n路径：$path`n原因：$reason"
}

$script:installInProgress = $false
$script:rollbackPluginDir = ''
$script:rollbackPluginBackup = ''
$script:fontBackupCreatedByCurrentInstall = $false
$script:fontOriginalPath = ''
$script:fontBackupPath = ''
$script:hadExistingPlugin = $false
$script:rollbackToolbag = ''


if (-not $OriginalUserSid) { $OriginalUserSid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value }


if (-not $OriginalUserProfile) { $OriginalUserProfile = $env:USERPROFILE }


if (-not $OriginalUserAppData) { $OriginalUserAppData = $env:APPDATA }


$script:UserRegistryHive = [Microsoft.Win32.Registry]::Users.OpenSubKey($OriginalUserSid, $true)


if (-not $script:UserRegistryHive) { throw "无法打开原始用户注册表配置单元: $OriginalUserSid" }


function Get-UserRegistryPath([string]$relativePath) {


    return "Registry::HKEY_USERS\$OriginalUserSid\$relativePath"


}

# The self-contained GUI installer has no console. Surface every terminating
# PowerShell error with its real line number and keep a diagnostic log so an
# association failure never degrades into an unhelpful generic message.
trap {
    if ($script:installInProgress) {
        try {
            if ($script:hadExistingPlugin) {
                Install-TbsceneAssociation -pluginDir $script:rollbackPluginDir -toolbag $script:rollbackToolbag
            } else {
                Restore-TbsceneAssociation
            }
        } catch {}
        try {
            Remove-Shortcuts
            if ($script:hadExistingPlugin) {
                New-Shortcuts -pluginDir $script:rollbackPluginDir -toolbag $script:rollbackToolbag
            }
        } catch {}
        try {
            if ($script:rollbackPluginDir -and (Test-Path -LiteralPath $script:rollbackPluginDir)) {
                Remove-Item -LiteralPath $script:rollbackPluginDir -Recurse -Force
            }
            if ($script:rollbackPluginBackup -and (Test-Path -LiteralPath $script:rollbackPluginBackup)) {
                Copy-Item -LiteralPath $script:rollbackPluginBackup -Destination $script:rollbackPluginDir -Recurse -Force
                Remove-Item -LiteralPath $script:rollbackPluginBackup -Recurse -Force
            }
            if ($script:fontBackupCreatedByCurrentInstall -and
                $script:fontBackupPath -and
                (Test-Path -LiteralPath $script:fontBackupPath)) {
                Copy-Item -LiteralPath $script:fontBackupPath -Destination $script:fontOriginalPath -Force
                Remove-Item -LiteralPath $script:fontBackupPath -Force
            }
        } catch {}
    }
    $detail = $_.Exception.Message + "`n`n" + $_.InvocationInfo.PositionMessage
    $logPath = Join-Path $env:TEMP '八猴5汉化版_安装错误.log'
    try { $detail | Set-Content -LiteralPath $logPath -Encoding UTF8 } catch {}
    try {
        [System.Windows.Forms.MessageBox]::Show(
            "安装过程发生错误：`n`n$detail`n`n错误日志：$logPath",
            '八猴5汉化版', 'OK', 'Error') | Out-Null
    } catch {}
    exit 1
}

function Update-ShellAssociations {
    try {
        if (-not ('ChineseLocalizer.ShellNotify' -as [type])) {
            Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace ChineseLocalizer {
    public static class ShellNotify {
        [DllImport("shell32.dll")]
        public static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2);
    }
}
'@
        }
        [ChineseLocalizer.ShellNotify]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)
    } catch {
        Write-Info ('刷新文件关联失败: ' + $_.Exception.Message)
    }
}

function Install-HandlerProxy([int]$index, [string]$rootPath,
                              [string]$openCommand, [string]$backupKey) {
    $root = $script:UserRegistryHive
    $commandPath = $rootPath + '\shell\open\command'
    $savedName = "Handler${index}Saved"
    $backup = Get-ItemProperty -LiteralPath $backupKey
    if (-not ($backup.PSObject.Properties.Name -contains $savedName)) {
        $rootKey = $root.OpenSubKey($rootPath, $false)
        $hadRoot = $null -ne $rootKey
        if ($rootKey) { $rootKey.Close() }
        $commandKey = $root.OpenSubKey($commandPath, $false)
        $hadCommand = $null -ne $commandKey
        $oldValue = ''
        $hadDefault = $false
        if ($commandKey) {
            $hadDefault = $null -ne $commandKey.GetValue('', $null)
            if ($hadDefault) { $oldValue = [string]$commandKey.GetValue('', '') }
            $commandKey.Close()
        }
        New-ItemProperty -LiteralPath $backupKey -Name $savedName -Value 1 -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name "Handler${index}HadRoot" -Value ([int]$hadRoot) -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name "Handler${index}HadCommand" -Value ([int]$hadCommand) -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name "Handler${index}HadDefault" -Value ([int]$hadDefault) -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name "Handler${index}Value" -Value $oldValue -PropertyType String -Force | Out-Null
    }
    $commandKey = $root.CreateSubKey($commandPath)
    $commandKey.SetValue('', $openCommand, [Microsoft.Win32.RegistryValueKind]::String)
    $commandKey.Close()
}

function Restore-HandlerProxy([int]$index, [string]$rootPath, $backup) {
    $savedName = "Handler${index}Saved"
    if (-not ($backup.PSObject.Properties.Name -contains $savedName)) { return }
    $root = $script:UserRegistryHive
    if ([int]$backup.("Handler${index}HadRoot") -eq 0) {
        $root.DeleteSubKeyTree($rootPath, $false)
        return
    }
    $commandPath = $rootPath + '\shell\open\command'
    if ([int]$backup.("Handler${index}HadCommand") -eq 1) {
        $commandKey = $root.CreateSubKey($commandPath)
        if ([int]$backup.("Handler${index}HadDefault") -eq 1) {
            $commandKey.SetValue('', [string]$backup.("Handler${index}Value"), [Microsoft.Win32.RegistryValueKind]::String)
        } else {
            $commandKey.DeleteValue('', $false)
        }
        $commandKey.Close()
    } else {
        $root.DeleteSubKeyTree($commandPath, $false)
    }
}

function Remove-StaleHandlerProxy([string]$rootPath) {
    $commandPath = $rootPath + '\shell\open\command'
    $commandKey = $script:UserRegistryHive.OpenSubKey($commandPath, $false)
    if (-not $commandKey) { return }
    $command = [string]$commandKey.GetValue('', '')
    $commandKey.Close()
    if ($command -notmatch '(?i)ToolbagChineseLauncher\.exe') { return }

    # The backup may be absent after an interrupted older uninstall. Remove
    # only a handler that still explicitly invokes our launcher; HKCR will then
    # expose Toolbag's machine-wide registration again.
    $script:UserRegistryHive.DeleteSubKeyTree($rootPath, $false)
    Write-Info "已清理残留的汉化处理器: $rootPath"
}

function Remove-LocalizedOpenWithEntries {
    $fileExtPath = 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.tbscene'
    $openWithListPath = $fileExtPath + '\OpenWithList'
    $openWithList = $script:UserRegistryHive.OpenSubKey($openWithListPath, $true)
    if ($openWithList) {
        $removedNames = @()
        foreach ($name in $openWithList.GetValueNames()) {
            if ($name -eq 'MRUList') { continue }
            if ([string]$openWithList.GetValue($name, '') -ieq 'ToolbagChineseLauncher.exe') {
                $openWithList.DeleteValue($name, $false)
                $removedNames += $name
            }
        }
        if ($removedNames.Count -gt 0) {
            $mru = [string]$openWithList.GetValue('MRUList', '')
            foreach ($name in $removedNames) { $mru = $mru.Replace($name, '') }
            $openWithList.SetValue('MRUList', $mru, [Microsoft.Win32.RegistryValueKind]::String)
        }
        $openWithList.Close()
    }

    foreach ($choiceName in @('UserChoice', 'UserChoiceLatest')) {
        $choicePath = $fileExtPath + '\' + $choiceName
        $choiceKey = $script:UserRegistryHive.OpenSubKey($choicePath, $false)
        $selectedProgId = if ($choiceKey) { [string]$choiceKey.GetValue('ProgId', '') } else { '' }
        if ($choiceKey) { $choiceKey.Close() }
        $progIdKey = $script:UserRegistryHive.OpenSubKey($choicePath + '\ProgId', $false)
        if (-not $selectedProgId -and $progIdKey) {
            $selectedProgId = [string]$progIdKey.GetValue('ProgId', '')
        }
        if ($progIdKey) { $progIdKey.Close() }
        if ($selectedProgId -eq 'ChineseLocalizer.tbscene') {
            $script:UserRegistryHive.DeleteSubKeyTree($choicePath, $false)
            Write-Info "已清理残留的 $choiceName 汉化关联"
        }
    }
}

function Install-TbsceneAssociation([string]$pluginDir, [string]$toolbag) {
    $classesRoot = Get-UserRegistryPath 'Software\Classes'
    $extensionKey = Join-Path $classesRoot '.tbscene'
    $progId = 'ChineseLocalizer.tbscene'
    $progIdKey = Join-Path $classesRoot $progId
    $applicationKey = Join-Path $classesRoot 'Applications\ToolbagChineseLauncher.exe'
    $capabilitiesKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer\Capabilities'
    $backupKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer\FileAssociationBackup'

    # Save the original per-user default once. Reinstallation must never turn
    # our own association into the value that uninstall later restores.
    if (-not (Test-Path -LiteralPath $backupKey)) {
        New-Item -Path $backupKey -Force | Out-Null
        $hadKey = Test-Path -LiteralPath $extensionKey
        $hadDefault = $false
        $defaultValue = ''
        if ($hadKey) {
            $key = Get-Item -LiteralPath $extensionKey
            $hadDefault = $null -ne $key.GetValue('', $null)
            if ($hadDefault) { $defaultValue = [string]$key.GetValue('', '') }
        }
        New-ItemProperty -LiteralPath $backupKey -Name HadKey -Value ([int]$hadKey) -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name HadDefault -Value ([int]$hadDefault) -PropertyType DWord -Force | Out-Null
        New-ItemProperty -LiteralPath $backupKey -Name DefaultValue -Value $defaultValue -PropertyType String -Force | Out-Null
    }

    $sceneIcon = '"' + (Join-Path $pluginDir 'tbscene.ico') + '"'

    New-Item -Path $extensionKey -Force | Out-Null
    Set-Item -LiteralPath $extensionKey -Value $progId
    New-Item -Path $progIdKey -Force | Out-Null
    Set-Item -LiteralPath $progIdKey -Value 'Toolbag 5 场景（八猴5汉化版）'
    $iconKey = New-Item -Path (Join-Path $progIdKey 'DefaultIcon') -Force
    Set-Item -LiteralPath $iconKey.PSPath -Value $sceneIcon
    $commandKey = New-Item -Path (Join-Path $progIdKey 'shell\open\command') -Force
    $launcher = Join-Path $pluginDir 'ToolbagChineseLauncher.exe'
    $openCommand = '"' + $launcher + '" "%1"'
    Set-Item -LiteralPath $commandKey.PSPath -Value $openCommand

    # Proxy every handler that existing Windows/Toolbag installations are
    # known to cache. This leaves toolbag.exe untouched and only redirects
    # Explorer/Shell opens through the localized launcher.
    Install-HandlerProxy 0 'Software\Classes\MarmosetToolbag5.Scene' $openCommand $backupKey
    Install-HandlerProxy 1 'Software\Classes\MarmosetToolbag.Scene' $openCommand $backupKey
    Install-HandlerProxy 2 'Software\Classes\Applications\toolbag.exe' $openCommand $backupKey

    # Register the launcher as a complete Shell application. Merely changing
    # the extension's default ProgID is not sufficient on current Windows;
    # Explorer may continue using the executable cached in OpenWithList.
    New-Item -Path $applicationKey -Force | Out-Null
    New-ItemProperty -LiteralPath $applicationKey -Name FriendlyAppName -Value '八猴5汉化版' -PropertyType String -Force | Out-Null
    $applicationCommand = New-Item -Path (Join-Path $applicationKey 'shell\open\command') -Force
    Set-Item -LiteralPath $applicationCommand.PSPath -Value $openCommand
    $supportedTypes = New-Item -Path (Join-Path $applicationKey 'SupportedTypes') -Force
    New-ItemProperty -LiteralPath $supportedTypes.PSPath -Name '.tbscene' -Value '' -PropertyType String -Force | Out-Null

    New-Item -Path $capabilitiesKey -Force | Out-Null
    New-ItemProperty -LiteralPath $capabilitiesKey -Name ApplicationName -Value '八猴5汉化版' -PropertyType String -Force | Out-Null
    New-ItemProperty -LiteralPath $capabilitiesKey -Name ApplicationDescription -Value '通过八猴5汉化版打开 Toolbag 5 场景' -PropertyType String -Force | Out-Null
    $fileAssociations = New-Item -Path (Join-Path $capabilitiesKey 'FileAssociations') -Force
    New-ItemProperty -LiteralPath $fileAssociations.PSPath -Name '.tbscene' -Value $progId -PropertyType String -Force | Out-Null
    $registeredApps = New-Item -Path (Get-UserRegistryPath 'Software\RegisteredApplications') -Force
    New-ItemProperty -LiteralPath $registeredApps.PSPath -Name '八猴5汉化版' -Value 'Software\MarmosetChineseLocalizer\Capabilities' -PropertyType String -Force | Out-Null

    $openWithProgidsPath = 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.tbscene\OpenWithProgids'
    $openWithProgids = $script:UserRegistryHive.CreateSubKey($openWithProgidsPath)
    $openWithProgids.SetValue($progId, [byte[]]@(), [Microsoft.Win32.RegistryValueKind]::None)
    $openWithProgids.Close()
    Update-ShellAssociations
    Write-Info '.tbscene 已关联到八猴5汉化版'
}

function Restore-TbsceneAssociation {
    $classesRoot = Get-UserRegistryPath 'Software\Classes'
    $extensionKey = Join-Path $classesRoot '.tbscene'
    $progId = 'ChineseLocalizer.tbscene'
    $progIdKey = Join-Path $classesRoot $progId
    $applicationKey = Join-Path $classesRoot 'Applications\ToolbagChineseLauncher.exe'
    $capabilitiesKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer\Capabilities'
    $backupKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer\FileAssociationBackup'

    $originalProgId = ''
    if (Test-Path -LiteralPath $backupKey) {
        $savedAssociation = Get-ItemProperty -LiteralPath $backupKey
        if ([int]$savedAssociation.HadDefault -eq 1 -and
            [string]$savedAssociation.DefaultValue -ne $progId) {
            $originalProgId = [string]$savedAssociation.DefaultValue
        }
    }
    if (-not $originalProgId) {
        $machineExtension = Get-Item -LiteralPath 'Registry::HKEY_LOCAL_MACHINE\Software\Classes\.tbscene' -ErrorAction SilentlyContinue
        if ($machineExtension) { $originalProgId = [string]$machineExtension.GetValue('', '') }
    }

    if (Test-Path -LiteralPath $backupKey) {
        $backup = Get-ItemProperty -LiteralPath $backupKey
        if ($originalProgId) {
            New-Item -Path $extensionKey -Force | Out-Null
            Set-Item -LiteralPath $extensionKey -Value $originalProgId
        } elseif (Test-Path -LiteralPath $extensionKey) {
            $key = $script:UserRegistryHive.OpenSubKey('Software\Classes\.tbscene', $true)
            $key.DeleteValue('', $false)
            $isEmpty = $key.SubKeyCount -eq 0 -and $key.ValueCount -eq 0
            $key.Close()
            if ([int]$backup.HadKey -eq 0 -and $isEmpty) {
                Remove-Item -LiteralPath $extensionKey -Force
            }
        }
    } elseif ($originalProgId) {
        New-Item -Path $extensionKey -Force | Out-Null
        Set-Item -LiteralPath $extensionKey -Value $originalProgId
    } elseif (Test-Path -LiteralPath $extensionKey) {
        $key = Get-Item -LiteralPath $extensionKey
        if ([string]$key.GetValue('', '') -eq $progId) {
            $key = $script:UserRegistryHive.OpenSubKey('Software\Classes\.tbscene', $true)
            $key.DeleteValue('', $false)
            $key.Close()
        }
    }
    if (Test-Path -LiteralPath $progIdKey) { Remove-Item -LiteralPath $progIdKey -Recurse -Force }
    if (Test-Path -LiteralPath $applicationKey) { Remove-Item -LiteralPath $applicationKey -Recurse -Force }
    if (Test-Path -LiteralPath $capabilitiesKey) { Remove-Item -LiteralPath $capabilitiesKey -Recurse -Force }
    Remove-ItemProperty -LiteralPath (Get-UserRegistryPath 'Software\RegisteredApplications') -Name '八猴5汉化版' -ErrorAction SilentlyContinue
    $openWithProgidsPath = 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.tbscene\OpenWithProgids'
    $openWithProgids = $script:UserRegistryHive.OpenSubKey($openWithProgidsPath, $true)
    if ($openWithProgids) {
        $openWithProgids.DeleteValue($progId, $false)
        $openWithProgids.Close()
    }
    if (Test-Path -LiteralPath $backupKey) {
        $backup = Get-ItemProperty -LiteralPath $backupKey
        Restore-HandlerProxy 0 'Software\Classes\MarmosetToolbag5.Scene' $backup
        Restore-HandlerProxy 1 'Software\Classes\MarmosetToolbag.Scene' $backup
        Restore-HandlerProxy 2 'Software\Classes\Applications\toolbag.exe' $backup
        Remove-Item -LiteralPath $backupKey -Recurse -Force
    }
    foreach ($handlerPath in @(
        'Software\Classes\MarmosetToolbag5.Scene',
        'Software\Classes\MarmosetToolbag.Scene',
        'Software\Classes\Applications\toolbag.exe')) {
        Remove-StaleHandlerProxy $handlerPath
    }
    Remove-LocalizedOpenWithEntries
    Update-ShellAssociations
    Write-Info '.tbscene 已恢复为安装汉化前的关联'
}


Write-Step '定位 Toolbag 安装目录'


$toolbag = Find-ToolbagDirectory -Override $ToolbagDir


if ($ToolbagDir -and -not $toolbag) {


    Show-Error '汉化安装' "指定目录中没有找到 toolbag.exe，未执行安装。`n`n$ToolbagDir"


    exit 1


}


if ($toolbag -and -not $ToolbagDir) {


    $operationName = if ($Uninstall) { '拆卸' } else { '安装' }


    if (-not (Ask-YesNo '八猴5汉化版' "检测到 Toolbag 目录：`n`n$toolbag`n`n是否在此目录执行$operationName？`n选择“否”可手动选择其他 toolbag.exe。")) {


        $toolbag = $null


    }


}


if (-not $toolbag) {


    # 最后兜底：弹文件选择框让用户手动选 toolbag.exe（不懂命令行的用户也能操作）


    Write-Info '未自动找到 Toolbag，请在弹出的窗口中选择 toolbag.exe'


    $dlg = New-Object System.Windows.Forms.OpenFileDialog


    $dlg.Title = '请选择 Toolbag 的 toolbag.exe'


    $dlg.Filter = 'toolbag.exe|toolbag.exe'


    if ($dlg.ShowDialog() -eq 'OK' -and (Test-Path -LiteralPath $dlg.FileName)) {


        $toolbag = Split-Path $dlg.FileName -Parent


    }


}


if (-not $toolbag) {


    Show-Error '汉化安装' "没有找到 Toolbag 5。`n请确认已安装，或用 -ToolbagDir 指定安装目录。"


    exit 1


}


Write-Info "Toolbag 目录: $toolbag"


# ================= 卸载模式 =================


if ($Uninstall) {


    Write-Step '卸载汉化插件'


    $pluginDir = Join-Path $toolbag 'data\ChineseLocalizer'

    $running = @(Get-ToolbagRelatedProcesses $toolbag)
    if ($running) {
        Write-Info '正在关闭 Toolbag 和旧汉化启动器...'
        Stop-ToolbagRelatedProcesses $toolbag
    }

    Restore-TbsceneAssociation


    # 1) 恢复安装前的主字体。备份文件本身就是是否替换过的可靠标记，
    #    即使旧安装状态注册表丢失也可以正常恢复。
    $fontDirectory = Join-Path $toolbag 'data\gui\font'
    $segoeFont = Join-Path $fontDirectory 'segoeui.slug'
    $segoeFontBackup = Join-Path $fontDirectory 'segoeui.slug.ChineseLocalizer.backup'
    if (Test-Path -LiteralPath $segoeFontBackup) {
        Copy-Item -LiteralPath $segoeFontBackup -Destination $segoeFont -Force
        Remove-Item -LiteralPath $segoeFontBackup -Force
        Write-Info '已恢复原版 segoeui.slug'
    }

    # 2) 删除插件目录


    if (Test-Path -LiteralPath $pluginDir) {

        Remove-DirectoryWithRetry $pluginDir '汉化插件目录'


        Write-Info "已删除插件目录: $pluginDir"


    } else {


        Write-Info '未发现插件目录（可能未安装汉化）'


    }


    # 删除安装时创建的中文快捷方式

    Remove-Shortcuts


    # 3) 清理运行时日志


    $logDir = Join-Path (Join-Path (Split-Path -Parent $OriginalUserAppData) 'Local') 'Marmoset Toolbag 5'


    foreach ($log in @('ChineseLocalizer_missing.tsv','ChineseLocalizer_trace.tsv','ChineseLocalizer_sniffer.tsv','ChineseLocalizer_sniffer.json')) {


        $lf = Join-Path $logDir $log


        if (Test-Path -LiteralPath $lf) { Remove-Item -LiteralPath $lf -Force; Write-Info "已清理日志: $log" }


    }

    # 清理旧版插件目录（关闭进程后再删除，避免旧 DLL 被占用）
    $oldPlugin = Join-Path $toolbag 'data\plugin\ChineseLocalizer'
    if (Test-Path -LiteralPath $oldPlugin) { Remove-DirectoryWithRetry $oldPlugin '旧版插件目录'; Write-Info "已清理旧插件目录: $oldPlugin" }

    $stateKey = Get-UserRegistryPath 'Software\MarmosetChineseLocalizer'
    Remove-ItemProperty -LiteralPath $stateKey -Name ToolbagDir -ErrorAction SilentlyContinue
    Remove-ItemProperty -LiteralPath $stateKey -Name ChineseFontInstalled -ErrorAction SilentlyContinue
    Remove-ItemProperty -LiteralPath $stateKey -Name SegoeUiFontReplaced -ErrorAction SilentlyContinue


    Write-Host "`n======================================================" -ForegroundColor Green


    Write-Host "  汉化已卸载完成！" -ForegroundColor Green


    Write-Host "======================================================" -ForegroundColor Green


    Write-Info "插件目录、快捷方式和文件关联已清理，Toolbag 已恢复原启动方式。"


    Write-Host ""


    if (-not $Quiet) { Read-Host '按回车键退出' }


    exit 0


}


# ---------- 3. 确认 dist 产物齐全（默认不编译，零环境依赖） ----------


$requiredFiles = @('dictionary_zh.json', 'ToolbagChineseHook.dll',
                   'ToolbagChineseLauncher.exe', 'segoeui.slug', 'tbscene.ico')


$missing = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Dist $_)) })


if ($missing.Count -gt 0) {


    if ($Build) {


        Write-Step '自动编译（-Build 指定）'


        Write-Info '正在调用 build.bat，需要安装 Visual Studio C++ 生成工具...'


        Push-Location $script:ProjectRoot


        try {


            & cmd /c "scripts\build.bat"


            if ($LASTEXITCODE -ne 0) { throw "build.bat 返回错误码 $LASTEXITCODE" }


        } finally { Pop-Location }


        $missing = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Dist $_)) })


    }


    if ($missing.Count -gt 0) {


        Show-Error '汉化安装' "dist 目录缺少文件: $($missing -join ', ')`n`n请使用包含 dist（预编译产物）的完整版本，无需安装任何编译环境。`n（开发者如需从源码重新编译，可加 -Build 参数，但需要 Visual Studio C++ 生成工具。）"


        exit 1


    }


}


# ---------- 4. 若 Toolbag 正在运行，先处理（避免文件占用） ----------


$running = @(Get-ToolbagRelatedProcesses $toolbag)


if ($running) {


    Write-Step '检测到 Toolbag 正在运行'


    if (Ask-YesNo '汉化安装' "检测到 Toolbag 或旧汉化启动器正在运行。安装需要替换汉化文件。`n是否自动关闭相关程序？`n（选“否”则取消安装）") {


        Stop-ToolbagRelatedProcesses $toolbag


        Write-Info '已关闭 Toolbag'


    } else {


        Show-Error '汉化安装' '已取消安装，请先关闭 Toolbag 再重试。'


        exit 1


    }


}


# ---------- 5. 安装（失败时整体回滚插件目录） ----------


$pluginDir = Join-Path $toolbag 'data\ChineseLocalizer'

$script:installInProgress = $true
$script:rollbackPluginDir = $pluginDir
$script:rollbackToolbag = $toolbag
$script:hadExistingPlugin = Test-Path -LiteralPath $pluginDir
$script:rollbackPluginBackup = Join-Path $env:TEMP ('ChineseLocalizer_backup_' + [Guid]::NewGuid().ToString('N'))
if ($script:hadExistingPlugin) {
    Copy-Item -LiteralPath $pluginDir -Destination $script:rollbackPluginBackup -Recurse -Force
}

# 清理旧版插件目录（不再使用 data\plugin 下的位置，避免残留菜单项）
$oldPlugin = Join-Path $toolbag 'data\plugin\ChineseLocalizer'
if (Test-Path -LiteralPath $oldPlugin) { Remove-DirectoryWithRetry $oldPlugin '旧版插件目录'; Write-Info "已清理旧插件目录: $oldPlugin" }

$script:installedFiles = New-Object System.Collections.Generic.List[string]


function Copy-PluginFile([string]$source, [string]$destination, [string]$label) {


    if (-not (Test-Path -LiteralPath $source)) {


        Write-Info "跳过 $label（源文件不存在: $source）"


        return


    }


    $parent = Split-Path -Parent $destination


    New-Item -ItemType Directory -Path $parent -Force | Out-Null


    Copy-Item -LiteralPath $source -Destination $destination -Force


    $script:installedFiles.Add($label)


    Write-Info "已安装 $label"


}


Write-Step '安装插件文件（含字典）'


New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null


foreach ($name in @('dictionary_zh.json', 'ToolbagChineseHook.dll',
                    'ToolbagChineseLauncher.exe', 'tbscene.ico')) {


    Copy-PluginFile (Join-Path $Dist $name) (Join-Path $pluginDir $name) $name


}

# F12 sniffer output lives beside the plugin. Program Files is normally
# read-only for standard users, so grant only the installing user access to
# this one pre-created log file (not to the plugin directory or binaries).
$oldSnifferLog = Join-Path $pluginDir 'ChineseLocalizer_sniffer.tsv'
if (Test-Path -LiteralPath $oldSnifferLog) {
    Remove-Item -LiteralPath $oldSnifferLog -Force
}
$snifferLog = Join-Path $pluginDir 'ChineseLocalizer_sniffer.json'
if (-not (Test-Path -LiteralPath $snifferLog)) {
    '{"captured_at": null, "duration_ms": 1500, "entries": []}' | Set-Content -LiteralPath $snifferLog -Encoding UTF8
}
try {
    & icacls.exe $snifferLog /grant:r "*${OriginalUserSid}:(M)" /c | Out-Null
    Write-Info '已配置 F12 嗅探日志写入权限'
} catch {
    Write-Info ('配置嗅探日志权限失败: ' + $_.Exception.Message)
}


Write-Step '检查中文字体'

$fontDirectory = Join-Path $toolbag 'data\gui\font'
$nativeChineseFont = Join-Path $fontDirectory 'notosans_chinese.slug'
$segoeFont = Join-Path $fontDirectory 'segoeui.slug'
$segoeFontBackup = Join-Path $fontDirectory 'segoeui.slug.ChineseLocalizer.backup'
$script:fontOriginalPath = $segoeFont
$script:fontBackupPath = $segoeFontBackup
$segoeFontReplaced = Test-Path -LiteralPath $segoeFontBackup

if (Test-Path -LiteralPath $nativeChineseFont) {
    Write-Info '已检测到 notosans_chinese.slug，无需替换主字体。'
} else {
    if (-not (Test-Path -LiteralPath $segoeFont)) {
        throw "未找到需要备份的原字体：$segoeFont"
    }
    if (-not (Test-Path -LiteralPath $segoeFontBackup)) {
        Copy-Item -LiteralPath $segoeFont -Destination $segoeFontBackup -Force
        $script:fontBackupCreatedByCurrentInstall = $true
        Write-Info "已备份原字体：$segoeFontBackup"
    } else {
        Write-Info '已存在原字体备份，保留该备份，不重复覆盖。'
    }
    Copy-Item -LiteralPath (Join-Path $Dist 'segoeui.slug') -Destination $segoeFont -Force
    $segoeFontReplaced = $true
    $script:installedFiles.Add('segoeui.slug（中文字体）')
    Write-Info '已用中文字体替换 segoeui.slug'
}

# ---------- 6. 自检 ----------


Write-Step '安装自检'


$checkOk = $true


foreach ($name in @('dictionary_zh.json', 'ToolbagChineseHook.dll',
                    'ToolbagChineseLauncher.exe', 'tbscene.ico')) {


    $p = Join-Path $pluginDir $name


    if (Test-Path -LiteralPath $p) { Write-Info "  [OK] 插件  $name" }


    else { Write-Info "  [缺] 插件  $name"; $checkOk = $false }


}


if (-not $checkOk) {


    Show-Error '汉化安装' '安装自检未通过，有文件缺失。请检查上方输出。'


    exit 1


}


Write-Step '创建桌面 / 开始菜单快捷方式'

Remove-Shortcuts

New-Shortcuts -pluginDir $pluginDir -toolbag $toolbag

Write-Step '关联 Toolbag 场景文件'

Install-TbsceneAssociation -pluginDir $pluginDir -toolbag $toolbag

$stateKey = New-Item -Path (Get-UserRegistryPath 'Software\MarmosetChineseLocalizer') -Force
New-ItemProperty -LiteralPath $stateKey.PSPath -Name ToolbagDir -Value $toolbag -PropertyType String -Force | Out-Null
New-ItemProperty -LiteralPath $stateKey.PSPath -Name SegoeUiFontReplaced -Value ([int]$segoeFontReplaced) -PropertyType DWord -Force | Out-Null

$script:installInProgress = $false
if (Test-Path -LiteralPath $script:rollbackPluginBackup) {
    Remove-Item -LiteralPath $script:rollbackPluginBackup -Recurse -Force
}


# ---------- 7. 完成提示 ----------


Write-Host "`n======================================================" -ForegroundColor Green


Write-Host "  汉化插件安装完成！" -ForegroundColor Green


Write-Host "======================================================" -ForegroundColor Green


Write-Info "安装位置: $pluginDir"


Write-Info "已安装: $($script:installedFiles -join ', ')"


Write-Host "`n如何启动："


Write-Host "  1. 打开插件目录：$pluginDir"


Write-Host "  2. 双击 ToolbagChineseLauncher.exe 启动 Toolbag（必须用它启动才有汉化）"


Write-Host ""


if (-not $Quiet) { Read-Host '按回车键退出' }


exit 0


