#
# Simple module to find UFE.
#
# This module searches for a valid UFE installation.
# It searches for UFE's libraries and include header files.
#
# Variables that will be defined:
# UFE_FOUND                 Defined if a UFE installation has been detected
# UFE_LIBRARY               Path to UFE library
# UFE_INCLUDE_DIR           Path to the UFE include directory
# UFE_VERSION               UFE version (major.minor.patch) from ufe.h
# UFE_LIGHTS_SUPPORT        Presence of UFE lights support
# UFE_MATERIALS_SUPPORT     Presence of UFE materials support
# UFE_SCENE_SEGMENT_SUPPORT Presence of UFE scene segment support
# UFE_PREVIEW_FEATURES      List of all features introduced gradually in the UFE preview version
#

find_path(UFE_INCLUDE_DIR
        ufe/versionInfo.h
    HINTS
        $ENV{UFE_INCLUDE_ROOT}
        ${UFE_INCLUDE_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        $ENV{MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATH_SUFFIXES
        devkit/ufe/include
        include/
    DOC
        "UFE header path"
)

# Get the UFE_VERSION and features from ufe.h
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/ufe.h")
    # Parse the file and get the three lines that have the version info.
    file(STRINGS
        "${UFE_INCLUDE_DIR}/ufe/ufe.h"
        _ufe_vers
        REGEX "#define[ ]+(UFE_MAJOR_VERSION|UFE_MINOR_VERSION|UFE_PATCH_LEVEL)[ ]+[0-9]+$")

    # Then extract the number from each one.
    foreach(_ufe_tmp ${_ufe_vers})
        if(_ufe_tmp MATCHES "#define[ ]+(UFE_MAJOR_VERSION|UFE_MINOR_VERSION|UFE_PATCH_LEVEL)[ ]+([0-9]+)$")
            set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
        endif()
    endforeach()
    set(UFE_VERSION ${UFE_MAJOR_VERSION}.${UFE_MINOR_VERSION}.${UFE_PATCH_LEVEL})

    if(UFE_MAJOR_VERSION VERSION_EQUAL "0")
        math(EXPR UFE_PREVIEW_VERSION_NUM "${UFE_MINOR_VERSION} * 1000 + ${UFE_PATCH_LEVEL}")
    elseif(UFE_VERSION VERSION_EQUAL "4.1.0")
        # Temporary. Once next Maya is released with UFE v4.1.0 this should
        # be removed (along with all the UFE_PREVIEW_VERSION_NUM checks).
        set(UFE_PREVIEW_VERSION_NUM 4100)
    elseif(UFE_VERSION VERSION_EQUAL "4.2.0")
        # Temporary. Once next Maya is released with UFE v4.2.0 this should
        # be removed (along with all the UFE_PREVIEW_VERSION_NUM checks).
        set(UFE_PREVIEW_VERSION_NUM 4202)
    elseif((UFE_VERSION VERSION_GREATER_EQUAL "4.2.300") AND (UFE_VERSION VERSION_LESS "5.0.0"))
        # Temporary - will only be used if there is development for a Ufe v4.3.0 to create
        # the UFE_PREVIEW_VERSION_NUM for that version while it is in development.
        # Note: the UFE_PATCH_LEVEL will start at 300 and will thus encode the minor version
        #       of 3 (so we don't use UFE_MINOR_VERSION in this formula).
        math(EXPR UFE_PREVIEW_VERSION_NUM "4 * 1000 + ${UFE_PATCH_LEVEL}")
    endif()

    file(STRINGS
        "${UFE_INCLUDE_DIR}/ufe/ufe.h"
        _ufe_features
        REGEX "#define UFE_V[0-9]+(_[0-9]+)?_FEATURES_AVAILABLE$")
    foreach(_ufe_tmp ${_ufe_features})
        if(_ufe_tmp MATCHES "#define UFE_V([0-9]+(_[0-9]+)?)_FEATURES_AVAILABLE$")
            set(CMAKE_UFE_V${CMAKE_MATCH_1}_FEATURES_AVAILABLE ON)
        endif()
    endforeach()
endif()

find_library(UFE_LIBRARY
    NAMES
        ufe_${UFE_MAJOR_VERSION}
    HINTS
        $ENV{UFE_LIB_ROOT}
        ${UFE_LIB_ROOT}
        ${MAYA_DEVKIT_LOCATION}
        $ENV{MAYA_DEVKIT_LOCATION}
        ${MAYA_LOCATION}
        $ENV{MAYA_LOCATION}
        ${MAYA_BASE_DIR}
    PATHS
        ${UFE_LIBRARY_DIR}
    PATH_SUFFIXES
        devkit/ufe/lib
        lib/
    DOC
        "UFE library"
    NO_DEFAULT_PATH
)

# Gather all preview features that might be there or not into a single list:
list(APPEND UFE_PREVIEW_FEATURES ufe)

if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/batchOpsHandler.h")
    list(APPEND UFE_PREVIEW_FEATURES v4_BatchOps)
endif()

if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/codeWrapperHandler.h")
    list(APPEND UFE_PREVIEW_FEATURES CodeWrapperHandler)
    message(STATUS "Maya has UFE code wrapper API")
endif()

# Handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(UFE
    REQUIRED_VARS
        UFE_INCLUDE_DIR
        UFE_LIBRARY
        UFE_PREVIEW_FEATURES
    VERSION_VAR
        UFE_VERSION
)

set(UFE_LIGHTS_SUPPORT FALSE CACHE INTERNAL "ufeLights")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/lightHandler.h")
    set(UFE_LIGHTS_SUPPORT TRUE CACHE INTERNAL "ufeLights")
    message(STATUS "Maya has UFE lights API")
endif()

set(UFE_MATERIALS_SUPPORT FALSE CACHE INTERNAL "ufeMaterials")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/material.h")
    set(UFE_MATERIALS_SUPPORT TRUE CACHE INTERNAL "ufeMaterials")
    message(STATUS "Maya has UFE materials API")
endif()

set(UFE_SCENE_SEGMENT_SUPPORT FALSE CACHE INTERNAL "ufeSceneSegment")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/sceneSegmentHandler.h")
    set(UFE_SCENE_SEGMENT_SUPPORT TRUE CACHE INTERNAL "ufeSceneSegment")
    message(STATUS "Maya has UFE scene segment API")
endif()

set(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR FALSE CACHE INTERNAL "ufeTrieNodeHasChildrenComponentsAccessor")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/trie.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/trie.h UFE_HAS_API REGEX "childrenComponents")
    if(UFE_HAS_API)
        set(UFE_TRIE_NODE_HAS_CHILDREN_COMPONENTS_ACCESSOR TRUE CACHE INTERNAL "ufeTrieNodeHasChildrenComponentsAccessor")
        message(STATUS "Maya has UFE TrieNode childrenComponents accessor")
    endif()
endif()

set(UFE_UINODEGRAPHNODE_HAS_SIZE FALSE CACHE INTERNAL "ufeUINodeGraphNodeHasSize")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/uiNodeGraphNode.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/uiNodeGraphNode.h UFE_HAS_API REGEX "getSize")
    if(UFE_HAS_API)
        set(UFE_UINODEGRAPHNODE_HAS_SIZE TRUE CACHE INTERNAL "ufeUINodeGraphNodeHasSize")
        message(STATUS "Maya has UFE UINodeGraphNode size interface")
    endif()
endif()

set(UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR FALSE CACHE INTERNAL "ufeUINodeGraphNodeHasDisplayColor")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/uiNodeGraphNode.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/uiNodeGraphNode.h UFE_HAS_API REGEX "getDisplayColor")
    if(UFE_HAS_API)
        set(UFE_UINODEGRAPHNODE_HAS_DISPLAYCOLOR TRUE CACHE INTERNAL "ufeUINodeGraphNodeHasDisplayColor")
        message(STATUS "Maya has UFE UINodeGraphNode display color interface")
    endif()
endif()

set(UFE_ATTRIBUTES_GET_ENUMS FALSE CACHE INTERNAL "ufeAttributesGetEnums")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/attributes.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/attributes.h UFE_HAS_API REGEX "getEnums")
    if(UFE_HAS_API)
        set(UFE_ATTRIBUTES_GET_ENUMS TRUE CACHE INTERNAL "ufeAttributesGetEnums")
        message(STATUS "Maya has UFE Attribute's getEnums interface")
    endif()
endif()

