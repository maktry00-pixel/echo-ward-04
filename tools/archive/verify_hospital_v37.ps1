param([ValidateSet('pilot','all')][string]$Scope='pilot')
$ErrorActionPreference='Stop'
$hospital37Root='D:/Projects/ECHO/unreal/EchoWard'
$hospital37Exe=Join-Path $hospital37Root 'Saved/PackagedWindows/Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$hospital37User=Join-Path $hospital37Root ('Saved/PackageTests/Facilities37_'+$Scope)
$hospital37Saved=Join-Path $hospital37User 'Saved'
New-Item -ItemType Directory -Path $hospital37Saved -Force | Out-Null
$hospital37Args=@(('-UserDir='+$hospital37User),'-WardFacilitiesCheck37','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080','-unattended','-nosound','-NoLiveCoding',('-abslog='+$hospital37Saved+'/runtime.log'))
if($Scope -eq 'all'){$hospital37Args+='-WardFacilitiesAllPictures37'}
$hospital37Start=Get-Date
$hospital37Proc=Start-Process -FilePath $hospital37Exe -ArgumentList $hospital37Args -WindowStyle Hidden -Wait -PassThru
if($hospital37Proc.ExitCode -ne 0){throw 'Facilities executable failed'}
$hospital37Result=$hospital37Saved+'/Prototype/facilities37_check.json'
if(!(Test-Path -LiteralPath $hospital37Result) -or (Get-Item -LiteralPath $hospital37Result).LastWriteTime -lt $hospital37Start){throw 'No fresh facilities result'}
$hospital37Data=Get-Content -Raw -LiteralPath $hospital37Result | ConvertFrom-Json
$hospital37Data | Add-Member -NotePropertyName executable_sha256 -NotePropertyValue (Get-FileHash -LiteralPath $hospital37Exe -Algorithm SHA256).Hash.ToLowerInvariant() -Force
$hospital37Data | Add-Member -NotePropertyName finished_at -NotePropertyValue (Get-Date).ToString('o') -Force
$hospital37Data | Add-Member -NotePropertyName test_saved -NotePropertyValue $hospital37Saved -Force
$hospital37Data | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath ('D:/Projects/ECHO/artifacts/echo_hospital_v37/runtime_'+$Scope+'.json') -Encoding utf8
if(!$hospital37Data.success){$hospital37Data.checks | Where-Object {!$_.passed} | ConvertTo-Json;throw 'Facilities checks failed; inspect audit diagnostics'}
Write-Output ('FACILITIES37_PASS '+$Scope+' '+$hospital37Data.count)
