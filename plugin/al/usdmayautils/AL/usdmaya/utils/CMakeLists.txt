
####################################################################################################
# Setup 
####################################################################################################

set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX} )

set(USDMAYA_UTILS_LIBRARY_NAME "AL_USDMayaUtils")
set(MAYA_UTILS_LIBRARY_LOCATION ${CMAKE_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND usdmaya_utils_headers
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

target_compile_definitions(${USDMAYA_UTILS_LIBRARY_NAME}
    PRIVATE
        AL_USDMAYA_UTILS_EXPORT
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
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/AL/usdmaya/utils
)

# install the library
install(TARGETS ${USDMAYA_UTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION}
)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${USDMAYA_UTILS_LIBRARY_NAME}> DESTINATION ${MAYA_UTILS_LIBRARY_LOCATION} OPTIONAL)
endif()
