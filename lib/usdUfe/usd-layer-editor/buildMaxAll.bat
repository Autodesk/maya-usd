if exist "D:\repos\3dsmax-component-usd\src\MaxUsdObjects\Icons\UfeRt" rmdir "D:\repos\3dsmax-component-usd\src\MaxUsdObjects\Icons\UfeRt"
mklink /j "D:\repos\3dsmax-component-usd\src\MaxUsdObjects\Icons\UfeRt" "D:\repos\3dsmax-component-usd\artifacts\2027\UsdUfe\lib\icons"

REM If the usd-layer-editor source is available, build it.
if not exist "D:\repos\3dsmax-component-usd\usd-layer-editor" exit

mkdir D:\repos\3dsmax-component-usd\usd-layer-editor\buildForMax2027
pushd D:\repos\3dsmax-component-usd\usd-layer-editor\buildForMax2027

set qt_version=6.8.3
set qt_major_version=%qt_version:~0,1%

set cmake_definitions=-DPXR_USD_LOCATION=D:\repos\3dsmax-component-usd\artifacts\2027\Pixar_USD ^
  -DUFE_INCLUDE_ROOT=D:\repos\3dsmax-component-usd\artifacts\2027\ufe\ufe-6.5.1\common\include ^
  -DUFE_LIB_ROOT=D:\repos\3dsmax-component-usd\artifacts\2027\ufe\ufe-6.5.1\platform\Windows\RelWithDebInfo\lib ^
  -DUSDUFE_ROOT_DIR=D:\repos\3dsmax-component-usd\artifacts\2027\UsdUfe ^
  -DPython3_VERSION=313 ^
  -DPython3_ROOT_DIR=D:\repos\3dsmax-component-usd\artifacts\2027\Python ^
  -DPython3_FIND_STRATEGY=LOCATION ^
  -DPython3_FIND_FRAMEWORK=NEVER ^
  -DPython3_FIND_REGISTRY=NEVER ^
  -DQT_VERSION=%qt_major_version% ^
  -DCMAKE_PREFIX_PATH=D:\repos\3dsmax-component-usd\artifacts\2027\Qt\6.8.3/lib/cmake/Qt%qt_major_version% ^
  -DCMAKE_INSTALL_PREFIX=D:\repos\3dsmax-component-usd\artifacts\2027\UsdLayerEditor

if 2027 GTR 2026 (
    REM additional definitions are required with USD 0.25.11
    cmake %cmake_definitions% -DTBB_DIR=D:\repos\3dsmax-component-usd\artifacts\2027\Pixar_USD/lib/CMake/TBB ^
                              -DOpenSubdiv_DIR=D:\repos\3dsmax-component-usd\artifacts\2027\Pixar_USD/lib/CMake/OpenSubdiv ^
                              .. -G "Visual Studio 17 2022"
) else (
    cmake %cmake_definitions% .. -G "Visual Studio 17 2022"
)

cmake --build . --config RelWithDebInfo --target install

popd