[CmdletBinding()]
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9_]+$')][string]$Label)
$ErrorActionPreference='Stop'
$perfProject=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$perfExe=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$perfUser=Join-Path $perfProject ('Saved/PackageTests/PerformanceV25_'+$Label)
$perfSaved=Join-Path $perfUser 'Saved'
New-Item -ItemType Directory -Path $perfSaved -Force | Out-Null
$perfArgs=@(('-UserDir='+$perfUser),'-WardHospitalBenchmark','-WardHorrorBenchmark28',('-WardPerfLabel='+$Label),'-unattended','-nosound','-NoLiveCoding','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $perfSaved 'performance.log')))
$perfStart=Get-Date
$perfExecutableHash=(Get-FileHash -LiteralPath $perfExe -Algorithm SHA256).Hash.ToLowerInvariant()
$perfDetailPath=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_detail_v26.json'
$perfDetailHash=if(Test-Path -LiteralPath $perfDetailPath){(Get-FileHash -LiteralPath $perfDetailPath -Algorithm SHA256).Hash.ToLowerInvariant()}else{''}
$perfHorror28Path=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_horror_v28.json'
$perfHorror28Hash=if(Test-Path -LiteralPath $perfHorror28Path){(Get-FileHash -LiteralPath $perfHorror28Path -Algorithm SHA256).Hash.ToLowerInvariant()}else{''}
$perfRealismPath=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_realism_v30.json'
$perfRealismHash=if(Test-Path -LiteralPath $perfRealismPath){(Get-FileHash -LiteralPath $perfRealismPath).Hash.ToLowerInvariant()}else{''}
$perfCars31Path=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_cars_v31.json'
$perfCars31Hash=if(Test-Path -LiteralPath $perfCars31Path){(Get-FileHash -LiteralPath $perfCars31Path).Hash.ToLowerInvariant()}else{''}
$perfQuality32Path=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_car_quality_v32.json'
$perfQuality32Hash=if(Test-Path -LiteralPath $perfQuality32Path){(Get-FileHash -LiteralPath $perfQuality32Path).Hash.ToLowerInvariant()}else{''}
$perfArchiveHashes=[ordered]@{}
foreach($perfArchive in (Get-ChildItem -LiteralPath (Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/Paks') -File | Where-Object {$_.Extension -in @('.pak','.ucas','.utoc')} | Sort-Object Name)){$perfArchiveHashes[$perfArchive.Name]=(Get-FileHash -LiteralPath $perfArchive.FullName).Hash.ToLowerInvariant()}
$perfProcess=Start-Process -FilePath $perfExe -ArgumentList (($perfArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $perfSaved 'console.log') -RedirectStandardError (Join-Path $perfSaved 'stderr.log')
if($perfProcess.ExitCode -ne 0){throw 'Performance capture failed'}
$perfResult=Join-Path $perfSaved ('Prototype/hospital_perf_'+$Label+'.json')
if(!(Test-Path -LiteralPath $perfResult) -or (Get-Item -LiteralPath $perfResult).LastWriteTime -lt $perfStart){throw 'No fresh performance result'}
$perfData=Get-Content -LiteralPath $perfResult -Raw | ConvertFrom-Json
if(!$perfData.success -or $perfData.real_instances -ne 16107 -or $perfData.quality_instances -ne 28850 -or $perfData.screen_percentage -ne 100){throw 'Performance run changed the scene or resolution'}
if($perfData.views.Count -ne 8 -or $perfData.horror_traces28 -ne 910){throw 'Horror benchmark did not include the full scene and three new dark views'}
if(Select-String -LiteralPath (Join-Path $perfSaved 'performance.log') -Pattern 'Non-Nanite Marking Job Queue overflow|Virtual Shadow Map.*overflow|surface cache atlas oversubscribed|Default Material will be used|Failed to compile Material|GPU Crashed|Out of video memory|is not a valid parent for MaterialInstanceDynamic'){throw 'Runtime rendering failure during performance capture'}
if((Get-FileHash -LiteralPath $perfExe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfExecutableHash){throw 'Executable changed during performance capture'}
$perfData | Add-Member -NotePropertyName executable_sha256 -NotePropertyValue $perfExecutableHash -Force
$perfData | Add-Member -NotePropertyName started_at -NotePropertyValue $perfStart.ToString('o') -Force
$perfData | Add-Member -NotePropertyName finished_at -NotePropertyValue (Get-Date).ToString('o') -Force
if($perfDetailHash){
    if((Get-FileHash -LiteralPath $perfDetailPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfDetailHash){throw 'Detail placement changed during capture'}
    $perfData | Add-Member -NotePropertyName detail_data_sha256 -NotePropertyValue $perfDetailHash -Force
}
if($perfHorror28Hash){
    if((Get-FileHash -LiteralPath $perfHorror28Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfHorror28Hash){throw 'v27 details changed during capture'}
    $perfData | Add-Member -NotePropertyName horror28_data_sha256 -NotePropertyValue $perfHorror28Hash -Force
}
if($perfRealismHash){
 if((Get-FileHash -LiteralPath $perfRealismPath).Hash.ToLowerInvariant() -ne $perfRealismHash){throw 'Realism data changed during performance run'}
 $perfData | Add-Member -NotePropertyName realism_data_sha256 -NotePropertyValue $perfRealismHash -Force
}
if($perfCars31Hash){if((Get-FileHash -LiteralPath $perfCars31Path).Hash.ToLowerInvariant() -ne $perfCars31Hash){throw 'Cars31 data changed during capture'};$perfData | Add-Member -NotePropertyName cars31_data_sha256 -NotePropertyValue $perfCars31Hash -Force}
if($perfQuality32Hash){if((Get-FileHash -LiteralPath $perfQuality32Path).Hash.ToLowerInvariant() -ne $perfQuality32Hash){throw 'Texture quality changed during capture'};$perfData | Add-Member -NotePropertyName car_quality32_data_sha256 -NotePropertyValue $perfQuality32Hash -Force}
$perfData | Add-Member -NotePropertyName packaged_content_sha256 -NotePropertyValue $perfArchiveHashes -Force
$perfData | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $perfResult -Encoding utf8
$perfData | ConvertTo-Json -Depth 8
