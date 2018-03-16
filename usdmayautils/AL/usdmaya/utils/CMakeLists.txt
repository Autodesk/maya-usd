
####################################################################################################
# Setup 
####################################################################################################

set(USDMAYA_UTILS_LIBRARY_NAME "AL_USDMayaUtils")
set(MAYA_UTILS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND usdmaya_utils_headers
    DgNodeHelper.h
    Utils.h
    ForwardDeclares.h
    MeshUtils.h
    SIMD.h
)

list(APPEND usdmaya_utils_source
    DgNodeHelper.cpp
    Utils.cpp
    MeshUtils.cpp
)

add_library(${USDMAYA_UTILS_LIBRARY_NAME}
    SHARED
        ${usdmaya_utils_source}
)

target_link_libraries(${USDMAYA_UTILS_LIBRARY_NAME}
  AL_USDUtils
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
  rt
  ${Boost_PYTHON_LIBRARY}
  ${PYTHON_LIBRARIES}
  ${MAYA_Foundation_LIBRARY}
  ${MAYA_OpenMaya_LIBRARY}
)

target_include_directories(${USDMAYA_UTILS_LIBRARY_NAME} 
    PUBLIC
    ${USDMAYAUTILS_INCLUDE_LOCATION}
)

# install the public headers
install( FILES ${usdmaya_utils_headers}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/usdmaya/utils
)

# install the library
install(TARGETS ${USDMAYA_UTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION}
)