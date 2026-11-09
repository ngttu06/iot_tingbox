param(
  [ValidateSet('setup','build','upload')][string]$Action = 'build',
  [ValidateSet('esp32','esp32s3')][string]$Board = 'esp32s3',
  [string]$Port = 'COM7',
  [ValidateSet('dfplayer','i2s')][string]$Audio = 'dfplayer'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$CliCommand = Get-Command arduino-cli -ErrorAction SilentlyContinue
if ($CliCommand) { $Cli = $CliCommand.Source }
else {
  $Cli = Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
  if (!(Test-Path -LiteralPath $Cli)) { throw 'Install Arduino CLI or Arduino IDE, or add arduino-cli to PATH.' }
}
function Invoke-Arduino {
  & $Cli @args
  if ($LASTEXITCODE -ne 0) { throw "Arduino CLI failed with exit code $LASTEXITCODE" }
}
if ($Action -eq 'setup') {
  Invoke-Arduino core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  Invoke-Arduino core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  Invoke-Arduino lib install 'ArduinoJson@7.4.2' 'TFT_eSPI@2.5.43' 'ESP8266Audio@2.4.1'
  exit 0
}
$Fqbn = if ($Board -eq 'esp32s3') {
  'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,FlashMode=qio,CDCOnBoot=default,UploadSpeed=921600'
} else { 'esp32:esp32:esp32:PartitionScheme=huge_app' }
$Build = Join-Path $Root "firmware\build\$Board-$Audio"
$Sketch = Join-Path $Root 'firmware\SmartPayESP32'
python (Join-Path $Root 'tools\audio\embed.py')
if ($LASTEXITCODE -ne 0) { throw 'Audio asset verification failed.' }
python (Join-Path $Root 'tools\audio\prepare_dfplayer.py') --check
if ($LASTEXITCODE -ne 0) { throw 'DFPlayer track mapping verification failed.' }
$TftSetup = (Join-Path $Root 'firmware\tft\User_Setup.h').Replace('\','/')
$Backend = if ($Audio -eq 'dfplayer') { 1 } else { 0 }
$Flags = "-DAUDIO_USE_DFPLAYER=$Backend -DUSER_SETUP_LOADED -include `"$TftSetup`""
# Preserve the core's build.extra_flags (BOARD_HAS_PSRAM, USB mode, etc.).
Invoke-Arduino compile --fqbn $Fqbn --build-path $Build --build-property "compiler.cpp.extra_flags=$Flags" --build-property "compiler.c.extra_flags=$Flags" $Sketch
if ($Action -eq 'upload') {
  Invoke-Arduino upload --fqbn $Fqbn --port $Port --upload-property upload.speed=460800 --input-dir $Build $Sketch
}