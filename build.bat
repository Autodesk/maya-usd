rem @echo off
setlocal

set CORE_USD_BUILD_DIRECTORY=C:\Users\sabrih\Autodesk2019\ECG\ecg-usd-build\install\RelWithDebInfo

set MAYA_USD_BUILD_DIRECTORY=%cd%
cd %MAYA_USD_BUILD_DIRECTORY%
if not exist "build" mkdir build
cd %MAYA_USD_BUILD_DIRECTORY%\build

cmake -G "Visual Studio 15 2017 Win64" .. ^
-DCMAKE_BUILD_TYPE=RelWithDebInfo ^
-DCMAKE_INSTALL_PREFIX=C:\Users\sabrih\Desktop\FINAL_BUILD ^
-DUSD_LOCATION_OVERRIDE=%CORE_USD_BUILD_DIRECTORY% ^
-DBUILD_PXR_USD=TRUE ^
-DPXR_BUILD_MAYA_PLUGIN=TRUE ^
-DBUILD_MAYAUSD_CORE=FALSE ^
-DBUILD_MAYAUSD_PLUGIN=FALSE

endlocal
exit /b %errorlevel%
