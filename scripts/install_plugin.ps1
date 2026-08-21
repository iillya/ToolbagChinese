$ErrorActionPreference = 'Stop'
$source = 'C:\Users\win10\Desktop\Toolbag\dist'
$destination = 'C:\Program Files\Marmoset\Toolbag 5\data\plugin\ChineseLocalizer'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'

New-Item -ItemType Directory -Path $destination -Force | Out-Null
foreach ($name in @('__main__.py', 'dictionary.txt', 'ToolbagChineseHook.dll', 'ToolbagChineseLauncher.exe', 'trace.enabled')) {
    $target = Join-Path $destination $name
    if (Test-Path -LiteralPath $target) {
        Copy-Item -LiteralPath $target -Destination ($target + '.bak-' + $stamp)
    }
    Copy-Item -LiteralPath (Join-Path $source $name) -Destination $target -Force
}
