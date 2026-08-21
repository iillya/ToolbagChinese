$ErrorActionPreference = 'Stop'
$source = 'C:\Users\win10\Desktop\Toolbag\dist\dictionary.txt'
$target = 'C:\Program Files\Marmoset\Toolbag 5\data\plugin\ChineseLocalizer\dictionary.txt'
$backup = $target + '.before-static-merge'
if (!(Test-Path -LiteralPath $backup)) {
    Copy-Item -LiteralPath $target -Destination $backup
}
Copy-Item -LiteralPath $source -Destination $target -Force
