set(PLUGIN_PACKAGE mayaUsdPlugin)

if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

add_library(${PLUGIN_PACKAGE} MODULE
    plugin.cpp
    importTranslator.cpp
    ProxyShape.cpp
)

target_compile_definitions(${PLUGIN_PACKAGE}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
)

target_include_directories(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_INCLUDE_DIRS}
        ${MAYAUSD_INCLUDE_DIR}
        ${PXR_INCLUDE_DIRS}
)

target_link_libraries(${PLUGIN_PACKAGE}
    PRIVATE
        ${MAYA_LIBRARIES}
        ${MAYAUSD_LIBRARIES}
		ar 
		gf 
		kind
		plug 
		sdf 
		tf
		usd
		usdGeom
		usdUtils
)

if(IS_WINDOWS)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES SUFFIX ".mll")
elseif(IS_MACOSX)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES PREFIX "" SUFFIX ".bundle")
elseif(IS_LINUX)
    set_target_properties(${PLUGIN_PACKAGE} PROPERTIES PREFIX "")
endif()

# rpath setup
if(IS_MACOSX OR IS_LINUX) 
    init_rpath(rpath "plugin")
    add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    install_rpath(rpath ${PLUGIN_PACKAGE})
endif()

install(TARGETS
    ${PLUGIN_PACKAGE}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PLUGIN_PACKAGE}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()

