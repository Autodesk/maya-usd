@echo off
setlocal

:: path to core usd directory
if not defined CORE_USD_LOCATION (
    set CORE_USD_LOCATION=''
)
:: path to maya runtime
if not defined MAYA_RUNTIME (
    set MAYA_RUNTIME=''
)
:: path to maya devkit
if not defined MAYA_DEVKIT_LOCATION (
    set MAYA_DEVKIT_LOCATION=''
)
:: path to where you want to install the project
if not defined INSTALL_LOCATION (
    set INSTALL_LOCATION=''
)
:: Debug, Release, RelWithDebInfo
if not defined BUILD_TYPE (
    set BUILD_TYPE=RelWithDebInfo
)
:: core num
if not defined CORE_NUM (
    set CORE_NUM=%NUMBER_OF_PROCESSORS%
)

:: Genrators (Ninja|VS2017)
set GENRATOR_NAME=Ninja
:: Want flags
set WANT_CORE_USD=ON
set WANT_ADSK_PLUGIN=ON
set WANT_PXRUSD_PLUGIN=ON
set WANT_ALUSD_PLUGIN=ON

:: ----------------------------------------------------------

set CUR_DIR=%cd%
if not exist "build" mkdir build
cd %CUR_DIR%/build

if "%GENRATOR_NAME%"=="Ninja" (
    set G=-G Ninja -DCMAKE_MAKE_PROGRAM=%CUR_DIR%/bin/win/ninja.exe
    set THREAD_FLAG=-j
)
if "%GENRATOR_NAME%"=="VS2017" (
    set G=-G "Visual Studio 15 2017 Win64"
    set THREAD_FLAG=-- /m:
)

cmake .. %G% ^
-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DCMAKE_INSTALL_PREFIX=%INSTALL_LOCATION% ^
-DMAYA_LOCATION=%MAYA_RUNTIME% ^
-DUSD_LOCATION_OVERRIDE=%CORE_USD_LOCATION% ^
-DUSD_CONFIG_FILE=%CORE_USD_LOCATION%/pxrConfig.cmake ^
-DBOOST_ROOT=%CORE_USD_LOCATION% ^
-DMAYA_DEVKIT_LOCATION=%MAYA_DEVKIT_LOCATION% ^
-DBUILD_CORE_USD_LIBRARY=%WANT_CORE_USD% ^
-DBUILD_ADSK_USD_PLUGIN=%WANT_ADSK_PLUGIN% ^
-DBUILD_PXR_USD_PLUGIN=%WANT_PXRUSD_PLUGIN% ^
-DBUILD_AL_USD_PLUGIN=%WANT_ALUSD_PLUGIN% ^
-DBUILD_USDMAYA_PXR_TRANSLATORS=OFF

cmake --build . --config %BUILD_TYPE% --target install %THREAD_FLAG% %CORE_NUM%

endlocal
exit /b %errorlevel%
