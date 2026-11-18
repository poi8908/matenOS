#requires -Version 5.1
# Build matenOS: C kernel (GCC -m32) + NASM boot sector -> floppy image.
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root 'build'
$ObjDir = Join-Path $BuildDir 'obj'
$KernelDir = Join-Path $Root 'kernel'
$BootDir = Join-Path $Root 'boot'

New-Item -ItemType Directory -Force -Path $BuildDir, $ObjDir | Out-Null
Get-ChildItem $ObjDir -Filter *.o -ErrorAction SilentlyContinue | Remove-Item -Force

function Find-Tool {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$Candidates
    )
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return (Resolve-Path $candidate).Path
        }
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    throw "Cannot find $Name. Install Dev-C++/MinGW and NASM, or add them to PATH."
}

$Gcc = Find-Tool 'gcc' @(
    'C:\Program Files (x86)\Dev-Cpp\MinGW64\bin\gcc.exe',
    'C:\Program Files\Dev-Cpp\MinGW64\bin\gcc.exe',
    'C:\MinGW\bin\gcc.exe'
)
$ToolDir = Split-Path -Parent $Gcc
$As = Find-Tool 'as' @((Join-Path $ToolDir 'as.exe'))
$Ld = Find-Tool 'ld' @((Join-Path $ToolDir 'ld.exe'))
$Objcopy = Find-Tool 'objcopy' @((Join-Path $ToolDir 'objcopy.exe'))
$Nasm = Find-Tool 'nasm' @(
    (Join-Path $env:LOCALAPPDATA 'bin\NASM\nasm.exe'),
    'C:\Program Files\NASM\nasm.exe'
)

Write-Host "compiler : $Gcc"
Write-Host "assembler: $As"
Write-Host "linker   : $Ld"
Write-Host "nasm     : $Nasm"
Write-Host ''

$cflags = @(
    '-m32',
    '-std=gnu99',
    '-ffreestanding',
    '-fno-builtin',
    '-fno-leading-underscore',
    '-fno-stack-protector',
    '-fno-pic',
    '-fno-asynchronous-unwind-tables',
    '-fno-common',
    '-mno-sse',
    '-mno-mmx',
    '-mno-80387',
    '-mpreferred-stack-boundary=2',
    '-O2',
    '-Wall',
    '-Wextra',
    '-Wno-unused-parameter',
    "-I$KernelDir"
)

$objects = @()
foreach ($source in Get-ChildItem $KernelDir -Filter *.c | Sort-Object Name) {
    $object = Join-Path $ObjDir ($source.BaseName + '.o')
    Write-Host ("  CC  {0}" -f $source.Name)
    & $Gcc @cflags -c $source.FullName -o $object
    if ($LASTEXITCODE -ne 0) { throw "gcc failed on $($source.Name)" }
    $objects += $object
}

foreach ($assembly in @('entry.S', 'isr.S')) {
    $object = Join-Path $ObjDir (([IO.Path]::GetFileNameWithoutExtension($assembly)) + '.o')
    Write-Host ("  AS  {0}" -f $assembly)
    & $As --32 -o $object (Join-Path $KernelDir $assembly)
    if ($LASTEXITCODE -ne 0) { throw "as failed on $assembly" }
    $objects += $object
}

$kernelExe = Join-Path $BuildDir 'kernel.exe'
$kernelBin = Join-Path $BuildDir 'kernel.bin'
Write-Host '  LD  kernel'
& $Ld -m i386pe -T (Join-Path $KernelDir 'kernel.ld') -Map (Join-Path $BuildDir 'kernel.map') -o $kernelExe @objects
if ($LASTEXITCODE -ne 0) { throw 'kernel link failed' }

& $Objcopy -O binary --only-section=.text $kernelExe $kernelBin
if ($LASTEXITCODE -ne 0) { throw 'objcopy failed on kernel' }

$kernel = [IO.File]::ReadAllBytes($kernelBin)
if ($kernel.Length -eq 0) { throw 'kernel is empty' }
if ($kernel.Length -gt 65024) { throw "kernel too large for the 16-bit loader: $($kernel.Length) bytes" }

$sectors = [int][Math]::Ceiling($kernel.Length / 512.0)
Write-Host ("      kernel {0} bytes, {1} sectors" -f $kernel.Length, $sectors)

$bootBin = Join-Path $BuildDir 'boot.bin'
Write-Host '  AS  boot sector'
& $Nasm -f bin "-DKERNEL_SECTORS=$sectors" -l (Join-Path $BuildDir 'boot.lst') -o $bootBin (Join-Path $BootDir 'boot.asm')
if ($LASTEXITCODE -ne 0) { throw 'nasm failed on boot.asm' }

$boot = [IO.File]::ReadAllBytes($bootBin)
if ($boot.Length -ne 512) { throw "boot sector is $($boot.Length) bytes, expected 512" }
if ($boot[510] -ne 0x55 -or $boot[511] -ne 0xAA) { throw 'missing 0x55AA boot signature' }

$floppySize = 1474560
if (512 + $kernel.Length -gt $floppySize) { throw 'kernel does not fit on the floppy image' }
$image = New-Object byte[] $floppySize
[Array]::Copy($boot, 0, $image, 0, 512)
[Array]::Copy($kernel, 0, $image, 512, $kernel.Length)

$imagePath = Join-Path $BuildDir 'matenos.img'
[IO.File]::WriteAllBytes($imagePath, $image)

$hash = (Get-FileHash $imagePath -Algorithm SHA256).Hash.ToLower()
Write-Host ''
Write-Host 'matenOS build complete'
Write-Host ("  image      : {0}" -f $imagePath)
Write-Host ("  boot sector: {0} bytes" -f $boot.Length)
Write-Host ("  kernel     : {0} bytes" -f $kernel.Length)
Write-Host ("  sha256     : {0}" -f $hash)
