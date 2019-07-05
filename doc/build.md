# Building

## Build Requirements

## Using Cmake

Start with cloning the repository and creating the build directory:

```
git clone <git_path_to_maya-usd>
cd maya-usd
```

Fill out the following variables below in build scripts and run it:

```
CORE_USD_BUILD_DIRECTORY='path to core usd directory'
MAYA_RUNTIME='path to maya runtime'
INSTALL_LOCATION='path to where you want to install the plugins and libraries'
BUILD_TYPE='Debug, Release, RelWithDebInfo'
CORE_NUM='core#'
```

### CMake Options

Name                    | Description                                       | Default
---                     | ---                                               | ---
BUILD_CORE_USD_LIBRARY  | Builds the Core USD libraries.                    | ON
BUILD_ADSK_PLUGIN       | Builds the Autodesk USD plugin.                   | ON
BUILD_AL_PLUGIN         | Builds the Animal Logic USD plugin and libraries. | ON
BUILD_PXR_PLUGIN        | Builds the Pixar USD plugin and libraries.        | ON

### Prerequisites
