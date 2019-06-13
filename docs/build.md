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

Fill out the following variables below in build scripts:

```
	CORE_USD_BUILD_DIRECTORY='path to core usd directory'
	MAYA_RUNTIME='path to maya runtime'
	INSTALL_LOCATION='path to where you want to install the plugins and libraries'
	BUILD_TYPE='Debug, Release, RelWithDebInfo'
	CORE_NUM='core#'
```

### CMake Options

Name                    | Description                                  | Default
---                     | ---                                          | ---
BUILD_CORE_USD_LIBRARY  | Build the Maya USD core libraries            | ON
BUILD_ADSK_USD_PLUGIN   | Build the Maya USD plugin                    | ON
BUILD_AL_USD_PLUGIN     | Build the Animal Logic plugin and libraries  | ON
BUILD_PXR_USD_PLUGIN    | Build the Pixar plugin and libraries         | ON

### Prerequisites
