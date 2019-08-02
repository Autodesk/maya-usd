# Building

## Getting and Building the Code

The simplest way to build the plugins is to run the supplied ```build.py``` script. This script will build and install the libraries and plugins into separate directories (e.g plugin/al, plugin/pxr).

Follow the instructions below to run the script with its default behavior, which will build Animal Logic and Pixar plugins/libraries. For more options and documentation, run the script with the ```--help``` parameter.

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
- workspace_location  ---> Directory where the project use as a workspace to build and install plugin/libraries.

Comma-separated list of cmake variables can be also passed to build system using ```--build-args```.
e.g
```
python build.py --build-args="-DBUILD_ADSK_PLUGIN=ON,-DBUILD_PXR_PLUGIN=OFF"
```

Comma-separated list of stages can also be passed to the build system using ```--stages```. By default 
'clean','configure','build','install' stages are executed if this argument is not set.

#### 4. CMake Generator

It is up to the user to select the CMake Generator of choice but, we encourage the use of the Ninja generator.

To use the Ninja Generator, you need to first install the Ninja binary from https://ninja-build.org/

You then need to set the generator to ```Ninja``` and the ```CMAKE_MAKE_PROGRAM``` variable to the Ninja binary you downloaded.
```
python build.py --generator Ninja --build-args=-DCMAKE_MAKE_PROGRAM='path to ninja binary'
```
#### 5. Build location

By default, the build directory is created inside the workspace directory but you can change the location to where ever you want by setting the ```--build-location``` flag.

#### 6. Build Log

Build log is written into ```build_log.txt``` inside the build directory.

#### 7. Run the script

For example, the following will build and install Animal Logic and Pixar plugins/libraries into the workspace directory.

##### Linux:
```
➜ python build.py --maya-location /usr/autodesk/maya2019 --pxrusd-location /usr/local/USD-Master /usr/local/workspace
```
##### MacOS:
```
➜ python build.py --maya-location /Applications/Autodesk/maya2019 --pxrusd-location /opt/local/USD-Master /opt/local/workspace
```
##### Windows:
```
C:\> python build.py --maya-location "C:\Program Files\Autodesk\maya2019" --pxrusd-location C:\USD-Master C:\workspace
```

## CMake Options

Name                        | Description                                       | Default
---                         | ---                                               | ---
BUILD_AL_PLUGIN             | Builds the Animal Logic USD plugin and libraries. | ON
BUILD_PXR_PLUGIN            | Builds the Pixar USD plugin and libraries.        | ON

# Building USD

##### Flags, Version

When building Pixar USD, you need to pass ```--no-maya``` flag to ensure that ```third_party/maya``` plugin doesn't get built since this plugin is now part of maya-usd.

It is important that the version between ```Pixar USD``` and ```Maya USD plugin```match. We are currently using ```v19.05``` for Maya plugin in ```master``` branch and therefore ```Pixar USD``` build should match this version.

##### Boost:

Currently Animal Logic plugin has a dependency on some of the boost components ( e.g thread, filesystem ). When building Pixar USD, one needs to pass following key,value paired arguments for boost to include those components: 
e.g 

```python build_usd.py --build-args boost,"--with-date_time --with-thread --with-system --with-filesystem" --no-maya ~/Desktop/BUILD```

##### Python:

Some DCCs (most notably, Maya) may ship with and run using their own version of Python. In that case, it is important that USD and the plugins for that DCC are built using the DCC's version of Python and not the system version. Note that this is primarily an issue on macOS, where a DCC's version of Python is likely to conflict with the version provided by the system. 

To build USD and the Maya plugins on macOS for Maya 2019, run:
```
/Applications/Autodesk/maya2019/Maya.app/Contents/bin/mayapy build_usd.py --no-maya ~/Desktop/BUILD
```
By default, ``usdview`` is built which has a dependency on PyOpenGL. Since Python version of Maya doesn't ship with PyOpenGL you will be prompted with following error message:
```
PyOpenGL is not installed. If you have pip installed, run "pip install PyOpenGL" to install it, then re-run this script.
If PyOpenGL is already installed, you may need to update your PYTHONPATH to indicate where it is located.
```
The easiest way to bypass this error is but setting up PYTHONPATH to point at your system python or third-party python package manager that has PyOpenGL already installed.
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
