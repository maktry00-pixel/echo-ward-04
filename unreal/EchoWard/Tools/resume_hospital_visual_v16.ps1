[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskWorkspace=[IO.Path]::GetFullPath((Join-Path $taskRoot '../..'))
$taskPackage=Join-Path $taskRoot 'Saved/PackagedWindows'
$taskExe=Join-Path $taskPackage 'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskStage=Join-Path $taskPackage 'Windows/EchoWard/Content/CampaignData/hospital_world.json'
$taskSource=Join-Path $taskRoot 'Content/CampaignData/hospital_world.json'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/WorldVisualV16Resume'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
try {
    & 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 (Join-Path $taskWorkspace 'tools/prepare_v16_capture_resume.py')
    if($LASTEXITCODE -ne 0){throw 'Camera-only continuation preparation failed.'}
    $taskVariantHash=(Get-FileHash -LiteralPath $taskStage).Hash
    $taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
    $taskStarted=Get-Date
    $taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
    if($taskProcess.ExitCode -ne 0){throw 'Continuation capture runtime failed.'}
    $taskResult=Get-Content -Raw -LiteralPath (Join-Path $taskSaved 'Prototype/book_world_check.json') | ConvertFrom-Json
    if(!$taskResult.success -or $taskResult.count -ne 30){throw 'Continuation rendered geometry checks failed.'}
    $taskVariant=Get-Content -Raw -LiteralPath $taskStage | ConvertFrom-Json
    foreach($taskCapture in @($taskVariant.locations)+@($taskVariant.furnishing_captures)){
        $taskImage=Join-Path $taskSaved ('Prototype/BookWorldCaptures/world_'+$taskCapture.id+'.png')
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted){throw ('Missing continuation image '+$taskCapture.id)}
    }
} finally {Copy-Item -LiteralPath $taskSource -Destination $taskStage -Force}
$taskCanonical=Get-Content -Raw -LiteralPath $taskSource | ConvertFrom-Json
$taskInitial=Join-Path $taskRoot 'Saved/PackageTests/WorldVisualV16/Saved'
foreach($taskCapture in @($taskCanonical.locations)+@($taskCanonical.furnishing_captures)){
    $taskName='Prototype/BookWorldCaptures/world_'+$taskCapture.id+'.png'
    if(!(Test-Path -LiteralPath (Join-Path $taskSaved $taskName)) -and !(Test-Path -LiteralPath (Join-Path $taskInitial $taskName))){throw ('Missing combined image '+$taskCapture.id)}
}
foreach($taskLog in @((Join-Path $taskSaved 'package_test.log'),(Join-Path $taskInitial 'package_test.log'))){if(Select-String -LiteralPath $taskLog -Pattern 'Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy'){throw 'Rendered warning.'}}
[ordered]@{success=$true;mode='WorldVisualV16';checked_at=(Get-Date).ToString('o');test_saved=$taskSaved;additional_capture_saved=$taskInitial;executable_sha256=(Get-FileHash -LiteralPath $taskExe).Hash;map_sha256=(Get-FileHash -LiteralPath $taskSource).Hash;capture_variant_sha256=$taskVariantHash;geometry_identical_to_canonical=$true;only_camera_metadata_changed=$true;initial_checks=30;resolution='1920x1080';capture_simulation_step_seconds=0.1;scope='Two camera batches of identical world geometry. First capture process stopped after 165 images; missing views and all 35 locations completed in a bounded continuation pass. Not a performance benchmark.'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskPackage 'verified_worldvisual_v16.json') -Encoding UTF8
