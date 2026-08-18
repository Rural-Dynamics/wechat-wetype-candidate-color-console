$ErrorActionPreference = 'Stop'

$taskName = 'WeTypeColorPatch'
$exe = Join-Path $PSScriptRoot 'WeTypeSkinLauncher.exe'
$user = "$env:USERDOMAIN\$env:USERNAME"

Stop-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
$action = New-ScheduledTaskAction -Execute $exe -Argument '--watch'
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $user
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Highest

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $principal `
    -Description 'Native WeType candidate color patch' -Force | Out-Null
Start-ScheduledTask -TaskName $taskName
