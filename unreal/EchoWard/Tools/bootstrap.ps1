[CmdletBinding()]
param(
    [ValidateSet('Doctor', 'Build', 'Setup', 'Editor', 'Play', 'Package')]
    [string]$Mode = 'Doctor',
    [string]$EngineRoot = '',
    [ValidateSet('Campaign','Prototype')][string]$Scene = 'Campaign'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$ProjectFile = Join-Path $ProjectRoot 'EchoWard.uproject'
$ReportDir = Join-Path $ProjectRoot 'Saved\Prototype'
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

function Find-Engine {
    param([string]$Explicit)
    if ($Explicit) {
        $resolved = [IO.Path]::GetFullPath($Explicit)
        if (!(Test-Path -LiteralPath (Join-Path $resolved 'Engine\Binaries\Win64\UnrealEditor.exe'))) {
            throw "EngineRoot does not contain Engine\Binaries\Win64\UnrealEditor.exe: $resolved"
        }
        return $resolved
    }
    $Candidates = @()
    if ($env:ECHOWARD_UE_ROOT) { $Candidates += $env:ECHOWARD_UE_ROOT }
    $LauncherData = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $LauncherData) {
        $installations = (Get-Content -Raw -LiteralPath $LauncherData | ConvertFrom-Json).InstallationList
        $Candidates += @($installations | Where-Object { $_.AppName -like 'UE_5*' } | ForEach-Object { $_.InstallLocation })
    }
    foreach ($base in @('HKLM:\SOFTWARE\EpicGames\Unreal Engine', 'HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine')) {
        if (Test-Path $base) {
            $Candidates += @(Get-ChildItem $base | ForEach-Object { (Get-ItemProperty $_.PSPath).InstalledDirectory })
        }
    }
    $Candidates += @('D:\Epic Games\UE_5.8', 'D:\Unreal\UE_5.8', 'C:\Program Files\Epic Games\UE_5.8')
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'Engine\Binaries\Win64\UnrealEditor.exe'))) {
            return [IO.Path]::GetFullPath($candidate)
        }
    }
    return $null
}

function Require-Exit {
    param([string]$Stage)
    if ($LASTEXITCODE -ne 0) { throw "$Stage failed, exit code $LASTEXITCODE. See Saved\Logs and Saved\Prototype." }
}

$Engine = Find-Engine $EngineRoot
$EngineManaged = $false
$EngineRegistered = $false
if ($Engine) {
    $EngineManaged = Test-Path -LiteralPath (Join-Path $Engine '.egstore')
    $EngineInstallList = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $EngineInstallList) {
        $Registered = (Get-Content -Raw -LiteralPath $EngineInstallList | ConvertFrom-Json).InstallationList
        $EngineRegistered = @($Registered | Where-Object { $_.InstallLocation -and ([IO.Path]::GetFullPath($_.InstallLocation).TrimEnd('\') -eq $Engine.TrimEnd('\')) }).Count -gt 0
    }
    # Newer Launcher installs can finish in their per-app item manifest without
    # updating the legacy LauncherInstalled.dat index. Require the engine item,
    # its matching path and an explicit completed-install flag, not plugin items.
    if (!$EngineRegistered) {
        $ItemRoot = Join-Path $env:ProgramData 'Epic\EpicGamesLauncher\Data\Manifests'
        if (Test-Path -LiteralPath $ItemRoot) {
            foreach ($ItemFile in Get-ChildItem -LiteralPath $ItemRoot -Filter '*.item' -File) {
                $Item = Get-Content -Raw -LiteralPath $ItemFile.FullName | ConvertFrom-Json
                if ($Item.AppName -like 'UE_5*' -and $Item.bIsIncompleteInstall -eq $false -and $Item.InstallLocation -and ([IO.Path]::GetFullPath($Item.InstallLocation).TrimEnd('\') -eq $Engine.TrimEnd('\'))) {
                    $EngineRegistered = $true
                    break
                }
            }
        }
    }
}
# Epic creates editor binaries while downloading; presence of an EXE is not installation completion.
$EngineReady = [bool]$Engine -and (!$EngineManaged -or $EngineRegistered)
$VSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$CompilerRoots = @()
if (Test-Path -LiteralPath $VSWhere) {
    $CompilerRoots = @(& $VSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
}
$VolumeReport = @(Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Name -match '^[A-Z]$' } | ForEach-Object {
    [ordered]@{ drive = $_.Name; free_gb = [Math]::Round($_.Free / 1GB, 1) }
})
$Native = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-EchoWard.dll'
$MapUrl = if ($Scene -eq 'Campaign') { '/Game/Campaign/Maps/Campaign' } else { '/Game/Prototype/Maps/Ward04' }
$Map = Join-Path $ProjectRoot ($MapUrl.Replace('/Game/','Content/').Replace('/','\')+'.umap')
$Diagnostic = [ordered]@{
    checked_at = (Get-Date).ToString('o')
    project = $ProjectFile
    engine_found = [bool]$Engine
    engine_root = $Engine
    engine_ready = $EngineReady
    epic_installation_registered = $EngineRegistered
    cpp_tools_found = ($CompilerRoots.Count -gt 0)
    cpp_installations = $CompilerRoots
    source_assets_ready = (Test-Path -LiteralPath (Join-Path $ProjectRoot 'RawAssets\manifest.json'))
    native_module_present = (Test-Path -LiteralPath $Native)
    generated_map_present = (Test-Path -LiteralPath $Map)
    scene = $Scene
    campaign_map_present = (Test-Path -LiteralPath (Join-Path $ProjectRoot 'Content\Campaign\Maps\Campaign.umap'))
    volumes = $VolumeReport
    can_start_build = ($EngineReady -and $CompilerRoots.Count -gt 0)
}
$Diagnostic['hardware'] = [ordered]@{
    cpu = @((Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue).Name)
    gpus = @((Get-CimInstance Win32_VideoController -ErrorAction SilentlyContinue).Name)
    ram_gb = [Math]::Round((Get-CimInstance Win32_ComputerSystem -ErrorAction SilentlyContinue).TotalPhysicalMemory / 1GB, 1)
}
$Diagnostic | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $ReportDir 'doctor.json')
if ($Mode -eq 'Doctor') {
    $Diagnostic | ConvertTo-Json -Depth 6
    if (!$Engine) { Write-Host 'Install Unreal Engine 5.8, or pass -EngineRoot. See README.md.' }
    elseif (!$EngineReady) { Write-Host 'Unreal download/install is not complete. Wait for the Launch button in Epic.' }
    if (!$Diagnostic.cpp_tools_found) { Write-Host 'Install Visual Studio C++ game-development tools + Windows SDK. See README.md.' }
    exit 0
}
if (!$Engine) { throw 'Unreal Engine is not installed/detected. Install it, or pass -EngineRoot <UE_5.x folder>. See README.md.' }
if (!$EngineReady) { throw 'Epic is still installing/updating this engine. Wait for the Launch button and completed Launcher registration before building.' }
if (!$Diagnostic.source_assets_ready) { throw 'RawAssets\manifest.json is missing. Run prepare_assets.py in the original game workspace first.' }
$Editor = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$BuildBat = Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat'
$CodeModes = @('Build', 'Setup', 'Package')
if ($Mode -in $CodeModes) {
    if (!$Diagnostic.cpp_tools_found) { throw 'Visual Studio MSVC C++ tools not detected by vswhere. Install the Game development with C++ workload and Windows SDK.' }
    & $BuildBat EchoWardEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
    Require-Exit 'C++ editor build'
    if (!(Test-Path -LiteralPath $Native)) { throw 'Build reported success but native editor module is missing.' }
}
if ($Mode -eq 'Setup') {
    if ($Scene -eq 'Campaign' -and !(Test-Path -LiteralPath (Join-Path $ProjectRoot 'Content\Prototype\Character\SK_Survivor.uasset'))) {
        $PrerequisiteScript = Join-Path $ProjectRoot 'Content\Python\build_prototype.py'
        & $EditorCmd $ProjectFile '/Engine/Maps/Entry' "-ExecutePythonScript=$PrerequisiteScript" -unattended -nosplash -nosound -NoLiveCoding
        Require-Exit 'Character prerequisite import'
    }
    $Script = Join-Path $ProjectRoot $(if($Scene -eq 'Campaign'){'Content\Python\build_campaign.py'}else{'Content\Python\build_prototype.py'})
    $ImportReport = Join-Path $ReportDir $(if($Scene -eq 'Campaign'){'campaign_import.json'}else{'import_result.json'})
    $Started = Get-Date
    # StaticMesh/LevelEditor subsystems require full editor initialization.
    # -run=pythonscript is a commandlet and does not create these subsystems.
    & $EditorCmd $ProjectFile '/Engine/Maps/Entry' "-ExecutePythonScript=$Script" -unattended -nosplash -nosound -NoLiveCoding -stdout -FullStdOutLogOutput "-abslog=$(Join-Path $ReportDir 'import.log')"
    Require-Exit 'Native content import'
    if (!(Test-Path -LiteralPath $ImportReport)) { throw 'Editor did not produce an import report.' }
    if ((Get-Item -LiteralPath $ImportReport).LastWriteTime -lt $Started.AddSeconds(-2)) { throw 'Stale import report; this launch did not finish the importer.' }
    $Import = Get-Content -Raw -LiteralPath $ImportReport | ConvertFrom-Json
    if (!$Import.success) { throw "Import failed: $($Import.error)" }
    if (!(Test-Path -LiteralPath $Map)) { throw "Importer did not save $MapUrl." }
    Write-Host "Native $Scene assets/map generated. Next: bootstrap.ps1 -Mode Play -Scene $Scene."
}
if ($Mode -in @('Editor', 'Play', 'Package')) {
    if (!(Test-Path -LiteralPath $Native) -or !(Test-Path -LiteralPath $Map)) { throw 'Run bootstrap.ps1 -Mode Setup first to build C++ and generate the native map.' }
}
if ($Mode -eq 'Editor') { Start-Process -FilePath $Editor -ArgumentList @(('"'+$ProjectFile+'"'),$MapUrl,'-NoLiveCoding') -WindowStyle Normal }
if ($Mode -eq 'Play') { Start-Process -FilePath $Editor -ArgumentList @(('"'+$ProjectFile+'"'),$MapUrl,'-game','-fullscreen','-NoLiveCoding','-nosplash') -WindowStyle Normal }
if ($Mode -eq 'Package') {
    $RunUAT = Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat'
    $Archive = Join-Path $ProjectRoot 'Saved\PackagedWindows'
    $Verification = Join-Path $Archive 'package_verified.json'
    if (Test-Path -LiteralPath $Verification) { Move-Item -LiteralPath $Verification -Destination (Join-Path $Archive 'package_verified.previous.json') -Force }
    & $RunUAT BuildCookRun "-project=$ProjectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$Archive" -utf8output
    Require-Exit 'Windows package'
    $Prerequisites = Join-Path $Archive 'Windows\Prerequisites'
    New-Item -ItemType Directory -Force -Path $Prerequisites | Out-Null
    foreach ($RuntimeName in @('vc_redist.x64.exe','GameInputRedist.msi')) {
        $RuntimeFile = Join-Path $Engine ('Engine\Extras\Redist\en-us\'+$RuntimeName)
        if (Test-Path -LiteralPath $RuntimeFile) { Copy-Item -LiteralPath $RuntimeFile -Destination (Join-Path $Prerequisites $RuntimeName) -Force }
    }
    Write-Host "Package output: $Archive"
}
