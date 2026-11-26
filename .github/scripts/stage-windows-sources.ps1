# All archives are verified by the native runner against tools/sources.sha256 before unpacking.
$ErrorActionPreference = 'Stop'
$names = python scripts/windows/runner.py --inputs $env:PHP_RELEASE release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$cache = Join-Path $env:PLUTOBOOK_WINDOWS_ROOT 'cache'
New-Item -ItemType Directory -Force $cache | Out-Null
$archives = @{}
foreach ($line in Get-Content tools/sources.sha256) {
    if ($line -match '^([0-9a-f]{64})\s+(https://\S+)$') {
        $url = $Matches[2]
        $archives[$url.Split('/')[-1]] = @{ url = $url; sha = $Matches[1] }
    }
}
foreach ($name in $names) {
    $pin = $archives[$name]
    if (!$pin) { throw "No source pin for $name" }
    $file = Join-Path $cache $name
    if ((Test-Path -LiteralPath $file) -and (Get-FileHash -LiteralPath $file).Hash -eq $pin.sha) { continue }
    curl.exe --fail --location --silent --show-error --retry 3 --output $file $pin.url
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    if ((Get-FileHash -LiteralPath $file).Hash -ne $pin.sha) { throw "Checksum mismatch for $name" }
}
