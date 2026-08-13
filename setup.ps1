# A script to install SDL and compile the program. Warning: the first running of the program will take time to install/configure SDL's library
# If you pass -F, skip SDL configure/build for a faster compilation.
# Use this only when SDL or the build configuration hasn't changed.

# Our flags
param(
    [switch]$F # "Fast" mode, skips SDL configuration
)

$displaySkipped = $true

# Filepaths we need to keep track of
$SDLRoot = Join-Path $env:USERPROFILE "vendored/SDL"
$SDLBuild = Join-Path $SDLRoot "build"
$SDLInstall = Join-Path $SDLRoot "install"

# Success/failure characters for compatibility
$checkmark = "+"
$skipmark = "-"
$crossmark = "x"

# Commands to run
$commands = @(
    @{
        command = {git clone https://github.com/libsdl-org/SDL.git $SDLRoot};
        message = "Install SDL";
        tests = @("SDL Installed", "Fast Flag")
    }
    @{
        command = {cmake -S $SDLRoot -B $SDLBuild};
        message = "Configure SDL Source";
        tests = @("Fast Flag")
    }
    @{
        command = {cmake --build $SDLBuild --config Release };
        message = "Build SDL with CMake";
        tests = @("Fast Flag")
    }
    @{
        command = {cmake --install $SDLBuild --prefix $SDLInstall};
        message = "Install SDL to vendored/install";
        tests = @("Fast Flag")
    }
    @{
        command = {cmake -S . -B build};
        message = "Configure Project Build";
        tests = @()
    }
    @{
        command = {cmake --build build};
        message = "Build Project";
        tests = @()
    }
    @{
        command = {Copy-Item .\build\Debug\NBody.exe . ; Copy-Item $SDLRoot\install\bin\SDL3.dll .};
        message = "Copy .exe and .dll to Root";
        tests = @()
    }
)

# Run each command, breaking if encountering an error
foreach ($instruction in $commands) {
    $command = $instruction.command
    $message = $instruction.message
    $tests = $instruction.tests

    # Check for various conditions
    $doCommand = $true;
    $reason = ""
    foreach ($test in $tests) {
        if ($test -eq "SDL Installed") {
            $SDLExists = Test-Path -Path $SDLRoot
            $doCommand = $doCommand -and !$SDLExists
        } elseif ($test -eq "Fast Flag") {
            $doCommand = $doCommand -and !$F
        } else {
            Write-Host "Unrecognized condition: $test"
            exit
        }
        if (!$doCommand) {
            $reason = $test
            break
        }
    }

    if ($doCommand) {
        # Run the command, storing its output to $output
        $output = & $command 2>&1
    
        # If no error happened, print the short description of the command
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[$checkmark] - $message" -ForegroundColor Green
        }
        # Otherwise, print the description *and* the output of the command
        else {
            Write-Host "[$crossmark] - $message" -ForegroundColor Red
            $output
            exit 1
        }
    } elseif ($displaySkipped) {
        Write-Host "[$skipmark] - Skipped '$message' - $reason" -ForegroundColor DarkGray
    }
}