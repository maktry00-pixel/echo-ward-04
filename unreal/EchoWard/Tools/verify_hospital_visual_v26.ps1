[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$v26Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$v26Exe=Join-Path $v26Root 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$v26User=Join-Path $v26Root 'Saved/PackageTests/WorldVisualV26'
$v26Saved=Join-Path $v26User 'Saved'
New-Item -ItemType Directory -Force -Path $v26Saved | Out-Null
$v26Args=@(('-UserDir='+$v26User),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $v26Saved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-WardCaptureSupplements','-WardCaptureQuality','-WardCaptureReal','-WardCaptureDetail','-WardCapturePrefix=v19','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$v26Started=Get-Date
$v26Process=Start-Process -FilePath $v26Exe -ArgumentList (($v26Args | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $v26Saved 'console.log') -RedirectStandardError (Join-Path $v26Saved 'stderr.log')
if($v26Process.ExitCode -ne 0){throw 'Native v26 visual capture failed'}
if((Get-Item -LiteralPath (Join-Path $v26Saved 'Prototype/book_world_check.json')).LastWriteTime -lt $v26Started){throw 'No fresh v26 capture result'}
& 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 (Join-Path $v26Root '../../tools/verify_hospital_visual_v26.py')
if($LASTEXITCODE -ne 0){throw 'v26 visual validation failed'}
