[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskExe=Join-Path $taskRoot 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/RealismV30'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardCaptureRealism30','-WardRealismReview30','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$taskStarted=Get-Date
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
if($taskProcess.ExitCode -ne 0){throw 'Realism v30 render failed'}
$taskPath=Join-Path $taskSaved 'Prototype/book_world_check.json'
if(!(Test-Path -LiteralPath $taskPath) -or (Get-Item -LiteralPath $taskPath).LastWriteTime -lt $taskStarted){throw 'No fresh realism check'}
$taskResult=Get-Content -Raw -LiteralPath $taskPath | ConvertFrom-Json
if(!$taskResult.success){$taskResult.checks | Where-Object {!$_.passed} | ConvertTo-Json;throw 'Realism runtime checks failed'}
$taskData=Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'Content/CampaignData/echo_realism_v30.json') | ConvertFrom-Json
if($taskResult.realism30_props -ne $taskData.props.Count -or $taskResult.custom_bounds_failures -ne 0){throw 'New service placements missing'}
foreach($taskView in $taskData.captures){$taskPng=Join-Path $taskSaved ('Prototype/BookWorldCaptures/world_'+$taskView.id+'.png');if(!(Test-Path -LiteralPath $taskPng) -or (Get-Item -LiteralPath $taskPng).LastWriteTime -lt $taskStarted){throw ('Missing fresh frame '+$taskView.id)}}
if(Select-String -LiteralPath (Join-Path $taskSaved 'package_test.log') -Pattern 'Default Material will be used|Failed to compile Material|GPU Crashed|Out of video memory|is not a valid parent for MaterialInstanceDynamic'){throw 'Realism rendering warning'}
[ordered]@{success=$true;checked_at=(Get-Date).ToString('o');test_saved=$taskSaved;checks=$taskResult.count;executable_sha256=(Get-FileHash -LiteralPath $taskExe).Hash;data_sha256=(Get-FileHash -LiteralPath (Join-Path $taskRoot 'Content/CampaignData/echo_realism_v30.json')).Hash;props=$taskResult.realism30_props;captures=$taskData.captures.Count} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot 'Saved/PackagedWindows/verified_realism_v30.json') -Encoding UTF8
Write-Output ('REALISM_V30_PASS '+$taskResult.count)
