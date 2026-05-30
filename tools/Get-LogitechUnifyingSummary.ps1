param(
    [string]$Vid = "046D",
    [string]$ProductId = "C52B",
    [int]$Slots = 6,
    [int]$TimeoutMs = 1200
)

$ErrorActionPreference = "Stop"
$tool = Join-Path $PSScriptRoot "Invoke-LogitechHidppReport.ps1"

function Convert-HexBytes([string]$text) {
    return @($text -split '\s+' | Where-Object { $_ -ne "" } | ForEach-Object {
        [Convert]::ToByte($_, 16)
    })
}

function Invoke-ReadLongRegister([byte]$subRegister) {
    $sub = "{0:X2}" -f $subRegister
    & $tool `
        -Vid $Vid `
        -ProductId $ProductId `
        -UsagePage FF00 `
        -Usage 0001 `
        -ReadUsagePage FF00 `
        -ReadUsage 0002 `
        -Report "10,FF,83,B5,$sub,00,00" `
        -TimeoutMs $TimeoutMs
}

function Read-OptionalLongRegister([byte]$subRegister) {
    try {
        return Invoke-ReadLongRegister $subRegister
    } catch {
        return $null
    }
}

$receiver = Invoke-ReadLongRegister 0x03
$receiverBytes = Convert-HexBytes $receiver.Received
$receiverPayload = $receiverBytes[4..($receiverBytes.Count - 1)]

$summary = [ordered]@{
    VidPid = "$Vid`:$ProductId"
    ReceiverSerial = (($receiverPayload[1..4] | ForEach-Object { "{0:X2}" -f $_ }) -join "")
    MaxDevices = $receiverPayload[6]
    Devices = @()
}

for ($slot = 1; $slot -le $Slots; $slot++) {
    $pairingInfo = Read-OptionalLongRegister ([byte](0x20 + $slot - 1))
    if ($null -eq $pairingInfo) {
        continue
    }

    $infoBytes = Convert-HexBytes $pairingInfo.Received
    if ($infoBytes.Count -lt 13) {
        continue
    }

    $name = ""
    $nameInfo = Read-OptionalLongRegister ([byte](0x40 + $slot - 1))
    if ($null -ne $nameInfo) {
        $nameBytes = Convert-HexBytes $nameInfo.Received
        if ($nameBytes.Count -gt 6) {
            $nameLen = [Math]::Min($nameBytes[5], $nameBytes.Count - 6)
            if ($nameLen -gt 0) {
                $name = [Text.Encoding]::ASCII.GetString($nameBytes[6..(5 + $nameLen)])
            }
        }
    }

    $kind = switch ($infoBytes[11]) {
        0x01 { "keyboard" }
        0x02 { "mouse" }
        0x03 { "numpad" }
        default { "unknown" }
    }

    $summary.Devices += [ordered]@{
        Slot = $slot
        WPID = "{0:X2}{1:X2}" -f $infoBytes[7], $infoBytes[8]
        Protocol = "{0:X2}" -f $infoBytes[9]
        Kind = $kind
        DeviceFlags = "{0:X2}" -f $infoBytes[12]
        Name = $name
    }
}

[PSCustomObject]$summary
