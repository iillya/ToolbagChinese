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
    [switch]$Quiet
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms

$script:ProjectRoot = Split-Path -Parent $PSScriptRoot
$script:Dist = Join-Path $script:ProjectRoot 'dist'
$script:Scripts = Join-Path $script:ProjectRoot 'scripts'

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
    Write-Host '需要管理员权限，正在重新以管理员身份启动...' -ForegroundColor Yellow
    $argList = '-NoProfile -ExecutionPolicy Bypass -File "' + $MyInvocation.MyCommand.Path + '"'
    if ($ToolbagDir) { $argList += ' -ToolbagDir "' + $ToolbagDir + '"' }
    if ($Build) { $argList += ' -Build' }
    if ($Quiet) { $argList += ' -Quiet' }
    try {
        Start-Process powershell.exe -Verb RunAs -ArgumentList $argList -Wait
        exit
    } catch {
        Show-Error '汉化安装' "无法获取管理员权限，安装已取消。`n请右键“一键安装汉化.bat”选择“以管理员身份运行”。"
        exit 1
    }
}

# ---------- 2. 定位 Toolbag 安装目录 ----------
function Find-Toolbag([string]$Override) {
    if ($Override) {
        $cand = Join-Path $Override 'toolbag.exe'
        if (Test-Path -LiteralPath $cand) { return (Resolve-Path -LiteralPath $Override).Path }
        return $null
    }
    $candidates = @(
        "$env:ProgramFiles\Marmoset\Toolbag 5",
        "${env:ProgramFiles(x86)}\Marmoset\Toolbag 5",
        "$env:LOCALAPPDATA\Marmoset Toolbag 5",
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
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*'
    )
    foreach ($root in $roots) {
        foreach ($key in Get-ItemProperty $root -ErrorAction SilentlyContinue) {
            $loc = $key.InstallLocation
            if (-not $loc -and $key.DisplayIcon) { $loc = Split-Path $key.DisplayIcon -Parent }
            if ($loc -and (Test-Path -LiteralPath (Join-Path $loc 'toolbag.exe'))) {
                return (Resolve-Path -LiteralPath $loc).Path
            }
        }
    }
    return $null
}

Write-Step '定位 Toolbag 安装目录'
$toolbag = Find-Toolbag -Override $ToolbagDir
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
    $pluginDir = Join-Path $toolbag 'data\plugin\ChineseLocalizer'
    $fontDir   = Join-Path $toolbag 'data\gui\font'
    $fontFile  = Join-Path $fontDir 'segoeui.slug'

    # 关闭正在运行的 Toolbag
    $running = Get-Process -Name 'toolbag' -ErrorAction SilentlyContinue
    if ($running) {
        Write-Info '正在关闭 Toolbag...'
        $running | Stop-Process -Force
        Start-Sleep -Seconds 2
    }

    $ok = $true

    # 1) 删除插件目录
    if (Test-Path -LiteralPath $pluginDir) {
        Remove-Item -LiteralPath $pluginDir -Recurse -Force
        Write-Info "已删除插件目录: $pluginDir"
    } else {
        Write-Info '未发现插件目录（可能未安装汉化）'
    }

    # 2) 还原原版字体
    $backupFont = Get-ChildItem -LiteralPath $fontDir -Filter 'segoeui.slug.bak-*' -File -ErrorAction SilentlyContinue |
                  Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($backupFont) {
        Remove-Item -LiteralPath $fontFile -Force -ErrorAction SilentlyContinue
        Copy-Item -LiteralPath $backupFont.FullName -Destination $fontFile -Force
        Remove-Item -LiteralPath $backupFont.FullName -Force
        Write-Info "已还原原版字体: $fontFile"
    } else {
        Write-Info '未找到字体备份，跳过还原（如需还原请手动处理 segoeui.slug）'
    }

    # 3) 清理运行时日志
    $logDir = Join-Path $env:LOCALAPPDATA 'Marmoset Toolbag 5'
    foreach ($log in @('ChineseLocalizer_missing.tsv','ChineseLocalizer_trace.tsv')) {
        $lf = Join-Path $logDir $log
        if (Test-Path -LiteralPath $lf) { Remove-Item -LiteralPath $lf -Force; Write-Info "已清理日志: $log" }
    }

    Write-Host "`n======================================================" -ForegroundColor Green
    Write-Host "  汉化已卸载完成！" -ForegroundColor Green
    Write-Host "======================================================" -ForegroundColor Green
    Write-Info "字体已还原，插件目录已删除，原版 Toolbag 恢复正常。"
    Write-Host ""
    if (-not $Quiet) { Read-Host '按回车键退出' }
    exit 0
}

# ---------- 3. 确认 dist 产物齐全（默认不编译，零环境依赖） ----------
$needed = @('__main__.py', 'dictionary.txt', 'dictionary_assets.txt', 'ToolbagChineseHook.dll', 'ToolbagChineseLauncher.exe', 'deng_ui.slug')
$missing = @($needed | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Dist $_)) })
if ($missing.Count -gt 0) {
    if ($Build) {
        Write-Step '自动编译（-Build 指定）'
        Write-Info '正在调用 build.bat，需要安装 Visual Studio C++ 生成工具...'
        Push-Location $script:ProjectRoot
        try {
            & cmd /c "scripts\build.bat"
            if ($LASTEXITCODE -ne 0) { throw "build.bat 返回错误码 $LASTEXITCODE" }
        } finally { Pop-Location }
        $missing = @($needed | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Dist $_)) })
    }
    if ($missing.Count -gt 0) {
        Show-Error '汉化安装' "dist 目录缺少文件: $($missing -join ', ')`n`n请使用包含 dist（预编译产物）的完整版本，无需安装任何编译环境。`n（开发者如需从源码重新编译，可加 -Build 参数，但需要 Visual Studio C++ 生成工具。）"
        exit 1
    }
}

# ---------- 4. 若 Toolbag 正在运行，先处理（避免文件占用） ----------
$running = Get-Process -Name 'toolbag' -ErrorAction SilentlyContinue
if ($running) {
    Write-Step '检测到 Toolbag 正在运行'
    if (Ask-YesNo '汉化安装' "检测到 Toolbag 正在运行。安装需要覆盖程序目录里的文件，建议先关闭。`n是否自动关闭 Toolbag？`n（选“否”则取消安装）") {
        $running | Stop-Process -Force
        Start-Sleep -Seconds 2
        Write-Info '已关闭 Toolbag'
    } else {
        Show-Error '汉化安装' '已取消安装，请先关闭 Toolbag 再重试。'
        exit 1
    }
}

# ---------- 5. 安装（每次都先备份） ----------
$pluginDir = Join-Path $toolbag 'data\plugin\ChineseLocalizer'
$fontTarget = Join-Path $toolbag 'data\gui\font\segoeui.slug'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$script:installed = New-Object System.Collections.Generic.List[string]

function Install-WithBackup([string]$src, [string]$dst, [string]$label) {
    if (-not (Test-Path -LiteralPath $src)) {
        Write-Info "跳过 $label（源文件不存在: $src）"
        return
    }
    $parent = Split-Path -Parent $dst
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    if (Test-Path -LiteralPath $dst) {
        Copy-Item -LiteralPath $dst -Destination ($dst + ".bak-$stamp") -Force
    }
    Copy-Item -LiteralPath $src -Destination $dst -Force
    $script:installed.Add($label)
    Write-Info "已安装 $label"
}

Write-Step '安装插件文件（含字典）'
New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
foreach ($name in @('__main__.py', 'dictionary.txt', 'dictionary_assets.txt', 'ToolbagChineseHook.dll', 'ToolbagChineseLauncher.exe')) {
    Install-WithBackup (Join-Path $Dist $name) (Join-Path $pluginDir $name) $name
}
$traceMarker = Join-Path $pluginDir 'trace.enabled'
if (Test-Path -LiteralPath $traceMarker) {
    Remove-Item -LiteralPath $traceMarker -Force
    Write-Info '已清理 trace.enabled（运行时追踪默认关闭）'
}

Write-Step '安装中文字体'
Install-WithBackup (Join-Path $Dist 'deng_ui.slug') $fontTarget 'deng_ui.slug'

Write-Step '清理历史备份'
$bakFiles = @(Get-ChildItem -LiteralPath $pluginDir -File -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -like '*.bak-*' -or $_.Name -like '*.before-static-merge' })
if ($bakFiles.Count -gt 0) {
    $bakFiles | Remove-Item -Force
    Write-Info "已删除 $($bakFiles.Count) 个历史备份/旧文件"
} else {
    Write-Info '目录已干净，无需清理'
}

# ---------- 6. 自检 ----------
Write-Step '安装自检'
$checkOk = $true
foreach ($name in @('__main__.py', 'dictionary.txt', 'dictionary_assets.txt', 'ToolbagChineseHook.dll', 'ToolbagChineseLauncher.exe')) {
    $p = Join-Path $pluginDir $name
    if (Test-Path -LiteralPath $p) { Write-Info "  [OK] 插件  $name" }
    else { Write-Info "  [缺] 插件  $name"; $checkOk = $false }
}
if (Test-Path -LiteralPath $fontTarget) { Write-Info "  [OK] 字体  segoeui.slug" }
else { Write-Info "  [缺] 字体  segoeui.slug"; $checkOk = $false }

if (-not $checkOk) {
    Show-Error '汉化安装' '安装自检未通过，有文件缺失。请检查上方输出。'
    exit 1
}

# ---------- 7. 完成提示 ----------
Write-Host "`n======================================================" -ForegroundColor Green
Write-Host "  汉化插件安装完成！" -ForegroundColor Green
Write-Host "======================================================" -ForegroundColor Green
Write-Info "安装位置: $pluginDir"
Write-Info "已安装: $($script:installed -join ', ')"
Write-Host "`n如何启动："
Write-Host "  1. 打开插件目录：$pluginDir"
Write-Host "  2. 双击 ToolbagChineseLauncher.exe 启动 Toolbag（必须用它启动才有汉化）"
Write-Host "  3. 想还原原版：把 data\gui\font\segoeui.slug.bak-* 改名回 segoeui.slug，并删掉汉化插件目录即可"
Write-Host ""
if (-not $Quiet) { Read-Host '按回车键退出' }
exit 0
