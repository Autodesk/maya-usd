# Building

## Build Requirements
- This project is buildable on a variety of Linux platforms (It has been tested extensively on CentOS 6.6 and 7.0) and on Windows 7 (it's probably buildable on more recent versions also)
- USD-0.18.11 (18.11) built with ptex-2.0.40 (<2.0.41)
- [google test framework](https://github.com/google/googletest) (>1.8.0) to build and run the tests
- GLEW: Maya (certainly up to 2018) uses a very old build of GLEW. We've found that both to use the latest OpenGL 4x features that Hydra exploits, and for stability reasons, you should use a newer version (e.g GLEW 2.0). We've had to LD_PRELOAD GLEW 2.0 to make this happen. 

### Pixar USDMaya
Recent versions of AL_USDMaya contain an export plugin for Pixar's USDMaya - see [here](../translators/pxrUsdTranslators) creating a plugin called AL_USDMayaPxrTranslators. This sandboxes the main AL_USDMayaPlugin from needing to link against any pixar maya libraries (ie, usdMaya)
There is a cmake option to disable building of the plugin entirely, BUILD_USDMAYA_PXR_TRANSLATORS

## Supported Maya Versions 
+ maya-2016 extension 2
  - have done limited testing (via docker). Selection probably broken. Does it work in VP1?
+ maya-2017
  - Update 3 recommended for viewport selection fixes
+ maya-2018
  - working as far as we know
+ maya-2019
  - we have done some testing up to and including beta PR97 

## Using Cmake

Start with preparing the repository ready for a build

```
git clone <git_path_to_AL_UsdMaya>
cd AL_USDMaya
mkdir build
cd build
```

Fill out the paths below then run the following commands to build and install
```
cmake \
      -DCMAKE_INSTALL_PREFIX='/path/to/install' \
      -DCMAKE_MODULE_PATH='/paths/to/folders/which/have/cmake/files' \
      -DBOOST_ROOT='/path/to/boost' \
      -DMAYA_LOCATION='/path/to/maya' \
      -DUSD_CONFIG_FILE='/path/to/pxrConfig.cmake'\
      -DGTEST_ROOT='/path/to/googletest'\
      -DCMAKE_PREFIX_PATH='/path/to/maya/lib/cmake'
      ..

make -j <NUM_CORES> install
```

### CMake Options

Name | Description | Default
--- | --- | ---
BUILD_USDMAYA_SCHEMAS | Build optional schemas | ON
BUILD_USDMAYA_TRANSLATORS | Build optional translators | ON

## Using Rez
```
git clone <git_path_to_AL_UsdMaya>
cd AL_USDMaya
<edit package names to match your internal named rez packages>
rez build --build-target RelWithDebInfo --install -- -- -j 8
```

## Using AL's Docker scripts

### Prerequisites

AL_USDMaya's DockerFile needs to be based on a docker image that contains Maya and usd. Build the wanted flavour of docker files from the docker-usd repository that contain the OS and maya version you would like to base your AL_USDMaya package on:

For example I want to base it on centos6 with maya2016 installed:
```
git clone https://github.com/AnimalLogic/docker-usd
cd docker-usd/linux
sudo ./build-centos6_maya2016.sh
```

### Build AL_USDMaya with Docker

To build AL_USDMaya's docker image:
```
git clone https://github.al.com.au/rnd/AL_USDMaya
cd AL_USDMaya
sudo ./build_docker_centos6.sh
```
Feel free to use any flavour of the build_docker_*.sh scripts. You just need to make sure you build the same flavour in the docker-usd repository.

# Running

AL_USDMaya is made of several plugins:
- AL_USDMayaPlugin.so (maya plugin): its path has to be added to MAYA_PLUG_IN_PATH
- libAL_USDMaya.so, libAL_USDMayaSchemas.so, libAL_USDMayaTranslators.so (usd plugins): they need to be setup properly (using PXR_PLUGINPATH_NAME) for them to register types used by AL_USDMaya.
- python bindings are also available, they have to be added to PYTHONPATH.
See the docker scripts for a configuration example.

### Additional `MayaReference` translator

To be able to use the additional `MayaReference` translator, an environment variable has to be set to the path where AL_USDMaya has been installed. By default, the environment variable's name is `AL_USDMAYA_LOCATION`.
This environment name can be configured when maya is built by setting the AL_USDMAYA_LOCATION_NAME cmake variable to the name you want.

### A note on VP2
If you need to use VP2, it won't work with the current ProxyShape implementation unless you set Legacy Profile - can to this via env vars, options etc.
e.g you can also use the following MEL code to switch the Maya preferences to use the Viewport 2.0 based OpenGL Legacy Mode profile:
```
optionVar -sv "vp2RenderingEngine" "OpenGL";
```
or env var:
```
MAYA_VP2_DEVICE_OVERRIDE=VirtualDeviceGL 
```
will force VP2 to use the "OpenGL -- Legacy" profile

see [Autodesk docs](https://knowledge.autodesk.com/support/maya/learn-explore/caas/CloudHelp/cloudhelp/2017/ENU/Maya/files/GUID-4928A912-DA6C-4734-863B-AB5959DA73C9-htm.html)

For selection to work in versions of Maya prior to `2019`, you also need to set:
```
MAYA_VP2_USE_VP1_SELECTION=1 
```
as mentioned here:
http://help.autodesk.com/view/MAYAUL/2017/ENU/?guid=__files_GUID_343690A7_F76D_4CD2_A964_E10DA7B5BDF8_htm

# Our internal development workflow
We use git and github at Animal Logic. This repository and it's companions are git subtrees of our internal repo. This repo contains almost everything in our internal repo except for some of the Rez files we use internally for building (we do provide examples). Rez is an excellent package management system used by many studios in our industry, and you can find the source [here](https://github.com/nerdvegas/rez). We run AL_USDMaya unit tests on our internal Jenkins server

