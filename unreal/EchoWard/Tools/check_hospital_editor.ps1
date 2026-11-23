param([switch]$Preview)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskUser=Join-Path $taskRoot ('Saved/HospitalEditor'+$(if($Preview){'Preview'}else{'Test'}))
New-Item -ItemType Directory -Force -Path $taskUser | Out-Null
$taskArgs=@((Join-Path $taskRoot 'EchoWard.uproject'),'/Game/Campaign/Maps/Campaign','-game','-unattended','-nosound','-NoLiveCoding','-WardBookWorldCheck',('-UserDir='+$taskUser),('-abslog='+$taskUser+'/editor_world.log'))
if($Preview){$taskArgs+=@('-WardBookWorldPictures','-WardHospitalPreview','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
else{$taskArgs+=@('-nullrhi','-benchmark','-deterministic','-fps=60')}
$taskQuoted=($taskArgs|ForEach-Object {'"'+$_+'"'}) -join ' '
$taskProcess=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru
if($taskProcess.ExitCode -ne 0){throw ('Editor test failed '+$taskProcess.ExitCode)}
if(Select-String -LiteralPath (Join-Path $taskUser 'editor_world.log') -Pattern 'Default Material will be used|Failed to compile Material|Non-Nanite Marking Job Queue overflow|surface cache atlas oversubscribed'){throw 'Hospital rendering regression.'}
$taskResult=Get-Content -Raw -LiteralPath (Join-Path $taskUser 'Saved/Prototype/book_world_check.json')|ConvertFrom-Json
$taskResult.checks | Where-Object {!$_.passed} | Format-Table
if(!$taskResult.success){throw 'Hospital world checks failed.'}
Write-Output ('Hospital checks passed: '+$taskResult.count)
