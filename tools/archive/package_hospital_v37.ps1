$ErrorActionPreference='Stop'
$hospital37Root='D:/Projects/ECHO/unreal/EchoWard'
foreach($hospital37Target in @('EchoWardEditor','EchoWard')){
 $hospital37Log=if($hospital37Target -eq 'EchoWardEditor'){'D:/Projects/ECHO/tmp/hospital37_build_editor_final.log'}else{'D:/Projects/ECHO/tmp/hospital37_build_game.log'}
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' $hospital37Target Win64 Development ('-Project='+$hospital37Root+'/EchoWard.uproject') -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2 *> $hospital37Log
 if($LASTEXITCODE -ne 0){Get-Content $hospital37Log -Tail 18;throw ('Build failed: '+$hospital37Target)}
}
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat' BuildCookRun ('-project='+$hospital37Root+'/EchoWard.uproject') -noP4 -platform=Win64 -clientconfig=Development -nobuild -cook -stage -pak -archive ('-archivedirectory='+$hospital37Root+'/Saved/PackagedWindows') -utf8output *> D:/Projects/ECHO/tmp/hospital37_package_pilot.log
if($LASTEXITCODE -ne 0){Get-Content D:/Projects/ECHO/tmp/hospital37_package_pilot.log -Tail 20;throw 'Package failed'}
Write-Output 'HOSPITAL37_PACKAGE_PASS'
