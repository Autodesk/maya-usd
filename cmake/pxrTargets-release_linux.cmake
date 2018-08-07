#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "arch" for configuration "Release"
set_property(TARGET arch APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(arch PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "dl;"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libarch.so"
  IMPORTED_SONAME_RELEASE "libarch.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS arch )
list(APPEND _IMPORT_CHECK_FILES_FOR_arch "${_IMPORT_PREFIX}/lib/libarch.so" )

# Import target "tf" for configuration "Release"
set_property(TARGET tf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(tf PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libboost_date_time.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtf.so"
  IMPORTED_SONAME_RELEASE "libtf.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS tf )
list(APPEND _IMPORT_CHECK_FILES_FOR_tf "${_IMPORT_PREFIX}/lib/libtf.so" )

# Import target "gf" for configuration "Release"
set_property(TARGET gf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(gf PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libgf.so"
  IMPORTED_SONAME_RELEASE "libgf.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS gf )
list(APPEND _IMPORT_CHECK_FILES_FOR_gf "${_IMPORT_PREFIX}/lib/libgf.so" )

# Import target "js" for configuration "Release"
set_property(TARGET js APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(js PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libjs.so"
  IMPORTED_SONAME_RELEASE "libjs.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS js )
list(APPEND _IMPORT_CHECK_FILES_FOR_js "${_IMPORT_PREFIX}/lib/libjs.so" )

# Import target "trace" for configuration "Release"
set_property(TARGET trace APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(trace PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtrace.so"
  IMPORTED_SONAME_RELEASE "libtrace.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS trace )
list(APPEND _IMPORT_CHECK_FILES_FOR_trace "${_IMPORT_PREFIX}/lib/libtrace.so" )

# Import target "work" for configuration "Release"
set_property(TARGET work APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(work PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;trace;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libwork.so"
  IMPORTED_SONAME_RELEASE "libwork.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS work )
list(APPEND _IMPORT_CHECK_FILES_FOR_work "${_IMPORT_PREFIX}/lib/libwork.so" )

# Import target "plug" for configuration "Release"
set_property(TARGET plug APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(plug PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;js;trace;work;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libplug.so"
  IMPORTED_SONAME_RELEASE "libplug.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS plug )
list(APPEND _IMPORT_CHECK_FILES_FOR_plug "${_IMPORT_PREFIX}/lib/libplug.so" )

# Import target "vt" for configuration "Release"
set_property(TARGET vt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(vt PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;gf;trace;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libvt.so"
  IMPORTED_SONAME_RELEASE "libvt.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS vt )
list(APPEND _IMPORT_CHECK_FILES_FOR_vt "${_IMPORT_PREFIX}/lib/libvt.so" )

# Import target "ar" for configuration "Release"
set_property(TARGET ar APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ar PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;plug;vt;${_IMPORT_PREFIX}/lib/libboost_python.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libar.so"
  IMPORTED_SONAME_RELEASE "libar.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS ar )
list(APPEND _IMPORT_CHECK_FILES_FOR_ar "${_IMPORT_PREFIX}/lib/libar.so" )

# Import target "kind" for configuration "Release"
set_property(TARGET kind APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(kind PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;plug"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libkind.so"
  IMPORTED_SONAME_RELEASE "libkind.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS kind )
list(APPEND _IMPORT_CHECK_FILES_FOR_kind "${_IMPORT_PREFIX}/lib/libkind.so" )

# Import target "sdf" for configuration "Release"
set_property(TARGET sdf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sdf PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;gf;trace;vt;work;ar;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libboost_regex.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libsdf.so"
  IMPORTED_SONAME_RELEASE "libsdf.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS sdf )
list(APPEND _IMPORT_CHECK_FILES_FOR_sdf "${_IMPORT_PREFIX}/lib/libsdf.so" )

# Import target "pcp" for configuration "Release"
set_property(TARGET pcp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pcp PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;trace;vt;sdf;work;ar;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpcp.so"
  IMPORTED_SONAME_RELEASE "libpcp.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS pcp )
list(APPEND _IMPORT_CHECK_FILES_FOR_pcp "${_IMPORT_PREFIX}/lib/libpcp.so" )

# Import target "usd" for configuration "Release"
set_property(TARGET usd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usd PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;kind;pcp;sdf;ar;plug;tf;trace;vt;work;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusd.so"
  IMPORTED_SONAME_RELEASE "libusd.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usd )
list(APPEND _IMPORT_CHECK_FILES_FOR_usd "${_IMPORT_PREFIX}/lib/libusd.so" )

# Import target "usdGeom" for configuration "Release"
set_property(TARGET usdGeom APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdGeom PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "js;tf;plug;vt;sdf;trace;usd;work;${_IMPORT_PREFIX}/lib/libboost_python.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdGeom.so"
  IMPORTED_SONAME_RELEASE "libusdGeom.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdGeom )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdGeom "${_IMPORT_PREFIX}/lib/libusdGeom.so" )

# Import target "usdLux" for configuration "Release"
set_property(TARGET usdLux APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdLux PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;vt;sdf;usd;usdGeom"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdLux.so"
  IMPORTED_SONAME_RELEASE "libusdLux.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdLux )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdLux "${_IMPORT_PREFIX}/lib/libusdLux.so" )

# Import target "usdShade" for configuration "Release"
set_property(TARGET usdShade APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdShade PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;vt;sdf;usd;usdGeom"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdShade.so"
  IMPORTED_SONAME_RELEASE "libusdShade.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdShade )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdShade "${_IMPORT_PREFIX}/lib/libusdShade.so" )

# Import target "usdHydra" for configuration "Release"
set_property(TARGET usdHydra APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdHydra PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;usd;usdShade"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdHydra.so"
  IMPORTED_SONAME_RELEASE "libusdHydra.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdHydra )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdHydra "${_IMPORT_PREFIX}/lib/libusdHydra.so" )

# Import target "usdRi" for configuration "Release"
set_property(TARGET usdRi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdRi PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;vt;sdf;usd;usdShade;usdGeom;usdLux;${_IMPORT_PREFIX}/lib/libboost_python.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdRi.so"
  IMPORTED_SONAME_RELEASE "libusdRi.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdRi )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdRi "${_IMPORT_PREFIX}/lib/libusdRi.so" )

# Import target "usdSkel" for configuration "Release"
set_property(TARGET usdSkel APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdSkel PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;gf;tf;trace;vt;work;sdf;usd;usdGeom;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdSkel.so"
  IMPORTED_SONAME_RELEASE "libusdSkel.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdSkel )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdSkel "${_IMPORT_PREFIX}/lib/libusdSkel.so" )

# Import target "usdUI" for configuration "Release"
set_property(TARGET usdUI APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdUI PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;vt;sdf;usd"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdUI.so"
  IMPORTED_SONAME_RELEASE "libusdUI.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdUI )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdUI "${_IMPORT_PREFIX}/lib/libusdUI.so" )

# Import target "usdUtils" for configuration "Release"
set_property(TARGET usdUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdUtils PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;gf;sdf;usd;usdGeom;${_IMPORT_PREFIX}/lib/libboost_python.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdUtils.so"
  IMPORTED_SONAME_RELEASE "libusdUtils.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdUtils "${_IMPORT_PREFIX}/lib/libusdUtils.so" )

# Import target "garch" for configuration "Release"
set_property(TARGET garch APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(garch PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;tf;${_IMPORT_PREFIX}/lib/libboost_system.so;"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libgarch.so"
  IMPORTED_SONAME_RELEASE "libgarch.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS garch )
list(APPEND _IMPORT_CHECK_FILES_FOR_garch "${_IMPORT_PREFIX}/lib/libgarch.so" )

# Import target "hf" for configuration "Release"
set_property(TARGET hf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hf PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "plug;tf;trace"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhf.so"
  IMPORTED_SONAME_RELEASE "libhf.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS hf )
list(APPEND _IMPORT_CHECK_FILES_FOR_hf "${_IMPORT_PREFIX}/lib/libhf.so" )

# Import target "cameraUtil" for configuration "Release"
set_property(TARGET cameraUtil APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(cameraUtil PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;gf"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcameraUtil.so"
  IMPORTED_SONAME_RELEASE "libcameraUtil.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS cameraUtil )
list(APPEND _IMPORT_CHECK_FILES_FOR_cameraUtil "${_IMPORT_PREFIX}/lib/libcameraUtil.so" )

# Import target "pxOsd" for configuration "Release"
set_property(TARGET pxOsd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pxOsd PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;gf;vt;${_IMPORT_PREFIX}/lib/libosdGPU.so;${_IMPORT_PREFIX}/lib/libosdCPU.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpxOsd.so"
  IMPORTED_SONAME_RELEASE "libpxOsd.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS pxOsd )
list(APPEND _IMPORT_CHECK_FILES_FOR_pxOsd "${_IMPORT_PREFIX}/lib/libpxOsd.so" )

# Import target "glf" for configuration "Release"
set_property(TARGET glf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(glf PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;garch;gf;hf;js;plug;tf;trace;sdf;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libboost_system.so;${_IMPORT_PREFIX}/lib/libOpenImageIO.so;${_IMPORT_PREFIX}/lib/libOpenImageIO_Util.so;${MAYA_LOCATION}/lib/libglew.so;"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libglf.so"
  IMPORTED_SONAME_RELEASE "libglf.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS glf )
list(APPEND _IMPORT_CHECK_FILES_FOR_glf "${_IMPORT_PREFIX}/lib/libglf.so" )

# Import target "hd" for configuration "Release"
set_property(TARGET hd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hd PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "plug;tf;trace;vt;work;sdf;hf;pxOsd;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhd.so"
  IMPORTED_SONAME_RELEASE "libhd.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS hd )
list(APPEND _IMPORT_CHECK_FILES_FOR_hd "${_IMPORT_PREFIX}/lib/libhd.so" )

# Import target "hdSt" for configuration "Release"
set_property(TARGET hdSt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hdSt PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "cameraUtil;garch;glf;hd;tf;trace;${MAYA_LOCATION}/lib/libglew.so;${_IMPORT_PREFIX}/lib/libosdGPU.so;${_IMPORT_PREFIX}/lib/libosdCPU.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhdSt.so"
  IMPORTED_SONAME_RELEASE "libhdSt.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS hdSt )
list(APPEND _IMPORT_CHECK_FILES_FOR_hdSt "${_IMPORT_PREFIX}/lib/libhdSt.so" )

# Import target "hdx" for configuration "Release"
set_property(TARGET hdx APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hdx PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "plug;tf;vt;gf;work;garch;glf;pxOsd;hd;hdSt;cameraUtil;sdf;${MAYA_LOCATION}/lib/libglew.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhdx.so"
  IMPORTED_SONAME_RELEASE "libhdx.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS hdx )
list(APPEND _IMPORT_CHECK_FILES_FOR_hdx "${_IMPORT_PREFIX}/lib/libhdx.so" )

# Import target "hdStream" for configuration "Release"
set_property(TARGET hdStream APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hdStream PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "plug;tf;trace;vt;work;hd;hdSt;hdx;${_IMPORT_PREFIX}/lib/libosdGPU.so;${_IMPORT_PREFIX}/lib/libosdCPU.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhdStream.so"
  IMPORTED_SONAME_RELEASE "libhdStream.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS hdStream )
list(APPEND _IMPORT_CHECK_FILES_FOR_hdStream "${_IMPORT_PREFIX}/lib/libhdStream.so" )

# Import target "usdImaging" for configuration "Release"
set_property(TARGET usdImaging APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdImaging PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "gf;tf;plug;trace;vt;work;garch;glf;hd;hdx;pxOsd;sdf;usd;usdGeom;usdHydra;usdLux;usdRi;usdShade;ar;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdImaging.so"
  IMPORTED_SONAME_RELEASE "libusdImaging.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdImaging )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdImaging "${_IMPORT_PREFIX}/lib/libusdImaging.so" )

# Import target "usdImagingGL" for configuration "Release"
set_property(TARGET usdImagingGL APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdImagingGL PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "gf;tf;plug;trace;vt;work;garch;glf;hd;hdx;pxOsd;sdf;usd;usdGeom;usdShade;usdHydra;usdImaging;ar;${_IMPORT_PREFIX}/lib/libboost_python.so;${MAYA_LOCATION}/lib/libglew.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdImagingGL.so"
  IMPORTED_SONAME_RELEASE "libusdImagingGL.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdImagingGL )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdImagingGL "${_IMPORT_PREFIX}/lib/libusdImagingGL.so" )

# Import target "usdviewq" for configuration "Release"
set_property(TARGET usdviewq APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdviewq PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;usd;usdGeom;${_IMPORT_PREFIX}/lib/libboost_python.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libusdviewq.so"
  IMPORTED_SONAME_RELEASE "libusdviewq.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdviewq )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdviewq "${_IMPORT_PREFIX}/lib/libusdviewq.so" )

# Import target "usdObj" for configuration "Release"
set_property(TARGET usdObj APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdObj PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;gf;sdf;usd;usdGeom"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/share/usd/examples/plugin/usdObj.so"
  IMPORTED_SONAME_RELEASE "usdObj.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdObj )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdObj "${_IMPORT_PREFIX}/share/usd/examples/plugin/usdObj.so" )

# Import target "usdSchemaExamples" for configuration "Release"
set_property(TARGET usdSchemaExamples APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdSchemaExamples PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "tf;sdf;usd;vt"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/share/usd/examples/plugin/usdSchemaExamples.so"
  IMPORTED_SONAME_RELEASE "usdSchemaExamples.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdSchemaExamples )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdSchemaExamples "${_IMPORT_PREFIX}/share/usd/examples/plugin/usdSchemaExamples.so" )

# Import target "px_vp20" for configuration "Release"
set_property(TARGET px_vp20 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(px_vp20 PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "garch;gf;glf;tf;${MAYA_LOCATION}/lib/libglew.so;${MAYA_LOCATION}/lib/libOpenMaya.so;${MAYA_LOCATION}/lib/libOpenMayaUI.so;${MAYA_LOCATION}/lib/libOpenMayaRender.so;"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/third_party/maya/lib/libpx_vp20.so"
  IMPORTED_SONAME_RELEASE "libpx_vp20.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS px_vp20 )
list(APPEND _IMPORT_CHECK_FILES_FOR_px_vp20 "${_IMPORT_PREFIX}/third_party/maya/lib/libpx_vp20.so" )

# Import target "pxrUsdMayaGL" for configuration "Release"
set_property(TARGET pxrUsdMayaGL APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pxrUsdMayaGL PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "arch;gf;glf;hd;hdSt;hdx;sdf;tf;usd;usdImaging;usdImagingGL;vt;px_vp20;usdMaya;${_IMPORT_PREFIX}/lib/libboost_date_time.so;${_IMPORT_PREFIX}/lib/libboost_filesystem.so;${_IMPORT_PREFIX}/lib/libboost_program_options.so;${_IMPORT_PREFIX}/lib/libboost_python.so;${_IMPORT_PREFIX}/lib/libboost_regex.so;${_IMPORT_PREFIX}/lib/libboost_system.so;${MAYA_LOCATION}/lib/libOpenMaya.so;${MAYA_LOCATION}/lib/libOpenMayaAnim.so;${MAYA_LOCATION}/lib/libOpenMayaFX.so;${MAYA_LOCATION}/lib/libOpenMayaRender.so;${MAYA_LOCATION}/lib/libOpenMayaUI.so;${MAYA_LOCATION}/lib/libImage.so;${MAYA_LOCATION}/lib/libFoundation.so;${MAYA_LOCATION}/lib/libIMFbase.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/third_party/maya/lib/libpxrUsdMayaGL.so"
  IMPORTED_SONAME_RELEASE "libpxrUsdMayaGL.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS pxrUsdMayaGL )
list(APPEND _IMPORT_CHECK_FILES_FOR_pxrUsdMayaGL "${_IMPORT_PREFIX}/third_party/maya/lib/libpxrUsdMayaGL.so" )

# Import target "usdMaya" for configuration "Release"
set_property(TARGET usdMaya APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(usdMaya PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "ar;gf;js;kind;plug;sdf;tf;usd;usdGeom;usdLux;usdRi;usdShade;usdUtils;vt;${_IMPORT_PREFIX}/lib/libboost_python.so;${MAYA_LOCATION}/lib/libFoundation.so;${MAYA_LOCATION}/lib/libOpenMaya.so;${MAYA_LOCATION}/lib/libOpenMayaAnim.so;${MAYA_LOCATION}/lib/libOpenMayaFX.so;${MAYA_LOCATION}/lib/libOpenMayaRender.so;${MAYA_LOCATION}/lib/libOpenMayaUI.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/third_party/maya/lib/libusdMaya.so"
  IMPORTED_SONAME_RELEASE "libusdMaya.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS usdMaya )
list(APPEND _IMPORT_CHECK_FILES_FOR_usdMaya "${_IMPORT_PREFIX}/third_party/maya/lib/libusdMaya.so" )

# Import target "pxrUsd" for configuration "Release"
set_property(TARGET pxrUsd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(pxrUsd PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "sdf;tf;usd;usdGeom;pxrUsdMayaGL;usdMaya;${MAYA_LOCATION}/lib/libOpenMaya.so;${MAYA_LOCATION}/lib/libOpenMayaAnim.so;${MAYA_LOCATION}/lib/libOpenMayaFX.so;${MAYA_LOCATION}/lib/libOpenMayaRender.so;${MAYA_LOCATION}/lib/libOpenMayaUI.so;${MAYA_LOCATION}/lib/libImage.so;${MAYA_LOCATION}/lib/libFoundation.so;${MAYA_LOCATION}/lib/libIMFbase.so;${_IMPORT_PREFIX}/lib/libtbb.so"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/third_party/maya/plugin/pxrUsd.so"
  IMPORTED_SONAME_RELEASE "pxrUsd.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS pxrUsd )
list(APPEND _IMPORT_CHECK_FILES_FOR_pxrUsd "${_IMPORT_PREFIX}/third_party/maya/plugin/pxrUsd.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
