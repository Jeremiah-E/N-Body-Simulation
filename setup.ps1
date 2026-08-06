# I don't feel like running three commands every time I want to run the program, so here's a script to do so
# If you pass -F, skip SDL configure/build for a faster compilation.
# Use this only when SDL or the build configuration hasn't changed.

param(
    [switch]$F
)

$UserName = $env:USERNAME
$SDLRoot = "C:/Users/$UserName/vendored/SDL"
$SDLBuild = "$SDLRoot/build"

$commands = @(
    @{
        command = {cmake -S $SDLRoot -B $SDLBuild};
        message = "Configure SDL source"
    }
    @{
        command = {cmake --build $SDLBuild --config Release };
        message = "Install SDL to vendored/install"
    }
    @{
        command = {cmake -S . -B build};
        message = "Configure project build"
    }
    @{
        command = {cmake --build build};
        message = "Build project"
    }
    @{
        command = {Copy-Item .\build\Debug\NBody.exe . ; Copy-Item $SDLRoot\install\bin\SDL3.dll .};
        message = "Copy executable and lib to current root"
    }
)

# If we give it the F flag, skip two commands
if ($F) {
    $commands = $commands[2..($commands.Count - 1)]
}

# Run each command, breaking if encountering an error
foreach ($instruction in $commands) {
    $command = $instruction.command
    $message = $instruction.message

    # Run the command, storing it
    $output = & $command 2>&1

    # If no error happened, print the message
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[Success] -" $message -ForegroundColor Green
    }
    # Otherwise, print the message *and* the output
    else {
        Write-Host "[Failure] -" $message -ForegroundColor Red
        $output
        break
    }
}