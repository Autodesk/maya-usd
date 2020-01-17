# Building

## Getting and Building the Code

The simplest way to build the project is by running the supplied **build.py** script. This script builds the project and installs all of the necessary libraries and plug-ins for you. Follow the instructions below to learn how to use the script. 

#### 1. Tools and System Prerequisites

Before building the project, consult the following table to ensure you use the recommended version of compiler, operating system, cmake, etc. 

|        Required       | ![](images/windows.png)   |                            ![](images/mac.png)               |   ![](images/linux.png)     |
|:---------------------:|:-------------------------:|:------------------------------------------------------------:|:---------------------------:|
|    Operating System   |         Windows 10        | Catalina (10.15), Mojave (10.14), High Sierra (10.13)        |       CentOS 7              |
| Minimum Compiler Version  | Microsoft VS 2017 or 2015 |                      Clang 9.0                           |       GCC 6.3.1             |
| Minimum Cmake Version |           3.13            |                             3.13                             |         3.13                |
|         Python        |           2.7.15          |                            2.7.15                            |        2.7.15               |
|    Python Packages    | PyYAML, PySide, PyOpenGL, Jinja2        | PyYAML, PySide2, PyOpenGL, Jinja2              |      PyYAML, PySide, PyOpenGL, Jinja2             |
|    Build generator    | Visual Studio, Ninja (Recommended)    |  XCode, Ninja(Recommended)                       |    Ninja(Recommended)       |
|    Command processor  | Visual Studio X64 2015 or 2017 command prompt  |                     bash                |             bash            |
| Supported Maya Version|      2018, 2019, 2020           |                      2018, 2019, 2020                  |        2018,2019, 2020      | 

***NOTE:*** We haven't fully tested the plug-ins on ```Catalina``` and it is still at the experimental stage.

#### 2. Download and Build Pixar USD 

See Pixar's official github page for instructions on how to build USD: https://github.com/PixarAnimationStudios/USD

For additional information on building USD, see the ***Building Pixar USD*** section below.

***NOTE:*** Make sure that you don't have older USD locations in your ```PATH``` and ```PYTHONPATH``` environment settings. ```PATH``` and ```PYTHONPATH``` are automatically adjusted inside the project to point to the correct USD location. See ```cmake/usd.cmake```.

#### 3. Download Universal Front End (UFE) 

The Universal Front End (UFE) is a DCC-agnostic component that allows Maya to browse and edit data in multiple data models. This allows Maya to edit pipeline data such as USD.
UFE is developed as a separate binary component, and is therefore versioned separately from Maya. UFE is installed as a built-in component of Maya.

UFE headers and Doxygen documentation is shipped with Maya Devkit and can be downloaded from the link below: 

https://www.autodesk.com/developer-network/platform-technologies/maya  

***NOTE:*** UFE is only supported for Maya 2019 and later. Earlier versions of Maya should still be able to load the plug-ins but without the features enabled by UFE.

#### 4. Download the source code

Start by cloning the repository:
```
git clone https://github.com/Autodesk/maya-usd.git
cd maya-usd
```
#### 5. How To Use build.py Script

##### Arguments

There are four arguments that must be passed to the script: 

| Flags                 | Description                                                                           |
|--------------------   |-------------------------------------------------------------------------------------- |
|   --maya-location     | directory where Maya is installed.                                                    |
|  --pxrusd-location    | directory where Pixar USD Core is installed.                                          |
|  --devkit-location    | directory where Maya devkit is installed.                                             |
| workspace_location    | directory where the project use as a workspace to build and install plugin/libraries  |

```
➜ maya-usd python build.py --maya-location /usr/autodesk/maya2020 --pxrusd-location /usr/local/USD-Master /usr/local/workspace
➜ maya-usd python build.py --maya-location /Applications/Autodesk/maya2020 --pxrusd-location /opt/local/USD-Master /opt/local/workspace
c:\maya-usd> python build.py --maya-location "C:\Program Files\Autodesk\maya2020" --pxrusd-location C:\USD-Master C:\workspace
```

##### Build Arguments

| Flag                  | Description                                                                           |
|--------------------   |---------------------------------------------------------------------------------------|
|   --build-args        | comma-separated list of cmake variables can be also passed to build system.           |

```
--build-args="-DBUILD_ADSK_PLUGIN=ON,-DBUILD_PXR_PLUGIN=OFF,-DBUILD_TESTS=OFF"
```

##### CMake Options

Name                        | Description                                                | Default
---                         | ---                                                        | ---
BUILD_MAYAUSD_LIBRARY       | builds Core USD libraries.                                 | ON
BUILD_ADSK_PLUGIN           | builds Autodesk USD plugin.                                | ON
BUILD_PXR_PLUGIN            | builds the Pixar USD plugin and libraries.                 | ON
BUILD_AL_PLUGIN             | builds the Animal Logic USD plugin and libraries.          | ON
BUILD_TESTS                 | builds all unit tests.                                     | ON
WANT_USD_RELATIVE_PATH      | this flag is used for Autodesk's internal build only.      | OFF
CMAKE_WANT_UFE_BUILD        | enables building with UFE (if found).                      | ON
CMAKE_WANT_QT_BUILD         | enables building with Qt (if found).                       | ON

##### Stages

| Flag                 | Description                                                                           |
|--------------------  |--------------------------------------------------------------------------------------------------- |
|   --stages           | comma-separated list of stages can also be passed to the build system. By default "clean, configure, build, install" stages are executed if this argument is not set. |

| Options       | Description                                                                                   |
|-----------    |---------------------------------------------------                                            |
| clean         | clean build                                                                                   |
| configure     | call this stage every time a cmake file is modified                                           |
| build         | builds the project                                                                            |
| install       | installs all the necessary plug-ins and libraries                                             |
| test          | runs all (PXR,AL,UFE) unit tests                                                              |
| package       | bundles up all the installation files as a zip file inside the package directory              |

```
Examples:
--stages=configure,build,install
--stages=test
```
***NOTE:*** All the flags can be followed by either ```space``` or ```=```

##### CMake Generator

It is up to the user to select the CMake Generator of choice, but we encourage the use of the Ninja generator. To use the Ninja Generator, you need to first install the Ninja binary from https://ninja-build.org/

You then need to set the generator to ```Ninja``` and the ```CMAKE_MAKE_PROGRAM``` variable to the Ninja binary you downloaded.
```
python build.py --generator Ninja --build-args=-DCMAKE_MAKE_PROGRAM='path to ninja binary'
```
##### Build and Install locations

By default, the build and install directories are created inside the **workspace** directory. However, you can change these locations by setting the ```--build-location``` and ```--install-location``` flags. 

##### Build Log

By default the build log is written into ```build_log.txt``` inside the build directory. If you want to redirect the output stream to the console instead,
you can pass ```--redirect-outstream-file``` and set it to false.

##### Additional flags and options

Run the script with the ```--help``` parameter to see all the possible flags and short descriptions.

#### 6. How To Run Unit Tests

Unit tests can be run by setting ```--stages=test``` or by simply calling `ctest` directly from the build directory.

For example, to run all Animal Logic's tests from the command-line go into ```build/<variant>/plugin/al``` and call `ctest`.

```
➜  ctest -j 8
Test project /Users/sabrih/Desktop/workspace/build/Debug/plugin/al
    Start 4: AL_USDMayaTestPlugin
    Start 5: TestUSDMayaPython
    Start 8: TestPxrUsdTranslators
    Start 7: TestAdditionalTranslators
    Start 1: AL_MayaUtilsTests
    Start 3: Python:AL_USDTransactionTests
    Start 2: GTest:AL_USDTransactionTests
    Start 6: testMayaSchemas
1/8 Test #2: GTest:AL_USDTransactionTests .....   Passed    0.06 sec
2/8 Test #6: testMayaSchemas ..................   Passed    0.10 sec
3/8 Test #3: Python:AL_USDTransactionTests ....   Passed    0.73 sec
4/8 Test #1: AL_MayaUtilsTests ................   Passed    6.01 sec
5/8 Test #8: TestPxrUsdTranslators ............   Passed    9.96 sec
6/8 Test #5: TestUSDMayaPython ................   Passed   10.28 sec
7/8 Test #7: TestAdditionalTranslators ........   Passed   12.06 sec
8/8 Test #4: AL_USDMayaTestPlugin .............   Passed   27.43 sec
100% tests passed, 0 tests failed out of 8
```

# Building Pixar USD

##### Flags, Version

Pixar has recently removed support for building Maya USD libraries and plug-ins in their ```build_usd.py```. When building USD, it is important that the ```Pixar USD``` and ```Maya USD plug-in``` versions match. Consult the source versions table in the [Developer](DEVELOPER.md) document. 

##### Boost:

Currently the Animal Logic plugin has a dependency on some of the boost components ( e.g thread, filesystem ). When building Pixar USD, one needs to pass the following key,value paired arguments for boost to include those components: 
e.g 

```
python build_usd.py ~/Desktop/BUILD --build-args boost,"--with-date_time --with-thread --with-system --with-filesystem" 
```

##### Python:

Some DCCs (most notably, Maya) may ship with and run using their own version of Python. In that case, it is important that USD and the plugins for that DCC are built using the DCC's version of Python and not the system version. Note that this is primarily an issue on macOS, where a DCC's version of Python is likely to conflict with the version provided by the system. 

To build USD and the Maya plug-ins on macOS for Maya(2018,2019,2020), run:
```
/Applications/Autodesk/maya2019/Maya.app/Contents/bin/mayapy build_usd.py ~/Desktop/BUILD
```
By default, ``usdview`` is built which has a dependency on PyOpenGL. Since the Python version of Maya doesn't ship with PyOpenGL you will be prompted with the following error message:
```
PyOpenGL is not installed. If you have pip installed, run "pip install PyOpenGL" to install it, then re-run this script.
If PyOpenGL is already installed, you may need to update your PYTHONPATH to indicate where it is located.
```
The easiest way to bypass this error is by setting PYTHONPATH to point at your system python or third-party python package manager that has PyOpenGL already installed.
e.g
```
export PYTHONPATH=$PYTHONPATH:Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages
```
Use `pip list` to see the list of installed packages with your python's package manager.

e.g
```
➜ pip list
DEPRECATION: Python 2.7 will reach the end of its life on January 1st, 2020. Please upgrade your Python as Python 2.7 won't be maintained after that date. A future version of pip will drop support for Python 2.7.
Package    Version
---------- -------
Jinja2     2.10   
MarkupSafe 1.1.0  
pip        19.1.1 
PyOpenGL   3.1.0  
PySide2    5.12.1 
PyYAML     3.13   
setuptools 39.0.1 
shiboken2  5.12.1 
```

# How to Load Plug-ins in Maya 

The provided module file (*.mod) facilitates setting various environment variables for plugins and libraries. After the project is successfully built, ```mayaUsd.mod``` is installed inside the install directory. In order for Maya to discover this mod file, ```MAYA_MODULE_PATH``` environment variable needs to be set to point to the location where the mod file is installed.
Examples:
```
set MAYA_MODULE_PATH=C:\workspace\install\RelWithDebInfo
export MAYA_MODULE_PATH=/usr/local/workspace/install/RelWithDebInfo
```
Once MAYA_MODULE_PATH is set, run maya and go to ```Windows->Setting/Preferences-->Plug-in Manager``` to load the plugins.

![](images/plugin_manager.png) 
