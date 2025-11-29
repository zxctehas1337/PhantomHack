@echo off
echo Building CS2 Menu...
echo.

REM Поиск MSBuild
set "MSBUILD="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set "MSBUILD=%%i\MSBuild\Current\Bin\MSBuild.exe"
)

if not exist "%MSBUILD%" (
    echo MSBuild not found! Please install Visual Studio 2022.
    pause
    exit /b 1
)

echo Found MSBuild: %MSBUILD%
echo.

REM Компиляция Release x64
echo Building Release x64...
"%MSBUILD%" CS2Menu.sln /p:Configuration=Release /p:Platform=x64 /m

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.
echo Files created:
echo - x64\Release\CS2Menu.dll
echo - x64\Release\Injector.exe
echo.
echo To use:
echo 1. Run CS2
echo 2. Run Injector.exe as Administrator
echo 3. Press INSERT in CS2 to open menu
echo.
pause