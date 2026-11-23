[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$v5Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$v5Package=Join-Path $v5Root 'Saved/PackagedWindows'
$v5Exe=Join-Path $v5Package 'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$v5User=Join-Path $v5Root 'Saved/PackageTests/WorldVisualV5'
$v5Saved=Join-Path $v5User 'Saved'
New-Item -ItemType Directory -Force -Path $v5Saved | Out-Null
$v5Args=@(('-UserDir='+$v5User),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $v5Saved 'package_test.log')),
    '-WardBookWorldCheck','-WardBookWorldPictures','-WardHospitalPreview','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')
$v5Started=Get-Date
$v5Process=Start-Process -FilePath $v5Exe -ArgumentList (($v5Args | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $v5Saved 'console.log') -RedirectStandardError (Join-Path $v5Saved 'stderr.log')
if($v5Process.ExitCode -ne 0){throw 'Visual runtime failed.'}
$v5ResultPath=Join-Path $v5Saved 'Prototype/book_world_check.json'
if(!(Test-Path -LiteralPath $v5ResultPath) -or (Get-Item -LiteralPath $v5ResultPath).LastWriteTime -lt $v5Started){throw 'No fresh visual result.'}
$v5Result=Get-Content -Raw -LiteralPath $v5ResultPath | ConvertFrom-Json
$v5Data=Get-Content -Raw -LiteralPath (Join-Path $v5Package 'Windows/EchoWard/Content/CampaignData/hospital_world.json') | ConvertFrom-Json
if(!$v5Result.success -or $v5Result.count -lt 10 -or $v5Result.rooms -ne 562 -or $v5Result.locations -ne 35){throw 'Initial rendered geometry checks failed.'}
foreach($v5Site in $v5Data.locations){
    $v5Image=Join-Path $v5Saved ('Prototype/BookWorldCaptures/world_'+$v5Site.id+'.png')
    if(!(Test-Path -LiteralPath $v5Image) -or (Get-Item -LiteralPath $v5Image).LastWriteTime -lt $v5Started){throw ('Missing fresh image '+$v5Site.id)}
}
if(Select-String -LiteralPath (Join-Path $v5Saved 'package_test.log') -Pattern 'Default Material will be used|Failed to compile Material|pending kill, unreachable or pending destroy|Virtual Shadow Map.*overflow|surface cache atlas oversubscribed'){throw 'Rendered material/runtime warning.'}
[ordered]@{success=$true;mode='WorldVisualV5';checked_at=(Get-Date).ToString('o');test_saved=$v5Saved;
    executable_sha256=(Get-FileHash -LiteralPath $v5Exe).Hash;
    map_sha256=(Get-FileHash -LiteralPath (Join-Path $v5Package 'Windows/EchoWard/Content/CampaignData/hospital_world.json')).Hash;
    initial_checks=[int]$v5Result.count;locations=35;resolution='1920x1080';
    scope='Rendered initial geometry checks and all 35 locations; complete traversal is checked separately by World.'} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $v5Package 'verified_worldvisual_v5.json') -Encoding UTF8
