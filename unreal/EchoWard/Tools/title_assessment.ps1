[CmdletBinding()]
param([switch]$Packaged,[int]$Width=1920,[int]$Height=1080)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Packaged){
    & (Join-Path $PSScriptRoot 'verify_windows_package.ps1') -Mode Menu
    exit $LASTEXITCODE
}
& (Join-Path $PSScriptRoot 'bootstrap.ps1') -Mode Doctor | Out-Null
$taskDoctor=Get-Content -Raw (Join-Path $taskRoot 'Saved/Prototype/doctor.json') | ConvertFrom-Json
$taskEditor=Join-Path $taskDoctor.engine_root 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$taskStarted=Get-Date
$taskArgs=@((Join-Path $taskRoot 'EchoWard.uproject'),'/Game/Campaign/Maps/Campaign?game=/Script/EchoWard.EWardCampaignMode','-game','-WardMenuCheck','-unattended','-NoLiveCoding','-nosplash','-RenderOffscreen','-windowed','-ForceRes',('-ResX='+$Width),('-ResY='+$Height),'-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskRoot 'Saved/Prototype/title_check.log')))
$taskArgs+='-WardLegacyCampaign'
& $taskEditor @taskArgs *> (Join-Path $taskRoot 'Saved/Prototype/title_check_console.log')
if($LASTEXITCODE -ne 0){throw 'Unreal menu assessment failed to run.'}
$taskResult=Join-Path $taskRoot 'Saved/Prototype/menu_check.json'
if(!(Test-Path -LiteralPath $taskResult) -or (Get-Item -LiteralPath $taskResult).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh menu assessment report.'}
$taskData=Get-Content -Raw -LiteralPath $taskResult | ConvertFrom-Json
if(!$taskData.success -or $taskData.count -lt 56){throw 'Menu transition checks failed.'}
foreach($taskImage in @('menu_1080','corridor_1080','game_1080','gallery_1080','menu_kk_1080','menu_en_1080','menu_hover_1080','menu_card_hover_1080','door_open_1080','scare_1080','nurse_1080','zombie_head_1080','ball_roll_1080','ball_wait_1080','hand_reach_1080','entrance_idle_1080')){
    $taskImagePath=Join-Path $taskRoot ('Saved/Prototype/MenuCaptures/'+$taskImage+'.png')
    if(!(Test-Path -LiteralPath $taskImagePath) -or (Get-Item -LiteralPath $taskImagePath).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw ('Missing fresh image '+$taskImage)}
}
$taskData | ConvertTo-Json -Depth 5
