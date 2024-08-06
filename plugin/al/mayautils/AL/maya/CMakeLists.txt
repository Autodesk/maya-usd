
####################################################################################################
# Setup 
####################################################################################################

set(MAYAUTILS_LIBRARY_NAME "AL_MayaUtils")
set(MAYAUTILS_LIBRARY_LOCATION ${AL_INSTALL_PREFIX}/lib) 

####################################################################################################
# Source 
####################################################################################################

list(APPEND maya_utils_headers
    utils/Api.h
    utils/CommandGuiHelper.h
    utils/FileTranslatorBase.h
    utils/FileTranslatorOptions.h
    utils/ForwardDeclares.h
    utils/MayaHelperMacros.h
    utils/MenuBuilder.h
    utils/MObjectMap.h
    utils/NodeHelper.h
    utils/Utils.h
    utils/DebugCodes.h
    utils/PluginTranslatorOptions.h
)

list(APPEND maya_event_headers
    event/MayaEventManager.h
    event/Api.h
)

list(APPEND maya_source
    utils/FileTranslatorOptions.cpp
    utils/MenuBuilder.cpp
    utils/NodeHelper.cpp
    utils/CommandGuiHelper.cpp
    utils/PluginTranslatorOptions.cpp
    utils/Utils.cpp
    event/MayaEventManager.cpp
)

add_library(${MAYAUTILS_LIBRARY_NAME}
    SHARED
        ${maya_source}
)

# compiler configuration
mayaUsd_compile_config(${MAYAUTILS_LIBRARY_NAME})

target_compile_definitions(${MAYAUTILS_LIBRARY_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        AL_MAYA_EVENTS_EXPORT
        AL_MAYA_UTILS_EXPORT
        AL_MAYA_MACROS_EXPORT
)

target_include_directories(${MAYAUTILS_LIBRARY_NAME}
    PUBLIC 
    ${MAYAUTILS_INCLUDE_LOCATION}
    ${MAYA_INCLUDE_DIRS}
    )

target_link_libraries(${MAYAUTILS_LIBRARY_NAME}
  AL_EventSystem
  ${MAYA_Foundation_LIBRARY}
  ${MAYA_OpenMaya_LIBRARY}
  ${MAYA_OpenMayaAnim_LIBRARY}
  ${MAYA_OpenMayaUI_LIBRARY}
  usdUfe
)

####################################################################################################
# Install
####################################################################################################
 
install( FILES ${maya_utils_headers}
        DESTINATION ${AL_INSTALL_PREFIX}/include/AL/maya/utils
)

install( FILES ${maya_event_headers}
        DESTINATION ${AL_INSTALL_PREFIX}/include/AL/maya/event
)

install(TARGETS ${MAYAUTILS_LIBRARY_NAME}
    LIBRARY
    DESTINATION ${MAYAUTILS_LIBRARY_LOCATION}
    RUNTIME
    DESTINATION ${MAYAUTILS_LIBRARY_LOCATION}
)
if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${MAYAUTILS_LIBRARY_NAME}> DESTINATION ${MAYAUTILS_LIBRARY_LOCATION} OPTIONAL)
endif()

if(NOT SKIP_USDMAYA_TESTS)
  add_subdirectory(tests)
endif()
