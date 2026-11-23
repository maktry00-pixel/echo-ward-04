[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskExe=Join-Path $taskRoot 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$taskUser=Join-Path $taskRoot 'Saved/PackageTests/WorldVisualV22'
$taskSaved=Join-Path $taskUser 'Saved'
New-Item -ItemType Directory -Force -Path $taskSaved | Out-Null
$taskArgs=@(('-UserDir='+$taskUser),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskSaved 'package_test.log')),'-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-WardCaptureSupplements','-WardCaptureQuality','-WardCapturePrefix=v19','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-benchmark','-deterministic','-fps=10')
$taskStart=Get-Date
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList (($taskArgs | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $taskSaved 'console.log') -RedirectStandardError (Join-Path $taskSaved 'stderr.log')
if($taskProcess.ExitCode -ne 0){throw 'Focused capture runtime failed.'}
if((Get-Item -LiteralPath (Join-Path $taskSaved 'Prototype/book_world_check.json')).LastWriteTime -lt $taskStart){throw 'No fresh capture result.'}
& 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 (Join-Path $taskRoot '../../tools/verify_hospital_visual_v22.py')
if($LASTEXITCODE -ne 0){throw 'Focused capture validation failed.'}
