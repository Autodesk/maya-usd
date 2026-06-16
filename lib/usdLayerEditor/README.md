# usd-layer-editor

A DCC agnostic USD Layer editor, extracted from [maya-usd](https://git.autodesk.com/maya3d/maya-usd/tree/dev/lib/usd/ui/layerEditor).

This is a work in progress, features from the maya-usd layer editor will be progressively enabled. 
Code that needs attention in that regard is flagged with `TODO LE-EXTRACT` to make it easy to find.

# Build Instructions
Download and install :
- [CMake](https://cmake.org/download/) >= 3.20

The usd-layer-editor depends on : 
- USD
- UFE
- UsdUfe
- Python
- QT

The following CMAKE options can be used to configure dependencies : 

Name                        | Description                            
---                         | ---                                    
PXR_USD_LOCATION            | Path to USD root directory             
UFE_INCLUDE_ROOT            | Path to UFE include directory          
UFE_LIB_ROOT                | Path to UFE lib directory.             
USDUFE_ROOT_DIR             | Path to UsdUfe root directory          
Python3_ROOT_DIR            | Path to the python root directory      
BOOST_LIB_TOOLSET           | Boost library toolset to link against  
QT_VERSION                  | QT version to use, support 5 or 6	     
CMAKE_PREFIX_PATH           | Path to the QT cmake folder            

For example  :

```bash
git clone git@git.autodesk.com:media-and-entertainment/usd-layer-editor.git
cd usd-layer-editor
mkdir build && cd build

cmake  -G "Visual Studio 16 2019" ^
  -DPXR_USD_LOCATION=D:/repos/3dsmax-component-usd/artifacts/2025/Pixar_USD/Release ^
  -DUFE_INCLUDE_ROOT=D:/repos/3dsmax-component-usd/artifacts/2025/ufe/ufe-5.3.0/common/include ^
  -DUFE_LIB_ROOT=D:/repos/3dsmax-component-usd/artifacts/2025/ufe/ufe-5.3.0/platform/Windows/RelWithDebInfo/lib ^
  -DUSDUFE_ROOT_DIR=D:/repos/3dsmax-component-usd/artifacts/2025/UsdUfe ^
  -DPython3_ROOT_DIR=D:/repos/3dsmax-component-usd/artifacts/2025/Python ^
  -DPython3_FIND_STRATEGY=LOCATION ^
  -DPython3_FIND_FRAMEWORK=NEVER ^
  -DPython3_FIND_REGISTRY=NEVER ^
  -DBOOST_LIB_TOOLSET="vc142" ^
  -DQT_VERSION=6 ^
  -DCMAKE_PREFIX_PATH=D:/artifactory/unzipped/Qt/6.5.0-3dsmax-001-vc142-test7/qt/6.5.0/lib/cmake/Qt6 ^
  -DCMAKE_INSTALL_PREFIX=../maxBuilds/2025 ^
  .. 

cmake --build . --config RelWithDebInfo --target install

cd ..
```

bat scripts are is provided as a convenience to build the usd-layer-editor for 3dsMax USD on Windows.