[CmdletBinding()]
param([switch]$Pictures)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskPackage=Join-Path $taskRoot 'Saved/PackagedWindows'
$taskExe=Join-Path $taskPackage 'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/ArrivalV29'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardArrivalCheck','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')
if($Pictures){$taskArgs+='-WardArrivalPictures'}
$taskStarted=Get-Date
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
if($taskProcess.ExitCode -ne 0){throw 'Arrival runtime failed.'}
$taskPath=Join-Path $taskSaved 'Prototype/arrival29_check.json'
if(!(Test-Path -LiteralPath $taskPath) -or (Get-Item -LiteralPath $taskPath).LastWriteTime -lt $taskStarted){throw 'No fresh arrival result.'}
$taskResult=Get-Content -Raw -LiteralPath $taskPath | ConvertFrom-Json
if(!$taskResult.success -or $taskResult.count -lt 20){$taskResult.checks | Where-Object {!$_.passed} | ConvertTo-Json;throw 'Arrival chronology, route or save checks failed.'}
if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Non-Nanite Marking Job Queue overflow|Virtual Shadow Map.*overflow|surface cache atlas oversubscribed|Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy|is not a valid parent for MaterialInstanceDynamic'){throw 'Arrival rendering warning.'}
if($Pictures){foreach($taskName in $taskResult.screenshots){$taskPng=Join-Path $taskSaved ('Prototype/ArrivalCaptures/'+$taskName);if(!(Test-Path -LiteralPath $taskPng) -or (Get-Item -LiteralPath $taskPng).LastWriteTime -lt $taskStarted){throw ('No fresh arrival frame '+$taskName)}}}
[ordered]@{success=$true;mode='ArrivalV29';checked_at=(Get-Date).ToString('o');test_saved=$taskSaved;executable_sha256=(Get-FileHash -LiteralPath $taskExe).Hash;arrival_data_sha256=(Get-FileHash -LiteralPath (Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/echo_arrival_v29.json')).Hash;checks=[int]$taskResult.count;screenshots=@($taskResult.screenshots)} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'verified_arrival_v29.json') -Encoding UTF8
Write-Output ('ARRIVAL_V29_PASS '+$taskResult.count)
