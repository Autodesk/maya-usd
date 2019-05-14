# Building

## Build Requirements

## Using Cmake

Start with cloning the repository and creating the build directory:

```
git clone <git_path_to_maya-usd>
cd maya-usd
mkdir build
cd build
```

Fill out the paths below and run the following commands to build and install.

```
cmake \
      -DCMAKE_INSTALL_PREFIX="<path>" \
      -DMAYA_LOCATION="<path>" \
      -DUSD_LOCATION_OVERRIDE="<path>" \
      -DUSD_CONFIG_FILE="<path>"\
      ..

cmake --build . --target install

Append -- -j <NUM_CORES> or /m:%NUMBER_OF_PROCESSORS% based on your platform or CMake Generator.
```

It is up to the users to select the CMake Generator based on the their preference:

```
-G Ninja -DCMAKE_MAKE_PROGRAM=<path to ninja executable>
-G "Visual Studio 14 2015 Win64" 
-G "Xcode" 
```

### CMake Options

### Prerequisites