# Building

## Getting and Building the Code

The simplest way to build the plugins is to run the supplied ```build.py``` script. This script will build and install the libraries and plugins into three separate directories (USD, AL_USDMaya, MayaUSD).

Follow the instructions below to run the script with its default behavior, which will build the USD Core libraries, ADSK_USDMaya, AL_USDMaya, PXR_USDMaya plugins/libraries. For more options and documentation, run the script with the ```--help``` parameter.

#### 1. Install prerequisites

- Required:
  - C++ compiler:
       - GCC
       - Xcode
       - Microsoft Visual Studio
  - CMake 
  - Python
- Optional
  - Ninja

#### 2. Download the source code

Start by cloning the repository:
```
git clone <git_path_to_maya-usd>
cd maya-usd
```
#### 3. Script arguments

There are four arguments that must be passed to the script: 

- --maya-location   ---> Directory where Maya is installed.
- --pxrusd-location ---> Directory where Pixar USD Core is installed.
- --devkit-location ---> Directory where Maya Devkit is installed.
- install_dir       ---> Directory where project will be installed

When builidng Pixar USD Core, you need to pass ```--no-maya``` to ensure that ```third_party/maya``` plugin doesn't get built.

Custom arguments can be also passed to build system using ```--build-args```.
e.g
```
python build.py --build-args="-DBUILD_ADSK_USD_PLUGIN=ON,-DBUILD_PXR_USD_PLUGIN=OFF"
```

#### 4. CMake Generator

It is up to the user to select the Cmake Generator of choice but we encourage the use of Ninja genrator.

To use the Ninja Generator, you need to first install the Ninja binary from https://ninja-build.org/

You then need to set the generator to ```Ninja``` and the ```CMAKE_MAKE_PROGRAM``` variable to the Ninja binary you downloaded.
```
python build.py --generator Ninja --build-args=-DCMAKE_MAKE_PROGRAM='path to ninja binary'
```
#### 5. Build location

By default the build directory is created in the install_dir but you can change the location to where ever you want by setting the ```--build_dir``` flag.

#### 6. Build Log

Build log is written into ```build_log.txt``` inside the build directory.

#### 7. Run the script

For example, the following will build and install ADSK_USDMaya, AL_USDMaya,PXR_USDMaya plugins/libraries into install_dir.

##### Linux:
```
➜ python build.py --maya-location /usr/autodesk/maya2019 --pxrusd-location /usr/local/USD-Master /usr/local/BUILD-USD
```
##### MacOS:
```
➜ python build.py --maya-location /Applications/Autodesk/maya2019 --pxrusd-location /opt/local/USD-Master /opt/local/BUILD-USD
```
##### Windows:
```
C:\> python build.py --maya-location "C:\Program Files\Autodesk\maya2019" --pxrusd-location C:\USD-Master C:\BUILD-USD
```

## CMake Options

Name                        | Description                                       | Default
---                         | ---                                               | ---
BUILD_MAYAUSD_CORE_LIBRARY  | Builds the Autodesk Maya Core USD libraries.      | ON
BUILD_ADSK_USD_PLUGIN       | Builds the Autodesk Maya USD plugin.              | ON
BUILD_AL_USD_PLUGIN         | Builds the Animal Logic USD plugin and libraries. | ON
BUILD_PXR_USD_PLUGIN        | Builds the Pixar USD plugin and libraries.        | ON
