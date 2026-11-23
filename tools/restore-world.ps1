param([string]$Project = (Join-Path $PSScriptRoot '..\unreal\EchoWard'))

$ErrorActionPreference = 'Stop'
$data = Join-Path (Resolve-Path -LiteralPath $Project).Path 'Content\CampaignData'
$world = Join-Path $data 'hospital_world.json'
$packed = "$world.gz"
$expected = [IO.File]::ReadAllText("$world.sha256").Trim()
if (Test-Path -LiteralPath $world -PathType Leaf) {
    if ((Get-FileHash -LiteralPath $world -Algorithm SHA256).Hash -ne $expected) {
        throw "Existing hospital_world.json differs from the repository data. Keep a copy of your edited file before restoring the packaged version."
    }
    Write-Output $world
    return
}

$temp = "$world.$([guid]::NewGuid().ToString('N')).tmp"
try {
    $inputStream = [IO.File]::OpenRead($packed)
    try {
        $reader = [IO.Compression.GZipStream]::new($inputStream, [IO.Compression.CompressionMode]::Decompress, $true)
        try {
            $outputStream = [IO.File]::Open($temp, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            try { $reader.CopyTo($outputStream) } finally { $outputStream.Dispose() }
        } finally { $reader.Dispose() }
    } finally { $inputStream.Dispose() }
    if ((Get-FileHash -LiteralPath $temp -Algorithm SHA256).Hash -ne $expected) {
        throw 'The extracted world failed SHA-256 verification.'
    }
    [IO.File]::Move($temp, $world)
    Write-Output $world
} finally {
    if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp }
}
