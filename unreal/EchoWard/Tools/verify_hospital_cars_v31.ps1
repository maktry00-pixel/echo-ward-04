[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskExe=Join-Path $taskRoot 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/CarsV31'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardCarsReview31','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$taskStarted=Get-Date
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
if($taskProcess.ExitCode -ne 0){throw 'Cars31 rendering failed'}
$taskPath=Join-Path $taskSaved 'Prototype/book_world_check.json'
if(!(Test-Path -LiteralPath $taskPath) -or (Get-Item -LiteralPath $taskPath).LastWriteTime -lt $taskStarted){throw 'No fresh Cars31 result'}
$taskResult=Get-Content -Raw -LiteralPath $taskPath | ConvertFrom-Json
if(!$taskResult.success -or $taskResult.cars31_count -ne 35 -or $taskResult.cars31_sources -ne 31){$taskResult.checks | Where-Object {!$_.passed} | ConvertTo-Json;throw 'Cars31 world check failed'}
$taskData=Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'Content/CampaignData/echo_cars_v31.json') | ConvertFrom-Json
foreach($taskView in $taskData.captures){$taskPng=Join-Path $taskSaved ('Prototype/BookWorldCaptures/world_'+$taskView.id+'.png');if(!(Test-Path -LiteralPath $taskPng) -or (Get-Item -LiteralPath $taskPng).LastWriteTime -lt $taskStarted){throw ('No fresh vehicle frame '+$taskView.id)}}
if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Non-Nanite Marking Job Queue overflow|Virtual Shadow Map.*overflow|surface cache atlas oversubscribed|Default Material will be used|Failed to compile Material|GPU Crashed|Out of video memory|is not a valid parent for MaterialInstanceDynamic'){throw 'Vehicle rendering warning'}
[ordered]@{success=$true;checked_at=(Get-Date).ToString('o');test_saved=$taskSaved;checks=$taskResult.count;executable_sha256=(Get-FileHash -LiteralPath $taskExe).Hash;cars_data_sha256=(Get-FileHash -LiteralPath (Join-Path $taskRoot 'Content/CampaignData/echo_cars_v31.json')).Hash;parked=$taskResult.cars31_count;native_sources=$taskResult.cars31_sources;captures=$taskData.captures.Count} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot 'Saved/PackagedWindows/verified_cars_v31.json') -Encoding UTF8
Write-Output ('CARS_V31_PASS '+$taskResult.count)
