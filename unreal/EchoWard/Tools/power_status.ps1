# Direct AC status: WMI BatteryStatus can still report discharging while AC is online.
function Get-WardPowerStatus {
    if(-not ('WardAssessmentPower' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class WardAssessmentPower {
    [StructLayout(LayoutKind.Sequential)] public struct Status {
        public byte ACLineStatus, BatteryFlag, BatteryLifePercent, SystemStatusFlag;
        public uint BatteryLifeTime, BatteryFullLifeTime;
    }
    [DllImport("kernel32.dll")] public static extern bool GetSystemPowerStatus(out Status status);
}
'@
    }
    $taskStatus=New-Object WardAssessmentPower+Status
    $taskSuccess=[WardAssessmentPower]::GetSystemPowerStatus([ref]$taskStatus)
    [pscustomobject]@{
        success=$taskSuccess
        ac_line_status=[int]$taskStatus.ACLineStatus
        battery_percent=[int]$taskStatus.BatteryLifePercent
        battery_flag=[int]$taskStatus.BatteryFlag
        battery_saver=[int]$taskStatus.SystemStatusFlag
    }
}
