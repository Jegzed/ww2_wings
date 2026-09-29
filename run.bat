@echo off
rem Launches WW2 Wings. Builds the extension first if it is missing.
setlocal
cd /d "%~dp0"

set "GODOT="
for %%G in (godot.exe godot4.exe) do if not defined GODOT for /f "delims=" %%P in ('where %%G 2^>nul') do if not defined GODOT set "GODOT=%%P"
if not defined GODOT (
    for /f "delims=" %%P in ('dir /b /s "%LOCALAPPDATA%\Microsoft\WinGet\Packages\GodotEngine.GodotEngine*\Godot_v4*_win64.exe" 2^>nul') do if not defined GODOT set "GODOT=%%P"
)
if not defined GODOT (
    echo Godot 4.7 was not found. Install it ^(winget install GodotEngine.GodotEngine^) or add it to PATH.
    exit /b 1
)

if not exist game\bin\ww2wings.dll (
    call build.bat
    if errorlevel 1 exit /b 1
)

rem The first run has to register the GDExtension with the project.
if not exist game\.godot\extension_list.cfg (
    "%GODOT%" --headless --path game --import
)

start "" "%GODOT%" --path game %*
