[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$v28Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$v28Exe=Join-Path $v28Root 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$v28User=Join-Path $v28Root 'Saved/PackageTests/WorldVisualV28'
$v28Saved=Join-Path $v28User 'Saved'
New-Item -ItemType Directory -Force -Path $v28Saved | Out-Null
$v28Args=@(('-UserDir='+$v28User),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $v28Saved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-WardCaptureSupplements','-WardCaptureQuality','-WardCaptureReal','-WardCaptureDetail','-WardCaptureDetail27','-WardCaptureHorror28','-WardCapturePrefix=v19','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$v28Started=Get-Date
$v28Process=Start-Process -FilePath $v28Exe -ArgumentList (($v28Args | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $v28Saved 'console.log') -RedirectStandardError (Join-Path $v28Saved 'stderr.log')
if($v28Process.ExitCode -ne 0){throw 'Native v28 visual capture failed'}
if((Get-Item -LiteralPath (Join-Path $v28Saved 'Prototype/book_world_check.json')).LastWriteTime -lt $v28Started){throw 'No fresh v28 capture result'}
& 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 (Join-Path $v28Root '../../tools/verify_hospital_visual_v28.py')
if($LASTEXITCODE -ne 0){throw 'v28 visual validation failed'}
