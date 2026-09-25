param(
    [Parameter(Mandatory=$true)][string]$ExtractedUmdRoot,
    [string]$DecryptedElf = "",
    [string]$Destination = "$PSScriptRoot\..\game",
    [switch]$AllowUnverifiedElf
)
$ErrorActionPreference = "Stop"

$root = (Resolve-Path $ExtractedUmdRoot).Path
$pspGame = Join-Path $root "PSP_GAME"
$paramSfo = Join-Path $pspGame "PARAM.SFO"
if (!(Test-Path $paramSfo)) {
    throw "PARAM.SFO not found. Provide an extracted PSP game directory."
}

New-Item -ItemType Directory -Force $Destination | Out-Null
Copy-Item -Recurse -Force $pspGame $Destination

$sourceElf = $null
if ($DecryptedElf -ne "") {
    $sourceElf = (Resolve-Path $DecryptedElf).Path
} else {
    $candidate = Join-Path $pspGame "SYSDIR\EBOOT_DECRYPTED.ELF"
    if (Test-Path $candidate) { $sourceElf = $candidate }
    if ($null -eq $sourceElf) {
        $boot = Join-Path $pspGame "SYSDIR\BOOT.BIN"
        if (Test-Path $boot) {
            $header = [System.IO.File]::ReadAllBytes($boot)[0..3]
            if ($header[0] -eq 0x7F -and $header[1] -eq 0x45 -and $header[2] -eq 0x4C -and $header[3] -eq 0x46) {
                $sourceElf = $boot
            }
        }
    }
}

if ($null -eq $sourceElf) {
    throw "A decrypted ELF was not found. PSPRecomp does not include EBOOT decryption code; pass -DecryptedElf with a legally obtained ELF."
}

$elfBytes = [System.IO.File]::ReadAllBytes($sourceElf)
if ($elfBytes.Length -lt 4 -or $elfBytes[0] -ne 0x7F -or $elfBytes[1] -ne 0x45 -or $elfBytes[2] -ne 0x4C -or $elfBytes[3] -ne 0x46) {
    throw "The supplied executable is not an ELF file."
}

$expectedSha256 = "fee2e86c7fe457ab6da463d9fdc16f156a0900adcdaa2fd260206c11907c5d65"
$actualSha256 = (Get-FileHash -Algorithm SHA256 $sourceElf).Hash.ToLowerInvariant()
if (!$AllowUnverifiedElf -and $actualSha256 -ne $expectedSha256) {
    throw "Executable SHA-256 does not match the VCS profile. Expected $expectedSha256, got $actualSha256. Use -AllowUnverifiedElf only for development."
}

$sysdir = Join-Path $Destination "PSP_GAME\SYSDIR"
New-Item -ItemType Directory -Force $sysdir | Out-Null
$destinationElf = Join-Path $sysdir "EBOOT_DECRYPTED.ELF"
Copy-Item -Force $sourceElf $destinationElf

Write-Host "Prepared VCS game root: $Destination"
Write-Host "Executable: $destinationElf"
Write-Host "SHA-256: $actualSha256"
