param([switch]$BuildOnly)
$ErrorActionPreference='Stop'
$hospital37Root='D:/Projects/ECHO/unreal/EchoWard'
foreach($hospital37Target in @('EchoWardEditor','EchoWard')){
 $hospital37Log=if($hospital37Target -eq 'EchoWardEditor'){'D:/Projects/ECHO/tmp/hospital37_editor_accepted.log'}else{'D:/Projects/ECHO/tmp/hospital37_game_accepted.log'}
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' $hospital37Target Win64 Development ('-Project='+$hospital37Root+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> $hospital37Log
 if($LASTEXITCODE -ne 0){Get-Content $hospital37Log -Tail 16;throw 'Build failed'}
}
foreach($hospital37Dest in @((Join-Path $hospital37Root 'Saved/PackagedWindows/Windows/EchoWard'),(Join-Path $hospital37Root 'Saved/StagedBuilds/Windows/EchoWard'))){
 Copy-Item -LiteralPath ($hospital37Root+'/Binaries/Win64/EchoWard.exe') -Destination ($hospital37Dest+'/Binaries/Win64/EchoWard.exe') -Force
 foreach($hospital37File in @('hospital_assets_v37.json','hospital_environment_v37.json')){Copy-Item -LiteralPath ($hospital37Root+'/Content/CampaignData/'+$hospital37File) -Destination ($hospital37Dest+'/Content/CampaignData/'+$hospital37File) -Force}
}
if(!$BuildOnly){& tools/verify_hospital_v37.ps1 -Scope all}
Write-Output 'FACILITIES37_ALL_PIPELINE_PASS'
