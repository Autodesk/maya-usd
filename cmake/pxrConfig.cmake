# - Configuration file for the pxr project
# Defines the following variables:
# PXR_INCLUDE_DIRS  - Root include directory for the installed project.
# PXR_LIBRARIES     - List of all libraries, by target name.
# PXR_foo_LIBRARY   - Absolute path to individual libraries.
message(STATUS "CMAKESOURCEDIR ${USD_CONFIG_DIR}")
set(PXR_CMAKE_DIR "${AL_USDMAYA_USD_LOCATION}")
if(WIN32)
    include("${USD_CONFIG_DIR}/pxrTargets_win32.cmake")
elseif(APPLE)
    include("${USD_CONFIG_DIR}/pxrTargets_osx.cmake")
else()
    include("${USD_CONFIG_DIR}/pxrTargets_linux.cmake")
endif()
set(libs "arch;tf;gf;js;work;plug;vt;ar;kind;sdf;pcp;usd;usdGeom;usdLux;usdShade;usdHydra;usdRi;usdSkel;usdUI;usdUtils;garch;hf;cameraUtil;pxOsd;glf;hd;hdSt;hdx;hdStream;usdImaging;usdImagingGL;usdSkelImaging;usdviewq")
set(PXR_LIBRARIES "")
set(PXR_INCLUDE_DIRS "${PXR_CMAKE_DIR}/include")
string(REPLACE " " ";" libs "${libs}")
foreach(lib ${libs})
    get_target_property(location ${lib} LOCATION)
    set(PXR_${lib}_LIBRARY ${location})
    list(APPEND PXR_LIBRARIES ${lib})
endforeach()
