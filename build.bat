@echo off
setlocal

:: path to core usd directory
set CORE_USD_BUILD_DIRECTORY=''
:: path to maya runtime
set MAYA_RUNTIME=''
:: path to maya devkit
set MAYA_DEVKIT_LOCATION=''
:: path to where you want to install the plugins and libraries
set INSTALL_LOCATION=''
:: Debug, Release, RelWithDebInfo
set BUILD_TYPE=''
:: core num
set CORE_NUM=%NUMBER_OF_PROCESSORS%
:: Genrators (Ninja|VS2017)
set GENRATOR_NAME=Ninja

:: ----------------------------------------------------------

set CUR_DIR=%cd%
if not exist "build" mkdir build
cd %CUR_DIR%/build

if "%GENRATOR_NAME%"=="Ninja" (
    set G=-G Ninja -DCMAKE_MAKE_PROGRAM=%CUR_DIR%/bin/win/ninja.exe
    set THREAD_FLAG=-j
)
if "%GENRATOR_NAME%"=="VS2017" (
    set G=-G Visual Studio 15 2017 Win64
    set THREAD_FLAG=--/m:
)

cmake .. %G% ^
-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DCMAKE_INSTALL_PREFIX=%INSTALL_LOCATION% ^
-DMAYA_LOCATION=%MAYA_RUNTIME% ^
-DUSD_LOCATION_OVERRIDE=%CORE_USD_BUILD_DIRECTORY% ^
-DUSD_CONFIG_FILE=%CORE_USD_BUILD_DIRECTORY%/pxrConfig.cmake ^
-DBOOST_ROOT=%CORE_USD_BUILD_DIRECTORY% ^
-DMAYA_DEVKIT_LOCATION=%MAYA_DEVKIT_LOCATION% 

cmake --build . --config %BUILD_TYPE% --target install %THREAD_FLAG% %CORE_NUM%

endlocal
exit /b %errorlevel%
