rem @echo off
setlocal

set CORE_USD_BUILD_DIRECTORY='path to core usd directory'

set MAYA_USD_BUILD_DIRECTORY=%cd%
if not exist "build" mkdir build
cd %MAYA_USD_BUILD_DIRECTORY%\build

cmake -G "Visual Studio 15 2017 Win64" .. ^
-DCMAKE_BUILD_TYPE=RelWithDebInfo ^
-DCMAKE_INSTALL_PREFIX='path to where you want to install the plugins and libraries' ^
-DUSD_LOCATION_OVERRIDE=%CORE_USD_BUILD_DIRECTORY% ^
-DPXR_BUILD_MAYA_PLUGIN=TRUE ^
-DBUILD_MAYAUSD_CORE=FALSE ^
-DBUILD_MAYAUSD_PLUGIN=FALSE

endlocal
exit /b %errorlevel%
