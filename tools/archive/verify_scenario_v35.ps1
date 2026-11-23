param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$scenarioRoot='D:/Projects/ECHO/unreal/EchoWard'
if(!$SkipBuild){
Write-Output 'PHASE editor-build'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' EchoWardEditor Win64 Development ('-Project='+$scenarioRoot+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> D:/Projects/ECHO/tmp/scenario35_build_editor.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/scenario35_build_editor.log -Tail 20;throw 'Editor build failed'}
Write-Output 'PHASE game-build'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' EchoWard Win64 Development ('-Project='+$scenarioRoot+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> D:/Projects/ECHO/tmp/scenario35_build_game.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/scenario35_build_game.log -Tail 20;throw 'Game build failed'}
}
foreach($scenarioDest in @((Join-Path $scenarioRoot 'Saved/PackagedWindows/Windows/EchoWard'),(Join-Path $scenarioRoot 'Saved/StagedBuilds/Windows/EchoWard'))){
 Copy-Item -LiteralPath ($scenarioRoot+'/Binaries/Win64/EchoWard.exe') -Destination ($scenarioDest+'/Binaries/Win64/EchoWard.exe') -Force
 Copy-Item -LiteralPath ($scenarioRoot+'/Content/CampaignData/echo_scenario_v35.json') -Destination ($scenarioDest+'/Content/CampaignData/echo_scenario_v35.json') -Force
}
Write-Output 'PHASE scenario-runtime-and-render'
$scenarioArgs=@('-UserDir=D:/Projects/ECHO/unreal/EchoWard/Saved/PackageTests/Scenario35Release','-WardScenarioCheck','-WardScenarioPictures','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-unattended','-nosound','-NoLiveCoding','-abslog=D:/Projects/ECHO/tmp/scenario35_release.log')
$scenarioStarted=Get-Date
$scenarioProc=Start-Process -FilePath ($scenarioRoot+'/Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe') -ArgumentList $scenarioArgs -WindowStyle Hidden -Wait -PassThru
if($scenarioProc.ExitCode -ne 0){throw 'Scenario executable failed'}
$scenarioResult=$scenarioRoot+'/Saved/PackageTests/Scenario35Release/Saved/Prototype/scenario35_check.json'
if(!(Test-Path -LiteralPath $scenarioResult) -or (Get-Item -LiteralPath $scenarioResult).LastWriteTime -lt $scenarioStarted){throw 'No fresh scenario evidence'}
$scenarioReport=Get-Content -Raw -LiteralPath $scenarioResult | ConvertFrom-Json
if(!$scenarioReport.success){$scenarioReport.checks | Where-Object {!$_.passed} | ConvertTo-Json;throw 'Scenario verification failed'}
Write-Output ('SCENARIO PASS '+$scenarioReport.count)
Write-Output 'PHASE world-regression'
& ($scenarioRoot+'/Tools/verify_windows_package.ps1') -Mode World
Write-Output 'PHASE menu-regression'
& ($scenarioRoot+'/Tools/verify_windows_package.ps1') -Mode Menu
Write-Output 'PHASE arrival-regression'
& ($scenarioRoot+'/Tools/verify_hospital_arrival_v29.ps1')
Write-Output 'SCENARIO35_PIPELINE_PASS'
