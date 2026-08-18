$ErrorActionPreference = 'Stop'

$taskName = 'WeTypeColorPatch'

Stop-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue

Write-Host "Removed scheduled task: $taskName"

