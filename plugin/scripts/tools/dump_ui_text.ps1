# dump_ui_text.ps1
#
# Runtime complement to the static scanner: while Toolbag is running (and the
# Chinese localizer hook is active), this walks the whole UI Automation tree of
# every toolbag window and dumps every visible text string to:
#     reports\ui_runtime_dump.txt
#
# This catches text that is currently displayed, even if it never went through
# the static sources (e.g. dynamic/preset/script-provided strings). Combined
# with the GDI capture hooks (ChineseLocalizer_missing.tsv) it forms a complete
# runtime "no-omission" net.
#
# Usage:
#   1. Start Toolbag via ToolbagChineseLauncher.exe (hook is injected).
#   2. Optionally open the panels/dialogs you want to capture.
#   3. Run:  powershell -ExecutionPolicy Bypass -File scripts\dump_ui_text.ps1
#      (add -ExpandMenus to also open menu/submenu items so their text is captured)
#
# Requires: .NET Framework (UIAutomation is built into Windows).
param(
    [switch]$ExpandMenus
)
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$repo   = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$report = Join-Path $repo 'reports'
$out    = Join-Path $report 'ui_runtime_dump.txt'
New-Item -ItemType Directory -Path $report -Force | Out-Null

$seen = New-Object 'System.Collections.Generic.HashSet[string]'
$rows = New-Object System.Collections.Generic.List[string]

function Get-Text([System.Windows.Automation.AutomationElement]$e) {
    $parts = New-Object System.Collections.Generic.List[string]
    try {
        $n = $e.Current.Name
        if ($n) { $parts.Add($n) }
    } catch {}
    try {
        $help = $e.Current.HelpText
        if ($help) { $parts.Add($help) }
    } catch {}
    try {
        $acc = $e.GetCurrentPattern([System.Windows.Automation.AutomationPattern]::LegacyIAccessiblePattern)
        if ($acc) {
            $v = $acc.Current.Name
            if ($v) { $parts.Add($v) }
        }
    } catch {}
    try {
        $vp = $e.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        if ($vp) { $parts.Add($vp.Current.Value) }
    } catch {}
    return ($parts | Select-Object -Unique)
}

function Expand-IfMenu([System.Windows.Automation.AutomationElement]$e) {
    if (-not $ExpandMenus) { return }
    try {
        $pattern = $e.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
        if ($pattern -and $pattern.Current.ExpandCollapseState -ne
            [System.Windows.Automation.ExpandCollapseState]::Expanded) {
            $pattern.Expand()
        }
    } catch {}
}

function Walk([System.Windows.Automation.AutomationElement]$e, [int]$depth) {
    if ($depth -gt 40) { return }
    foreach ($t in Get-Text $e) {
        $t = $t.Trim()
        if ($t.Length -ge 1 -and $seen.Add($t)) {
            $rows.Add($t)
        }
    }
    # reveal submenus (menu / menu item) before walking children
    Expand-IfMenu $e
    try {
        $children = $e.FindAll([System.Windows.Automation.TreeScope]::Children,
                               [System.Windows.Automation.Condition]::TrueCondition)
        if ($children) {
            foreach ($c in $children) { Walk $c ($depth + 1) }
        }
    } catch {}
}

$proc = Get-Process -Name 'toolbag' -ErrorAction SilentlyContinue
if (-not $proc) {
    Write-Host 'Toolbag is not running. Start it first (via ToolbagChineseLauncher.exe).' -ForegroundColor Yellow
    exit 1
}

$root = [System.Windows.Automation.AutomationElement]::RootElement
$pids = $proc.Id | Select-Object -Unique
foreach ($pid in $pids) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $pid)
    $windows = $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
    foreach ($w in $windows) {
        try {
            Write-Host ("Window: {0}  (pid {1})" -f $w.Current.Name, $pid)
            Walk $w 0
        } catch {}
    }
}

$rows | Sort-Object -Unique | Set-Content -LiteralPath $out -Encoding UTF8
Write-Host ("Done. Captured {0} unique strings -> {1}" -f $rows.Count, $out)
