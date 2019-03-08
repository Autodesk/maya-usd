# - Configuration file for the pxr project
# Defines the following variables:
# PXR_INCLUDE_DIRS  - Root include directory for the installed project.
# PXR_LIBRARIES     - List of all libraries, by target name.
# PXR_foo_LIBRARY   - Absolute path to individual libraries.
set(PXR_CMAKE_DIR "${AL_USDMAYA_USD_LOCATION}")
get_filename_component(_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
if(WIN32)
    include("${_DIR}/pxrTargets_win32.cmake")
elseif(APPLE)
    include("${_DIR}/pxrTargets_osx.cmake")
else()
    include("${_DIR}/pxrTargets_linux.cmake")
endif()
set(libs "arch;tf;gf;js;trace;work;plug;vt;ar;kind;sdf;ndr;sdr;pcp;usd;usdGeom;usdVol;usdLux;usdShade;usdHydra;usdRi;usdSkel;usdUI;usdUtils;garch;hf;cameraUtil;pxOsd;glf;hd;hdSt;hdx;hdStream;usdImaging;usdImagingGL;usdShaders;usdSkelImaging;usdVolImaging;usdviewq;pxrUsdTranslators")
set(PXR_LIBRARIES "")
set(PXR_INCLUDE_DIRS "${PXR_CMAKE_DIR}/include")
string(REPLACE " " ";" libs "${libs}")
foreach(lib ${libs})
    get_target_property(location ${lib} LOCATION)
    set(PXR_${lib}_LIBRARY ${location})
    list(APPEND PXR_LIBRARIES ${lib})
endforeach()
