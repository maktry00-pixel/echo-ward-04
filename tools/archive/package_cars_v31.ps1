$ErrorActionPreference='Stop'
$taskRoot='D:/Projects/ECHO/unreal/EchoWard'
$taskEditor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
$taskImported=Get-Content -Raw -LiteralPath ($taskRoot+'/Saved/Prototype/cars31_batch_0_0.json') | ConvertFrom-Json
$taskManifest=Get-Content -Raw -LiteralPath ($taskRoot+'/RawAssets/EchoCarsV31/manifest.json') | ConvertFrom-Json
if(!$taskImported.success -or $taskImported.imported.Count -ne $taskManifest.assets.Count){Get-Content ($taskRoot+'/Saved/Prototype/cars31_failed.txt');throw 'Incomplete vehicle import'}
Write-Output 'PHASE native-audit'
$taskStarted=Get-Date
$taskArgs=@(($taskRoot+'/EchoWard.uproject'),('-ExecutePythonScript='+$taskRoot+'/Content/Python/audit_echo_cars_v31.py'),'-unattended','-nosplash','-nullrhi','-nosound','-NoLiveCoding','-abslog=D:/Projects/ECHO/tmp/echo_v31_audit.log')
$taskProcess=Start-Process -FilePath $taskEditor -ArgumentList $taskArgs -WindowStyle Hidden -Wait -PassThru
$taskAudit=$taskRoot+'/Saved/Prototype/cars31_native_audit.json'
if(!(Test-Path $taskAudit) -or (Get-Item $taskAudit).LastWriteTime -lt $taskStarted){Get-Content ($taskRoot+'/Saved/Prototype/cars31_audit_failed.txt');throw 'No fresh native audit'}
$taskNative=Get-Content -Raw $taskAudit | ConvertFrom-Json
if(!$taskNative.success -or $taskNative.meshes -ne $taskManifest.assets.Count){throw 'Native audit failed'}
Write-Output 'PHASE editor-build'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' EchoWardEditor Win64 Development ('-Project='+$taskRoot+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> D:/Projects/ECHO/tmp/echo_v31_build_editor.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/echo_v31_build_editor.log -Tail 20;throw 'Editor build failed'}
Write-Output 'PHASE game-build'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' EchoWard Win64 Development ('-Project='+$taskRoot+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> D:/Projects/ECHO/tmp/echo_v31_build_final_game.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/echo_v31_build_final_game.log -Tail 20;throw 'Game build failed'}
Write-Output 'PHASE cook-package'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat' BuildCookRun ('-project='+$taskRoot+'/EchoWard.uproject') -noP4 -platform=Win64 -clientconfig=Development -nobuild -cook -stage -pak -archive ('-archivedirectory='+$taskRoot+'/Saved/PackagedWindows') -utf8output *> D:/Projects/ECHO/tmp/echo_v31_package.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/echo_v31_package.log -Tail 25;throw 'Package failed'}
Write-Output 'PHASE cars-render'
& ($taskRoot+'/Tools/verify_hospital_cars_v31.ps1')
Write-Output 'PHASE arrival-render'
& ($taskRoot+'/Tools/verify_hospital_arrival_v29.ps1') -Pictures
Write-Output 'V31_PIPELINE_PASS'
