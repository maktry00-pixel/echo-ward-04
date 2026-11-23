[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskPackage=Join-Path $taskRoot 'Saved/PackagedWindows'
$taskExe=Join-Path $taskPackage 'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/NativeMenuV16'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
# Intentionally load the real hospital and forest, not WardLegacyCampaign.
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardMenuCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')
$taskStarted=Get-Date
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
if($taskProcess.ExitCode -ne 0){throw 'Native menu runtime failed.'}
$taskPath=Join-Path $taskSaved 'Prototype/menu_check.json'
if(!(Test-Path -LiteralPath $taskPath) -or (Get-Item -LiteralPath $taskPath).LastWriteTime -lt $taskStarted){throw 'No fresh native menu result.'}
$taskResult=Get-Content -Raw -LiteralPath $taskPath | ConvertFrom-Json
if(!$taskResult.success -or $taskResult.count -lt 45){throw 'Native menu and game entrance checks failed.'}
if(!@($taskResult.checks | Where-Object {$_.name -eq 'title_and_all_submenu_sets_are_outside_the_campus_forest' -and $_.passed}).Count){throw 'No forest isolation check.'}
if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Native menu rendering warning.'}
[ordered]@{success=$true;mode='NativeMenuV16';checked_at=(Get-Date).ToString('o');test_saved=$taskSaved;executable_sha256=(Get-FileHash -LiteralPath $taskExe).Hash;map_sha256=(Get-FileHash -LiteralPath (Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/hospital_world.json')).Hash;checks=[int]$taskResult.count} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'verified_native_menu_v16.json') -Encoding UTF8
