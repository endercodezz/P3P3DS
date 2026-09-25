param(
    [Parameter(Mandatory=$true)][string]$RamDirectory
)
$ErrorActionPreference = "Stop"
$ramBase = [uint32]0x08000000
$player = [uint32]0x098B0940
$positionOffset = 0x30
$offset = [int64]$player - [int64]$ramBase + $positionOffset

function Read-Position([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "RAM dump ausente: $Path" }
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($offset -lt 0 -or ($offset + 12) -gt $bytes.Length) { throw "Posicao do player fora do dump: $Path" }
    return @(
        [BitConverter]::ToSingle($bytes, [int]$offset),
        [BitConverter]::ToSingle($bytes, [int]$offset + 4),
        [BitConverter]::ToSingle($bytes, [int]$offset + 8)
    )
}

$p1710 = Read-Position (Join-Path $RamDirectory "ram_vblank_001710.bin")
$p1740 = Read-Position (Join-Path $RamDirectory "ram_vblank_001740.bin")
$drift = [Math]::Abs($p1740[2] - $p1710[2])
"vblank 1710: X={0:F6} Y={1:F6} Z={2:F9}" -f $p1710[0],$p1710[1],$p1710[2]
"vblank 1740: X={0:F6} Y={1:F6} Z={2:F9}" -f $p1740[0],$p1740[1],$p1740[2]
"Z drift: {0:F9}" -f $drift
if ($p1740[2] -ge 14.5 -and $drift -le 0.01) {
    "VSCL_COLLISION_VALIDATION=PASS"
    exit 0
}
"VSCL_COLLISION_VALIDATION=FAIL"
exit 1
