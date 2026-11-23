[CmdletBinding()]
param([ValidateSet('Smoke','High','Epic','Balanced','Performance')][string]$Mode='High',
      [ValidateRange(640,3840)][int]$Width=1600,
      [ValidateRange(480,2160)][int]$Height=900,
      [ValidatePattern('^[a-z0-9_]+$')][string]$Label='',
      [switch]$RequireAC)
$ErrorActionPreference='Stop'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
& (Join-Path $PSScriptRoot 'bootstrap.ps1') -Mode Doctor | Out-Null
$taskDoctor=Get-Content -Raw (Join-Path $taskRoot 'Saved\Prototype\doctor.json') | ConvertFrom-Json
if(!$taskDoctor.engine_ready -or !$taskDoctor.native_module_present -or !$taskDoctor.generated_map_present){throw 'Run Setup before assessment.'}
$taskEditor=Join-Path $taskDoctor.engine_root 'Engine\Binaries\Win64\UnrealEditor.exe'
$taskProfile=if($Label){$Label}else{$Mode.ToLowerInvariant()}
$taskArguments=@(('"'+(Join-Path $taskRoot 'EchoWard.uproject')+'"'),'/Game/Prototype/Maps/Ward04','-game','-windowed',('-ResX='+$Width),('-ResY='+$Height),'-ForceRes','-NoLiveCoding','-nosplash')
if($Mode -eq 'Smoke'){$taskArguments+='-WardSmoke';$taskResult='smoke_result.json'}
else{
    $taskQuality=if($Mode -eq 'Epic'){3}else{2}
    $taskCommands=@('t.MaxFPS 0','r.VSync 0','t.IdleWhenNotForeground 0','sg.ResolutionQuality 100','r.ScreenPercentage 100')
    foreach($taskGroup in @('ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage','Shading')){$taskCommands+='sg.'+$taskGroup+'Quality '+$taskQuality}
    if($Mode -eq 'Performance'){$taskCommands+='r.ScreenPercentage 75'}
    $taskArguments+=@('-WardBenchmark',('-WardProfile='+$taskProfile),('-ExecCmds="'+($taskCommands -join ',')+'"'))
    $taskResult='benchmark_'+$taskProfile+'.json'
}
$taskArguments+=('-abslog="'+(Join-Path $taskRoot ('Saved\Prototype\assessment_'+$taskProfile+'.log'))+'"')
$taskStarted=Get-Date
. (Join-Path $PSScriptRoot 'power_status.ps1')
$taskACBefore=Get-WardPowerStatus
if($RequireAC -and (!$taskACBefore.success -or $taskACBefore.ac_line_status -ne 1)){throw 'Windows does not report AC power online; AC benchmark cancelled.'}
$taskPowerBefore=@(Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue | Select-Object BatteryStatus,EstimatedChargeRemaining)
$taskTelemetry=$null
$taskSmi=Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue
try {
    if($taskSmi){
        $taskTelemetry=Start-Process -FilePath $taskSmi.Source -ArgumentList @('--query-gpu=timestamp,pstate,temperature.gpu,utilization.gpu,clocks.gr,power.draw','--format=csv','--loop-ms=1000') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskRoot ('Saved\Prototype\gpu_'+$taskProfile+'.csv'))
    }
    Start-Process -FilePath $taskEditor -ArgumentList $taskArguments -WindowStyle Normal -Wait
} finally {
    if($taskTelemetry -and !$taskTelemetry.HasExited){Stop-Process -InputObject $taskTelemetry}
}
$taskResultPath=Join-Path $taskRoot ('Saved\Prototype\'+$taskResult)
if(!(Test-Path -LiteralPath $taskResultPath) -or (Get-Item -LiteralPath $taskResultPath).LastWriteTime -lt $taskStarted.AddSeconds(-1)){throw 'Assessment did not produce a fresh result.'}
$taskData=Get-Content -Raw -LiteralPath $taskResultPath | ConvertFrom-Json
$taskACAfter=Get-WardPowerStatus
$taskData | Add-Member -NotePropertyName ac_power_before -NotePropertyValue $taskACBefore -Force
$taskData | Add-Member -NotePropertyName ac_power_after -NotePropertyValue $taskACAfter -Force
$taskData | Add-Member -NotePropertyName power_before -NotePropertyValue $taskPowerBefore -Force
$taskData | Add-Member -NotePropertyName power_after -NotePropertyValue @(Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue | Select-Object BatteryStatus,EstimatedChargeRemaining) -Force
$taskData | Add-Member -NotePropertyName started_at -NotePropertyValue $taskStarted.ToString('o') -Force
$taskData | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $taskResultPath -Encoding UTF8
if($RequireAC -and (!$taskACAfter.success -or $taskACAfter.ac_line_status -ne 1)){throw 'AC power changed during benchmark; report saved but not accepted as an AC result.'}
if($Mode -eq 'Smoke' -and (!$taskData.won -or @($taskData.checks | Where-Object {!$_.passed}).Count -gt 0)){throw 'Runtime check failed; see smoke_result.json.'}
$taskData | ConvertTo-Json -Depth 6
