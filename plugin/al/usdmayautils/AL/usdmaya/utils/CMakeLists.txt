
####################################################################################################
# Setup 
####################################################################################################

set(USDMAYA_UTILS_LIBRARY_NAME "AL_USDMayaUtils")
set(MAYA_UTILS_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND usdmaya_utils_headers
    AnimationTranslator.h
    Api.h
    AttributeType.h
    DgNodeHelper.h
    Utils.h
    ForwardDeclares.h
    MeshUtils.h
    NurbsCurveUtils.h
    DiffPrimVar.h
)

list(APPEND usdmaya_utils_source
    AnimationTranslator.cpp
    AttributeType.cpp
    DgNodeHelper.cpp
    Utils.cpp
    MeshUtils.cpp
    NurbsCurveUtils.cpp
    DiffPrimVar.cpp
)

add_library(${USDMAYA_UTILS_LIBRARY_NAME}
    SHARED
        ${usdmaya_utils_source}
)

# compiler configuration
mayaUsd_compile_config(${USDMAYA_UTILS_LIBRARY_NAME})

target_compile_definitions(${USDMAYA_UTILS_LIBRARY_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        AL_USDMAYA_UTILS_EXPORT
)

target_link_libraries(${USDMAYA_UTILS_LIBRARY_NAME}
  mayaUsdUtils
  AL_MayaUtils
  ar 
  gf 
  kind
  plug 
  sdf 
  tf
  usd
  usdGeom
  usdUtils
  vt
  ${Boost_PYTHON_LIBRARY}
  ${PYTHON_LIBRARIES}
  ${MAYA_Foundation_LIBRARY}
  ${MAYA_OpenMaya_LIBRARY}
)

target_include_directories(${USDMAYA_UTILS_LIBRARY_NAME} 
    PUBLIC
    ${USDMAYAUTILS_INCLUDE_LOCATION}
    ${USD_INCLUDE_DIR}
)

# install the public headers
install( FILES ${usdmaya_utils_headers}
        DESTINATION ${AL_INSTALL_PREFIX}/include/AL/usdmaya/utils
)

# install the library
install(TARGETS ${USDMAYA_UTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${USDMAYA_UTILS_LIBRARY_NAME}> DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION} OPTIONAL)
endif()
