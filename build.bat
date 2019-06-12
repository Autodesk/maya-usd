rem @echo off
setlocal

rem variables to be set
set CORE_USD_BUILD_DIRECTORY='path to core usd directory'
set MAYA_RUNTIME='path to maya runtime'
set INSTALL_LOCATION='path to where you want to install the plugins and libraries'
set BUILD_TYPE='Debug, Release, RelWithDebInfo'
set CORE_NUM=%NUMBER_OF_PROCESSORS%

set MAYA_USD_BUILD_DIRECTORY=%cd%
if not exist "build" mkdir build
cd %MAYA_USD_BUILD_DIRECTORY%/build

cmake .. -G "Visual Studio 15 2017 Win64" ^
-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DCMAKE_INSTALL_PREFIX=%INSTALL_LOCATION% ^
-DMAYA_LOCATION=%MAYA_RUNTIME% ^
-DUSD_LOCATION_OVERRIDE=%CORE_USD_BUILD_DIRECTORY% ^
-DPXR_BUILD_MAYA_PLUGIN=TRUE 

cmake --build . --config %BUILD_TYPE% --target install -- /m:%CORE_NUM%

endlocal
exit /b %errorlevel%
