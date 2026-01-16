# Building

## Getting and Building the Code

The simplest way to build the project is by running the supplied **build.py** script. This script builds the project and installs all of the necessary libraries and plug-ins for you. Follow the instructions below to learn how to use the script. 

#### 1. Tools and System Prerequisites

Before building the project, consult the following table to ensure you use the recommended version of compiler, operating system, cmake, etc. 

|        Required       | ![](images/windows.png)   |                            ![](images/mac.png)               |   ![](images/linux.png)     |
|:---------------------:|:-------------------------:|:------------------------------------------------------------:|:---------------------------:|
|    Operating System   |       Windows 10/11        | High Sierra (10.13)<br>Mojave (10.14)<br>Catalina (10.15)<br>Big Sur (11.2.x)<br>Monterey (12.6) | CentOS 7/8<br>RHEL 8.6<br>Rocky 8.6    |
|   Compiler Requirement| Maya 2022 (VS 2017/2019)<br>Maya 2023 (VS 2019)<br>Maya 2024 (VS 2022)<br>Maya 2025 (VS 2022)<br>Maya 2026 (VS 2022) | Maya 2022 (Xcode 10.2.1)<br>Maya 2023 (Xcode 10.2.1)<br>Maya 2024 (Xcode 13.4)<br>Maya 2025 (Xcode 14.3)<br>Maya 2026 (Xcode 14.3) | Maya 2022 (gcc 6.3.1/9.3.1)<br>Maya 2023 (gcc 9.3.1)<br>Maya 2024 (gcc 11.2.1)<br>Maya 2025 (gcc 11.2.1)<br>Maya 2026 (gcc 11.2.1) |
| CMake Version (min/max) |        3.13 - 3.17      |                              3.13 - 3.17                     |           3.13 - 3.17       |
|         Python        | 2.7.15/3.7.7/3.9.7/3.10/3.11 |               2.7.15/3.7.7/3.9.7/3.10/3.11                | 2.7.15/3.7.7/3.9.7/3.10/3.11 |
|    Python Packages    | PyYAML, PySide, PyOpenGL, Jinja2        | PyYAML, PySide2, PyOpenGL, Jinja2              | PyYAML, PySide, PyOpenGL, Jinja2 |
|    Build generator    | Visual Studio, Ninja (Recommended)    |  XCode, Ninja (Recommended)                      |    Ninja (Recommended)      |
|    Command processor  | Visual Studio X64 {2017/2019/2022} command prompt |                  bash                |             bash            |
| Supported Maya Version| 2023, 2024, 2025, 2026<br>2022: builds no longer provided | 2023, 2024, 2025, 2026<br>2022: builds no longer provided | 2023, 2024, 2025, 2026<br>2022: builds no longer provided |

|        Optional       | ![](images/windows.png)   |                            ![](images/mac.png)               |   ![](images/linux.png)     |
|:---------------------:|:-------------------------:|:------------------------------------------------------------:|:---------------------------:|
|          Qt           | Maya 2022 = 5.15.2<br>Maya 2023 = 5.15.2<br>Maya 2024 = 5.15.2<br>Maya 2025 = 6.5.3<br>Maya 2026 = 6.5.3 | Maya 2022 = 5.15.2<br>Maya 2023 = 5.15.2<br>Maya 2024 = 5.15.2<br>Maya 2025 = 6.5.3<br>Maya 2026 = 6.5.3 | Maya 2022 = 5.15.2<br>Maya 2023 = 5.15.2<br>Maya 2024 = 5.15.2<br>Maya 2025 = 6.5.3<br>Maya 2026 = 6.5.3 |

***NOTE:*** Visit the online Maya developer help document under ***Setting up your build environment*** for additional compiler requirements on different platforms.

#### 2. Download and Build Pixar USD 

See Pixar's official github page for instructions on how to build USD: https://github.com/PixarAnimationStudios/USD . Pixar has removed support for building Maya USD libraries/plug-ins in their github repository and ```build_usd.py```. When building the maya-usd project, it is important the recommended ```Pixar USD``` commitID or tag from the table below are used: 

|               |      ![](images/pxr.png)          | USD version used in Maya | USD source for MayaUsd |
|:------------: |:---------------:                  |:------------------------:|:-------------------------:|
|  CommitID/Tags | Officially supported:<br> [v21.11](https://github.com/PixarAnimationStudios/USD/releases/tag/v21.11), [v22.11](https://github.com/PixarAnimationStudios/USD/releases/tag/v22.11), [v23.11](https://github.com/PixarAnimationStudios/USD/releases/tag/v23.11), [v24.11](https://github.com/PixarAnimationStudios/USD/releases/tag/v24.11), [v25.05](https://github.com/PixarAnimationStudios/USD/releases/tag/v24.05.01), [v25.11](https://github.com/PixarAnimationStudios/USD/releases/tag/v25.11)<br>Other versions used:<br> [v22.05b](https://github.com/PixarAnimationStudios/USD/releases/tag/v22.05b), [v22.08](https://github.com/PixarAnimationStudios/USD/releases/tag/v22.08)<br>[v23.02](https://github.com/PixarAnimationStudios/USD/releases/tag/v23.02), [v23.08](https://github.com/PixarAnimationStudios/USD/releases/tag/v23.08)<br>[v24.05](https://github.com/PixarAnimationStudios/USD/releases/tag/v24.05), [v24.08](https://github.com/PixarAnimationStudios/USD/releases/tag/v24.08)<br>[v25.08](https://github.com/PixarAnimationStudios/USD/releases/tag/v25.08) | Maya 2022 = v21.11<br>Maya 2023 = v21.11<br>Maya 2024 = v22.11<br>Maya 2025 = v23.11<br>Maya 2026 = v24.11 & v25.05 | [v21.11-MayaUsd-Public](https://github.com/autodesk-forks/USD/tree/v21.11-MayaUsd-Public)<br> <br>[v22.11-MayaUsd-Public](https://github.com/autodesk-forks/USD/tree/v22.11-MayaUsd-Public)<br>[v23.11-MayaUsd-Public](https://github.com/autodesk-forks/USD/tree/v23.11-MayaUsd-Public)<br>[v24.11-MayaUsd-Public](https://github.com/autodesk-forks/USD/tree/v24.11-MayaUsd-Public)<br>[v25.11-MayaUsd-Public](https://github.com/autodesk-forks/USD/tree/v25.11-MayaUsd-Public) |

For additional information on building Pixar USD, see the ***Additional Build Instruction*** section below.

***NOTE:*** Make sure that you don't have an older USD locations in your ```PATH``` and ```PYTHONPATH``` environment settings. ```PATH``` and ```PYTHONPATH``` are automatically adjusted inside the project to point to the correct USD location. See ```cmake/usd.cmake```.

#### 3. Universal Front End (UFE)

The Universal Front End (UFE) is a DCC-agnostic component that allows Maya to browse and edit data in multiple data models. This allows Maya to edit pipeline data such as USD. UFE comes installed as a built-in component with Maya 2019 and later. UFE is developed as a separate binary component, and therefore versioned separately from Maya.

| Ufe Version                | Maya Version                                           | Ufe Docs (external) |
|----------------------------|--------------------------------------------------------|:-------------------:|
| v1.0.0                     | Maya 2019.x                                            | |
| v1.0.0                     | Maya 2020.x                                            | |
| v2.0.0<br>v2.0.3<br>v2.1.0 | Maya 2022 <br>Maya 2022.1/2022.2/2022.3<br>Maya 2022.4 | https://help.autodesk.com/view/MAYAUL/2022/ENU/?guid=Maya_SDK_ufe_ref_index_html |
| v3.0.0<br>v3.2.0<br>v3.3.0 | Maya 2023/2023.1<br>Maya 2023.2<br>Maya 2023.3         | https://help.autodesk.com/view/MAYAUL/2023/ENU/?guid=MAYA_API_REF_ufe_ref_index_html |
| v4.0.0<br>v4.1.0<br>v4.2.0 | Maya 2024<br>Maya 2024.1<br>Maya 2024.2                | https://help.autodesk.com/view/MAYAUL/2024/ENU/?guid=MAYA_API_REF_ufe_ref_index_html |
| v5.0.0<br>v5.1.0<br>v5.3.0<br>v5.5.0 | Maya 2025<br>Maya 2025.1<br>Maya 2025.2<br>Maya 2025.3 | https://help.autodesk.com/view/MAYADEV/2025/ENU/?guid=MAYA_API_REF_ufe_ref_index_html |
| v6.0.0<br> | Maya 2026 | https://help.autodesk.com/view/MAYADEV/2026/ENU/?guid=MAYA_API_REF_ufe_ref_index_html |
| v0.7.x                     | Maya BETA                                              | |

To build the project with UFE support, you will need to use the headers and libraries included in the ***Maya Devkit***:

https://www.autodesk.com/developer-network/platform-technologies/maya

***NOTE:*** UFE is only supported in Maya 2019 and later.

#### 4. Download the source code

Start by cloning the repository:
```
git clone https://github.com/Autodesk/maya-usd.git
cd maya-usd
```

##### Repository Layout

| Location      | Description                                                                                   |
|-------------  |---------------------------------------------------------------------------------------------  |
|     lib       | The libraries that all other plugins depend on. Will contain common utilities and features.   |
| plugin/adsk   | The Autodesk Maya plugin                                                                      |
|  plugin/pxr   | The Pixar Maya plugin                                                                         |
|  plugin/al    | The Animal Logic Maya plugin                                                                  |

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
Linux:
➜ maya-usd python build.py --maya-location /usr/autodesk/maya2025 --pxrusd-location /usr/local/USD-Release --devkit-location /usr/local/devkitBase /usr/local/workspace

MacOSX:
➜ maya-usd python build.py --maya-location /Applications/Autodesk/maya2025 --pxrusd-location /opt/local/USD-Release --devkit-location /opt/local/devkitBase /opt/local/workspace

Windows:
c:\maya-usd> python build.py --maya-location "C:\Program Files\Autodesk\Maya2025" --pxrusd-location C:\USD-Release --devkit-location C:\devkitBase C:\workspace
```

##### Default Build Arguments

| Flag                  | Description                                                                           |
|--------------------   |---------------------------------------------------------------------------------------|
|   --materialx         | build with MaterialX features enabled                                                 |

##### Optional Build Arguments

| Flag                  | Description                                                                           |
|--------------------   |---------------------------------------------------------------------------------------|
|   --build-args        | comma-separated list of cmake variables can be also passed to build system.           |
|   --no-materialx      | do not build MaterialX support in MayaUSD.                                            |

```
--build-args="-DBUILD_ADSK_PLUGIN=ON,-DBUILD_PXR_PLUGIN=OFF,-DBUILD_TESTS=OFF"
```

##### CMake Options

Name                        | Description                                                | Default
---                         | ---                                                        | ---
BUILD_MAYAUSD_LIBRARY       | builds Core USD libraries.                                 | ON
BUILD_MAYAUSDAPI_LIBRARY    | Build the mayaUsdAPI subset library that provides a stable versioned interface to mayaUsd for external plugins. | ON
BUILD_LOOKDEVXUSD_LIBRARY   | Build LookdevXUsd library using LookdevXUfe.               | ON
BUILD_ADSK_PLUGIN           | builds Autodesk USD plugin.                                | ON
BUILD_PXR_PLUGIN            | builds the Pixar USD plugin and libraries.                 | ON
BUILD_AL_PLUGIN             | builds the Animal Logic USD plugin and libraries.          | ON
BUILD_HDMAYA                | builds the legacy Maya-To-Hydra plugin and scene delegate. | OFF
BUILD_RFM_TRANSLATORS       | builds translators for RenderMan for Maya shaders.         | ON
BUILD_TESTS                 | builds all unit tests.                                     | ON
BUILD_STRICT_MODE           | enforces all warnings as errors.                           | ON
BUILD_WITH_PYTHON_3			| build with python 3.										 | OFF
BUILD_SHARED_LIBS			| build libraries as shared or static.						 | ON
BUILD_UB2                   | build universal binary 2 (UB2) Intel64+arm64 (apple only)  | OFF
CMAKE_WANT_MATERIALX_BUILD  | enable building with MaterialX (experimental).             | OFF

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

# Additional Build Instruction

##### Python:

It is important to use the Python version shipped with Maya and not the system version when building USD on MacOS. Note that this is primarily an issue on MacOS, where Maya's version of Python is likely to conflict with the version provided by the system. 

To build USD and the Maya plug-ins on MacOS for Maya (2022, 2023, 2024, 2025, 2026), run:
```
/Applications/Autodesk/maya2025/Maya.app/Contents/bin/mayapy build_usd.py ~/Desktop/BUILD
```
By default, ``usdview`` is built which has a dependency on PyOpenGL. Since the Python version of Maya doesn't ship with PyOpenGL you will be prompted with the following error message:
```
PyOpenGL is not installed. If you have pip installed, run "pip install PyOpenGL" to install it, then re-run this script.
If PyOpenGL is already installed, you may need to update your ```PYTHONPATH``` to indicate where it is located.
```
The easiest way to bypass this error is by setting ```PYTHONPATH``` to point at your system python or third-party python package manager that has PyOpenGL already installed.
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

##### Dependencies on Linux DSOs when running tests

Normally either runpath or rpath are used on some DSOs in this library to specify explicit on other libraries (such as USD itself)

If for some reason you don't want to use either of these options, and switch them off with:
```
CMAKE_SKIP_RPATH=TRUE
```
To allow your tests to run, you can inject LD_LIBRARY_PATH into any of the mayaUSD_add_test calls by setting the ADDITIONAL_LD_LIBRARY_PATH cmake variable to $ENV{LD_LIBRARY_PATH} or similar.

There is a related ADDITIONAL_PXR_PLUGINPATH_NAME cmake var which can be used if schemas are installed in a non-standard location

##### Devtoolset-6:

Devtoolset-6, which includes GCC 6 on CentOS has been retired from the main repo and moved into the vault. Follow instructions below for installing devtoolset-6 on CentOS:

```
# download the packages, install may fail with "no public key found"
sudo yum-config-manager --add-repo=http://vault.centos.org/7.6.1810/sclo/x86_64/rh/

# to fix "no public key found"
cd /etc/pki/rpm-gpg
ls # confirm RPM-GPG-KEY-CentOS-SIG-SCLo exists
sudo rpm --import RPM-GPG-KEY-CentOS-SIG-SCLo
rpm -qa gpg* # confirm key with substring f2ee9d55 exists

# to install devtoolset-6
sudo yum install devtoolset-6

# disable the vault after successful install
sudo yum-config-manager --disable vault.centos.org_7.6.1810_sclo_x86_64_rh_
```

# How to Load Plug-ins in Maya 

The provided module files (*.mod) facilitates setting various environment variables for plugins and libraries. After the project is successfully built, ```mayaUsd.mod/alUSD.mod/pxrUSD.mod``` are installed inside the install directory. In order for Maya to discover these mod files, ```MAYA_MODULE_PATH``` environment variable needs to be set to point to the location where the mod files are installed.
Examples:
```
set MAYA_MODULE_PATH=C:\workspace\install\RelWithDebInfo
export MAYA_MODULE_PATH=/usr/local/workspace/install/RelWithDebInfo
```
Once MAYA_MODULE_PATH is set, run maya and go to ```Windows -> Setting/Preferences -> Plug-in Manager``` to load the plugins.

![](images/plugin_manager.png) 
