$ErrorActionPreference = 'Stop'
$source = 'C:\Users\win10\Desktop\Toolbag\dist\deng_ui.slug'
$target = 'C:\Program Files\Marmoset\Toolbag 5\data\gui\font\segoeui.slug'
$backup = $target + '.original-5.02'
if (!(Test-Path -LiteralPath $backup)) {
    Copy-Item -LiteralPath $target -Destination $backup
}
Copy-Item -LiteralPath $source -Destination $target -Force
