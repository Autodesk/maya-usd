# Building

## Build Requirements

## Using Cmake

Start with cloning the repository and creating the build directory:

```
git clone <git_path_to_maya-usd>
cd maya-usd
git submodule update --init
mkdir build
cd build
```

Fill out the paths below and run the following commands to build and install.

```
cmake \
      -DCMAKE_INSTALL_PREFIX="<path to where you want to install the plugins and libraries>" \
      -DMAYA_LOCATION="<path to maya binary directory>" \
      -DMAYA_DEVKIT_LOCATION="<path to devkit directory>"  \
      -DUSD_LOCATION_OVERRIDE="<path to usd directory>" \
      -DUSD_CONFIG_FILE="<path_to_usd_location/pxrConfig.cmake>"\
      ..

cmake --build . --target install

Append -- -j <NUM_CORES> or /m:%NUMBER_OF_PROCESSORS% based on your platform or CMake Generator.
```

It is up to the users to select the CMake Generator based on their preference:

```
-G Ninja -DCMAKE_MAKE_PROGRAM=<path to ninja executable>
-G "Visual Studio 14 2015 Win64" 
-G "Xcode" 
```

### CMake Options

Name                 | Description                                  | Default
---                  | ---                                          | ---
BUILD_MAYAUSD_CORE   | Build the Maya USD core libraries            | ON
BUILD_MAYAUSD_PLUGIN | Build the Maya USD plugin                    | ON
BUILD_AL_USD         | Build the Animal Logic plugin and libraries  | OFF
BUILD_PXR_USD        | Build the Pixar plugin and libraries         | OFF

### Prerequisites
