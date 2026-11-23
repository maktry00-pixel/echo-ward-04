[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$v27Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$v27Exe=Join-Path $v27Root 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$v27User=Join-Path $v27Root 'Saved/PackageTests/WorldVisualV27'
$v27Saved=Join-Path $v27User 'Saved'
New-Item -ItemType Directory -Force -Path $v27Saved | Out-Null
$v27Args=@(('-UserDir='+$v27User),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $v27Saved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-WardCaptureSupplements','-WardCaptureQuality','-WardCaptureReal','-WardCaptureDetail','-WardCaptureDetail27','-WardCapturePrefix=v19','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$v27Started=Get-Date
$v27Process=Start-Process -FilePath $v27Exe -ArgumentList (($v27Args | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $v27Saved 'console.log') -RedirectStandardError (Join-Path $v27Saved 'stderr.log')
if($v27Process.ExitCode -ne 0){throw 'Native v27 visual capture failed'}
if((Get-Item -LiteralPath (Join-Path $v27Saved 'Prototype/book_world_check.json')).LastWriteTime -lt $v27Started){throw 'No fresh v27 capture result'}
& 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 (Join-Path $v27Root '../../tools/verify_hospital_visual_v27.py')
if($LASTEXITCODE -ne 0){throw 'v27 visual validation failed'}
