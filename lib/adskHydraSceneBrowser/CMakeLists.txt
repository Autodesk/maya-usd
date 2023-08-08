# -----------------------------------------------------------------------------
# setup Qt
# -----------------------------------------------------------------------------
if(DEFINED QT_LOCATION)
    if(NOT DEFINED QT_VERSION)
        set(QT_VERSION "5.12")
    endif()
    set(CMAKE_PREFIX_PATH "${QT_LOCATION}")
    find_package(Qt5 ${QT_VERSION} COMPONENTS Core Widgets REQUIRED)
    if(Qt5_FOUND)
        message(STATUS "QT_LOCATION set. Building adskHydraSceneBrowser enabled.")
    endif()
else()
    message(STATUS "QT_LOCATION not set. Building adskHydraSceneBrowser will be disabled.")
    return()
endif()

# -----------------------------------------------------------------------------
# setup adskHydraSceneBrowser library
# -----------------------------------------------------------------------------
set(TARGET_NAME adskHydraSceneBrowser)
add_library(${TARGET_NAME} SHARED api.h)

# -----------------------------------------------------------------------------
# setup sources
# -----------------------------------------------------------------------------
set(HEADERS
    dataSourceTreeWidget.h
    dataSourceValueTreeView.h
    registeredSceneIndexChooser.h
    sceneIndexDebuggerWidget.h
    sceneIndexObserverLoggingTreeView.h
    sceneIndexObserverLoggingWidget.h
    sceneIndexTreeWidget.h
)
set(SOURCES
    dataSourceTreeWidget.cpp
    dataSourceValueTreeView.cpp
    registeredSceneIndexChooser.cpp
    sceneIndexDebuggerWidget.cpp
    sceneIndexObserverLoggingTreeView.cpp
    sceneIndexObserverLoggingWidget.cpp
    sceneIndexTreeWidget.cpp
)

string(REGEX REPLACE "^[0-9]+\\.([0-9]+\\.[0-9]+)$" "\\1" USD_minor_patch_version ${USD_VERSION})
set(DOWNLOAD_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/src/download")
set(DLL_API_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/src/dll_api")

macro(download file)
    file(DOWNLOAD 
    "https://raw.githubusercontent.com/PixarAnimationStudios/OpenUSD/v${USD_minor_patch_version}/extras/imaging/examples/hdui/${file}" 
    "${DOWNLOAD_LOCATION}/${file}")
endmacro()

# Download source files
foreach(SOURCE IN ITEMS ${SOURCES})
    download(${SOURCE})
    # Copy source file to dll_api directory
    # Note: we intentionally copy over the sources to make the build folders easier 
    #       to understand and the debugging process easier.
    configure_file("${DOWNLOAD_LOCATION}/${SOURCE}" "${DLL_API_LOCATION}/${SOURCE}" COPYONLY)
endforeach()

foreach(HEADER IN ${HEADERS})
    # Download header files
    download(${HEADER})
    
    # Add DLL import/export support for Windows 
    # Insert 'HDUI_API' symbols for class declarations
    # Note: we assume class declarations are always followed by double colons
    file(READ "${DOWNLOAD_LOCATION}/${HEADER}" FILE_CONTENTS)
    string(REGEX REPLACE "class ([a-zA-Z]+[^\;]:)" "class HDUI_API \\1" FILE_CONTENTS "${FILE_CONTENTS}")
    
    # Insert '#include <api.h>' above the first occurrence of "#include"
    # Note: we assume there is always at least one include statement in header files
    string(FIND "${FILE_CONTENTS}" "#include" include_pos)
    string(SUBSTRING "${FILE_CONTENTS}" 0 ${include_pos} include_line)
    string(SUBSTRING "${FILE_CONTENTS}" ${include_pos} -1 remaining_contents)
    set(modified_contents "${include_line}\n#include <api.h>\n${remaining_contents}")
    
    file(WRITE "${DLL_API_LOCATION}/${HEADER}" "${modified_contents}")
endforeach()

# Prepend an absolute path to the downloaded source files, located in 
# the CMAKE_CURRENT_BINARY_DIR.
# Note: the sources are downloaded for PixarAnimationStudios/OpenUSD and 
#       are not located in this repository's folder
list(TRANSFORM SOURCES PREPEND "${DLL_API_LOCATION}/")

# Add CMAKE_CURRENT_BINARY_DIR downloaded sources to the adskHydraSceneBrowser target
target_sources(${TARGET_NAME}
    PRIVATE
        ${SOURCES}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${PXR_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DLL_API_LOCATION}
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
# QT_NO_KEYWORDS prevents Qt from defining the foreach, signals, slots and emit macros.
# this avoids overlap between Qt macros and boost, and enforces using Q_ macros.
set_target_properties(Qt5::Core PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)

target_compile_definitions(${TARGET_NAME} 
    PRIVATE 
        HDUI_EXPORT
)
mayaUsd_compile_config(${TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME} 
    PUBLIC
        usd
        usdImaging
        Qt5::Core
        Qt5::Widgets
)

# -----------------------------------------------------------------------------
# Qt file processing (moc)
# -----------------------------------------------------------------------------
set_property(TARGET adskHydraSceneBrowser PROPERTY AUTOMOC ON)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

list(TRANSFORM HEADERS PREPEND "${DLL_API_LOCATION}/")
install(FILES ${HEADERS}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/include/adskHydraSceneBrowser
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()