[CmdletBinding()]
param([switch]$Pictures)
$ErrorActionPreference='Stop'
$v7Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$v7Package=Join-Path $v7Root 'Saved/PackagedWindows'
$v7Exe=Join-Path $v7Package 'Windows/EchoWard/Binaries/Win64/EchoWard.exe'
$v7User=Join-Path $v7Root $(if($Pictures){'Saved/PackageTests/StoryV7Visual'}else{'Saved/PackageTests/StoryV7'})
$v7Saved=Join-Path $v7User 'Saved'
New-Item -ItemType Directory -Force -Path $v7Saved | Out-Null
$v7Args=@(('-UserDir='+$v7User),'-unattended','-nosound','-NoLiveCoding','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $v7Saved 'package_test.log')),'-WardStoryCheck')
if($Pictures){$v7Args+=@('-WardStoryPictures','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')}
else{$v7Args+=@('-nullrhi','-benchmark','-deterministic','-fps=60')}
$v7Started=Get-Date
$v7Process=Start-Process -FilePath $v7Exe -ArgumentList (($v7Args | ForEach-Object {'"'+$_+'"'}) -join ' ') -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $v7Saved 'console.log') -RedirectStandardError (Join-Path $v7Saved 'stderr.log')
if($v7Process.ExitCode -ne 0){throw 'Story runtime failed.'}
$v7ResultPath=Join-Path $v7Saved 'Prototype/story_check.json'
if(!(Test-Path -LiteralPath $v7ResultPath) -or (Get-Item -LiteralPath $v7ResultPath).LastWriteTime -lt $v7Started){throw 'No fresh story result.'}
$v7Result=Get-Content -Raw -LiteralPath $v7ResultPath | ConvertFrom-Json
if(!$v7Result.success -or $v7Result.count -lt 23){throw 'Story checks failed.'}
if($Pictures){
    $v7Image=Join-Path $v7Saved 'Prototype/story_v7.png'
    if(!(Test-Path -LiteralPath $v7Image) -or (Get-Item -LiteralPath $v7Image).LastWriteTime -lt $v7Started){throw 'No fresh story image.'}
}
[ordered]@{success=$true;mode=$(if($Pictures){'StoryV7Visual'}else{'StoryV7'});checked_at=(Get-Date).ToString('o');test_saved=$v7Saved;
    executable_sha256=(Get-FileHash -LiteralPath $v7Exe).Hash;
    map_sha256=(Get-FileHash -LiteralPath (Join-Path $v7Package 'Windows/EchoWard/Content/CampaignData/hospital_world.json')).Hash;
    checks=[int]$v7Result.count} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $v7Package $(if($Pictures){'verified_storyvisual_v7.json'}else{'verified_story_v7.json'})) -Encoding UTF8
