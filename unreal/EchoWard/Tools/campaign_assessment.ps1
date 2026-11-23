[CmdletBinding()]
param(
    [ValidateSet('Smoke','Physics','Agent','Capture','Pose','Review','Balanced','Performance','Quality')][string]$Mode='Smoke',
    [ValidateRange(640,3840)][int]$Width=1920,
    [ValidateRange(480,2160)][int]$Height=1080,
    [switch]$Resume,
    [switch]$RequireAC
)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
& (Join-Path $PSScriptRoot 'bootstrap.ps1') -Mode Doctor | Out-Null
$taskDoctor=Get-Content -Raw (Join-Path $taskRoot 'Saved\Prototype\doctor.json') | ConvertFrom-Json
if(!$taskDoctor.engine_ready -or !$taskDoctor.native_module_present -or !(Test-Path -LiteralPath (Join-Path $taskRoot 'Content\Campaign\Maps\Campaign.umap'))){throw 'Build and import the native campaign first.'}
$taskEditor=Join-Path $taskDoctor.engine_root 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$taskReport=Join-Path $taskRoot 'Saved\Prototype'
$taskLabel='campaign_'+$Mode.ToLowerInvariant()
$taskArgs=@((Join-Path $taskRoot 'EchoWard.uproject'),'/Game/Campaign/Maps/Campaign?game=/Script/EchoWard.EWardCampaignMode','-game','-unattended','-nosound','-NoLiveCoding','-nosplash','-stdout','-FullStdOutLogOutput',('-abslog='+(Join-Path $taskReport ($taskLabel+'.log'))))
$taskArgs+='-WardLegacyCampaign'
switch($Mode){
    'Smoke' {$taskArgs+=@('-nullrhi','-WardCampaignSmoke');$taskResult='campaign_smoke.json'}
    'Physics' {$taskArgs+=@('-nullrhi','-WardPhysicsCheck','-benchmark','-deterministic','-fps=60');$taskResult='physics_assessment.json'}
    'Agent' {$taskArgs+=@('-nullrhi','-WardCampaignAgent','-benchmark','-deterministic','-fps=60');if($Resume){$taskArgs+='-WardAgentResume'};$taskResult='campaign_agent.json'}
    'Capture' {$taskArgs+='-WardCampaignCapture';$taskResult=$null}
    'Pose' {$taskArgs+=@('-WardCampaignCapture','-WardPoseCapture');$taskResult=$null}
    'Review' {$taskArgs+=@('-WardCampaignCapture','-WardVisualReview');$taskResult=$null}
    default {$taskPreset=switch($Mode){'Quality'{0}'Balanced'{1}'Performance'{2}};$taskArgs+=@('-WardCampaignBenchmark',('-WardGraphics='+$taskPreset));$taskResult='full_campaign_'+$taskPreset+'_benchmark.json'}
}
if($Mode -notin @('Smoke','Physics','Agent')){$taskArgs+=@('-RenderOffscreen','-windowed','-ForceRes',('-ResX='+$Width),('-ResY='+$Height))}
. (Join-Path $PSScriptRoot 'power_status.ps1')
$taskPowerBefore=Get-WardPowerStatus
if($RequireAC -and (!$taskPowerBefore.success -or $taskPowerBefore.ac_line_status -ne 1)){throw 'AC power is required for this measurement.'}
$taskStarted=Get-Date
& $taskEditor @taskArgs *> (Join-Path $taskReport ($taskLabel+'_console.log'))
if($LASTEXITCODE -ne 0){throw "Unreal exited with code $LASTEXITCODE. See $taskLabel.log."}
if(!$taskResult){Write-Host ('Screenshots: '+(Join-Path $taskReport 'CampaignCaptures'));exit 0}
$taskResultPath=Join-Path $taskReport $taskResult
if(!(Test-Path -LiteralPath $taskResultPath) -or (Get-Item -LiteralPath $taskResultPath).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'No fresh assessment result was produced.'}
$taskData=Get-Content -Raw -LiteralPath $taskResultPath | ConvertFrom-Json
$taskPowerAfter=Get-WardPowerStatus
$taskData | Add-Member -NotePropertyName started_at -NotePropertyValue $taskStarted.ToString('o') -Force
$taskData | Add-Member -NotePropertyName ac_power_before -NotePropertyValue $taskPowerBefore -Force
$taskData | Add-Member -NotePropertyName ac_power_after -NotePropertyValue $taskPowerAfter -Force
$taskData | Add-Member -NotePropertyName resolution -NotePropertyValue ($Width.ToString()+'x'+$Height) -Force
$taskData | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $taskResultPath -Encoding UTF8
if($RequireAC -and (!$taskPowerAfter.success -or $taskPowerAfter.ac_line_status -ne 1)){throw 'AC power changed during measurement; do not use this as an AC result.'}
if(!$taskData.success){throw "Campaign $Mode failed. See $taskResultPath."}
$taskData | ConvertTo-Json -Depth 8
