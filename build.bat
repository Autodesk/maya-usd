@echo off
setlocal

:: path to pixar's usd build location 
set USD_LOCATION_PATH=''
:: path to maya location
set MAYA_LOCATION_PATH=''
:: path to maya devkit
set MAYA_DEVKIT_LOCATION=''
:: path to where you want to install the project
set INSTALL_LOCATION=''
:: Debug, Release, RelWithDebInfo
set BUILD_TYPE=RelWithDebInfo
:: core num
set CORE_NUM=%NUMBER_OF_PROCESSORS%
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
    set THREAD_FLAG=-j%CORE_NUM%
)
if "%GENRATOR_NAME%"=="VS2017" (
    set G=-G "Visual Studio 15 2017 Win64"
    set THREAD_FLAG="/m:%CORE_NUM%"
)

cmake .. %G% ^
-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DCMAKE_INSTALL_PREFIX=%INSTALL_LOCATION% ^
-DMAYA_LOCATION=%MAYA_LOCATION_PATH% ^
-DUSD_LOCATION_OVERRIDE=%USD_LOCATION_PATH% ^
-DUSD_CONFIG_FILE=%USD_LOCATION_PATH%/pxrConfig.cmake ^
-DBOOST_ROOT=%USD_LOCATION_PATH% ^
-DMAYA_DEVKIT_LOCATION=%MAYA_DEVKIT_LOCATION% ^
-DBUILD_CORE_USD_LIBRARY=%WANT_CORE_USD% ^
-DBUILD_ADSK_USD_PLUGIN=%WANT_ADSK_PLUGIN% ^
-DBUILD_PXR_USD_PLUGIN=%WANT_PXRUSD_PLUGIN% ^
-DBUILD_AL_USD_PLUGIN=%WANT_ALUSD_PLUGIN%

cmake --build . --config %BUILD_TYPE% --target install -- %THREAD_FLAG%

endlocal
exit /b %errorlevel%
