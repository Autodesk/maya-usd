#!/usr/bin/env bash
mkdir build
cd build

cmake -Wno-dev \
      -DCMAKE_INSTALL_PREFIX='/path/to/install' \
      -DCMAKE_MODULE_PATH='/paths/to/folders/which/have/cmake/files' \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DBOOST_ROOT='/path/to/root/folder' \
      -DMAYA_LOCATION='/path/to/root/folder' \
      -DOPENEXR_LOCATION='/path/to/root/folder'\
      -DOPENGL_gl_LIBRARY='/path/to/glfw/library'\
      -DGLEW_LOCATION='/path/to/root/folder'\
      ..

make -j 46 install
cd -
