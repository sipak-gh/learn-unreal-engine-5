@echo off
rem Example for chapter 20 of "Learn Unreal Engine 5".
rem Packages a project for Windows with RunUAT BuildCookRun.
rem
rem Usage:   package-windows.bat [Shipping^|Development]
rem Edit ENGINE_DIR, PROJECT and OUTPUT_DIR below first.
rem
rem The flags, in order:
rem   BuildCookRun         the UAT command that does build + cook + stage + package
rem   -project=...         the .uproject to package (absolute path)
rem   -noP4                do not talk to Perforce source control
rem   -unattended          never wait for a dialog; fail instead
rem   -utf8output          UTF-8 log output, so non-ASCII paths print correctly
rem   -platform=Win64      the target platform
rem   -clientconfig=...    the build configuration: Shipping or Development
rem   -build               compile the game target for that platform and configuration
rem   -cook                convert the content to the platform's runtime format
rem   -stage               copy binaries and cooked content into a staging folder
rem   -pak                 put the staged content into .pak container files
rem   -iostore             ...and into IoStore containers (.utoc/.ucas)
rem   -compressed          compress the containers (smaller, a little slower to load)
rem   -prereqs             include the prerequisites installer (Visual C++ runtime)
rem   -package             run the platform's packaging step
rem   -archive             copy the finished build to -archivedirectory
rem   -archivedirectory=   where the finished build goes; a "Windows" folder is created inside

setlocal

set "ENGINE_DIR=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=C:\Projects\GridSnake\GridSnake.uproject"
set "OUTPUT_DIR=C:\Builds\GridSnake"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Shipping"

if not exist "%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" (
  echo RunUAT.bat not found - set ENGINE_DIR in this script.
  exit /b 1
)
if not exist "%PROJECT%" (
  echo %PROJECT% not found - set PROJECT in this script.
  exit /b 1
)

call "%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="%PROJECT%" ^
  -noP4 ^
  -unattended ^
  -utf8output ^
  -platform=Win64 ^
  -clientconfig=%CONFIG% ^
  -build ^
  -cook ^
  -stage ^
  -pak ^
  -iostore ^
  -compressed ^
  -prereqs ^
  -package ^
  -archive ^
  -archivedirectory="%OUTPUT_DIR%"

if errorlevel 1 (
  echo Packaging FAILED - scroll up to the first error.
  exit /b 1
)
echo Packaged %CONFIG% build in %OUTPUT_DIR%\Windows
