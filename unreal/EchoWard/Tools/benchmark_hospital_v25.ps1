[CmdletBinding()]
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9_]+$')][string]$Label)
$ErrorActionPreference='Stop'
$perfProject=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$perfExe=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$perfUser=Join-Path $perfProject ('Saved/PackageTests/PerformanceV25_'+$Label)
$perfSaved=Join-Path $perfUser 'Saved'
New-Item -ItemType Directory -Path $perfSaved -Force | Out-Null
$perfArgs=@(('-UserDir='+$perfUser),'-WardHospitalBenchmark',('-WardPerfLabel='+$Label),'-unattended','-nosound','-NoLiveCoding','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $perfSaved 'performance.log')))
$perfStart=Get-Date
$perfExecutableHash=(Get-FileHash -LiteralPath $perfExe -Algorithm SHA256).Hash.ToLowerInvariant()
$perfDetailPath=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_detail_v26.json'
$perfDetailHash=if(Test-Path -LiteralPath $perfDetailPath){(Get-FileHash -LiteralPath $perfDetailPath -Algorithm SHA256).Hash.ToLowerInvariant()}else{''}
$perfDetail27Path=Join-Path $perfProject 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData/echo_detail_v27.json'
$perfDetail27Hash=if(Test-Path -LiteralPath $perfDetail27Path){(Get-FileHash -LiteralPath $perfDetail27Path -Algorithm SHA256).Hash.ToLowerInvariant()}else{''}
$perfProcess=Start-Process -FilePath $perfExe -ArgumentList (($perfArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $perfSaved 'console.log') -RedirectStandardError (Join-Path $perfSaved 'stderr.log')
if($perfProcess.ExitCode -ne 0){throw 'Performance capture failed'}
$perfResult=Join-Path $perfSaved ('Prototype/hospital_perf_'+$Label+'.json')
if(!(Test-Path -LiteralPath $perfResult) -or (Get-Item -LiteralPath $perfResult).LastWriteTime -lt $perfStart){throw 'No fresh performance result'}
$perfData=Get-Content -LiteralPath $perfResult -Raw | ConvertFrom-Json
if(!$perfData.success -or $perfData.real_instances -ne 16107 -or $perfData.quality_instances -ne 28850 -or $perfData.screen_percentage -ne 100){throw 'Performance run changed the scene or resolution'}
if(Select-String -LiteralPath (Join-Path $perfSaved 'performance.log') -Pattern 'Default Material will be used|Failed to compile Material|GPU Crashed|Out of video memory|is not a valid parent for MaterialInstanceDynamic'){throw 'Runtime rendering failure during performance capture'}
if((Get-FileHash -LiteralPath $perfExe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfExecutableHash){throw 'Executable changed during performance capture'}
$perfData | Add-Member -NotePropertyName executable_sha256 -NotePropertyValue $perfExecutableHash -Force
$perfData | Add-Member -NotePropertyName started_at -NotePropertyValue $perfStart.ToString('o') -Force
$perfData | Add-Member -NotePropertyName finished_at -NotePropertyValue (Get-Date).ToString('o') -Force
if($perfDetailHash){
    if((Get-FileHash -LiteralPath $perfDetailPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfDetailHash){throw 'Detail placement changed during capture'}
    $perfData | Add-Member -NotePropertyName detail_data_sha256 -NotePropertyValue $perfDetailHash -Force
}
if($perfDetail27Hash){
    if((Get-FileHash -LiteralPath $perfDetail27Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $perfDetail27Hash){throw 'v27 details changed during capture'}
    $perfData | Add-Member -NotePropertyName detail27_data_sha256 -NotePropertyValue $perfDetail27Hash -Force
}
$perfData | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $perfResult -Encoding utf8
$perfData | ConvertTo-Json -Depth 8
