$ErrorActionPreference='Stop'
& tools/verify_hospital_v37_all.ps1 -BuildOnly
$hospital37Root='D:/Projects/ECHO/unreal/EchoWard'
foreach($hospital37Dest in @((Join-Path $hospital37Root 'Saved/PackagedWindows/Windows/EchoWard/Content/CampaignData'),(Join-Path $hospital37Root 'Saved/StagedBuilds/Windows/EchoWard/Content/CampaignData'))){Copy-Item -LiteralPath ($hospital37Root+'/Content/CampaignData/THIRD_PARTY_ART.md') -Destination ($hospital37Dest+'/THIRD_PARTY_ART.md') -Force}
& 'C:/Users/YOUR_WINDOWS_USER/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 tools/audit_hospital_v37.py
$hospital37Freeze=@{executable_sha256=(Get-FileHash -LiteralPath ($hospital37Root+'/Binaries/Win64/EchoWard.exe') -Algorithm SHA256).Hash.ToLowerInvariant();data_sha256=@{}}
foreach($hospital37Fn in @('hospital_world.json','echo_scenario_v35.json','hospital_environment_v37.json','hospital_assets_v37.json')){$hospital37Freeze.data_sha256[$hospital37Fn]=(Get-FileHash -LiteralPath ($hospital37Root+'/Content/CampaignData/'+$hospital37Fn) -Algorithm SHA256).Hash.ToLowerInvariant()}
$hospital37Freeze | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath 'D:/Projects/ECHO/artifacts/echo_hospital_v37/freeze_identity.json' -Encoding utf8
Write-Output 'PHASE world-first'
& unreal/EchoWard/Tools/verify_windows_package.ps1 -Mode World
Write-Output 'PHASE full-facilities-render'
& tools/verify_hospital_v37.ps1 -Scope all
Write-Output 'PHASE scenario'
$hospital37Exe=$hospital37Root+'/Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$hospital37Start=Get-Date
$hospital37Args=@('-UserDir=D:/Projects/ECHO/unreal/EchoWard/Saved/PackageTests/Scenario35Release','-WardScenarioCheck','-WardScenarioPictures','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-unattended','-nosound','-NoLiveCoding','-abslog=D:/Projects/ECHO/tmp/scenario35_release.log')
$hospital37Proc=Start-Process -FilePath $hospital37Exe -ArgumentList $hospital37Args -WindowStyle Hidden -Wait -PassThru
if($hospital37Proc.ExitCode -ne 0){throw 'Scenario failed'}
$hospital37Result=$hospital37Root+'/Saved/PackageTests/Scenario35Release/Saved/Prototype/scenario35_check.json'
if((Get-Item -LiteralPath $hospital37Result).LastWriteTime -lt $hospital37Start){throw 'No fresh scenario result'}
$hospital37Scenario=Get-Content -Raw -LiteralPath $hospital37Result | ConvertFrom-Json
if(!$hospital37Scenario.success){throw 'Scenario check failed'}
Write-Output ('SCENARIO_PASS '+$hospital37Scenario.count)
Write-Output 'PHASE menu'
& unreal/EchoWard/Tools/verify_windows_package.ps1 -Mode Menu
Write-Output 'PHASE arrival'
& unreal/EchoWard/Tools/verify_hospital_arrival_v29.ps1
Write-Output 'PHASE performance'
& unreal/EchoWard/Tools/benchmark_hospital_v28.ps1 -Label hospital37_after
Write-Output 'HOSPITAL37_FINAL_VERIFICATION_PASS'
