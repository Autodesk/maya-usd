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
# UFE_CLIPBOARD_SUPPORT     Presence of UFE clipboard support
# UFE_DEFAULT_VALUE_SUPPORT Presence of UFE default value support
# UFE_HAS_UNSIGNED_INT      Presence of UFE unsigned int support
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

    set(UFE_VOLUME_LIGHTS_SUPPORT FALSE CACHE INTERNAL "ufeVolumeLights")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/light.h UFE_HAS_API REGEX "VolumeProps")
    if(UFE_HAS_API)
        set(UFE_VOLUME_LIGHTS_SUPPORT TRUE CACHE INTERNAL "ufeVolumeLights")
        message(STATUS "Maya has UFE VolumeLights API")
    endif()
endif()

set(UFE_MATERIALS_SUPPORT FALSE CACHE INTERNAL "ufeMaterials")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/material.h")
    set(UFE_MATERIALS_SUPPORT TRUE CACHE INTERNAL "ufeMaterials")
    message(STATUS "Maya has UFE materials API")

    set(UFE_MATERIAL_HAS_HASMATERIAL FALSE CACHE INTERNAL "ufeMaterialHashasMaterial")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/material.h UFE_HAS_API REGEX "hasMaterial")
    if(UFE_HAS_API)
        set(UFE_MATERIAL_HAS_HASMATERIAL TRUE CACHE INTERNAL "ufeMaterialHashasMaterial")
        message(STATUS "Maya UFE Material interface has hasMaterial method")
    endif()
endif()

set(UFE_SCENE_SEGMENT_SUPPORT FALSE CACHE INTERNAL "ufeSceneSegment")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/sceneSegmentHandler.h")
    set(UFE_SCENE_SEGMENT_SUPPORT TRUE CACHE INTERNAL "ufeSceneSegment")
    message(STATUS "Maya has UFE scene segment API")
endif()

set(UFE_HAS_DISPLAY_NAME FALSE CACHE INTERNAL "ufeHasDisplayName")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/attribute.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/attribute.h UFE_HAS_API REGEX "displayName")
    if(UFE_HAS_API)
        set(UFE_HAS_DISPLAY_NAME TRUE CACHE INTERNAL "ufeHasDisplayName")
        message(STATUS "Maya has UFE Attribute displayName")
    endif()
endif()

set(UFE_DEFAULT_VALUE_SUPPORT FALSE CACHE INTERNAL "ufeHasDefaultValue")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/attribute.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/attribute.h UFE_HAS_API REGEX "isDefault")
    if(UFE_HAS_API)
        set(UFE_DEFAULT_VALUE_SUPPORT TRUE CACHE INTERNAL "ufeHasDefaultValue")
        message(STATUS "Maya has UFE Attribute default value support")
    endif()
endif()

set(UFE_HAS_UNSIGNED_INT FALSE CACHE INTERNAL "ufeHasUnsignedInt")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/attribute.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/attribute.h UFE_HAS_API REGEX "AttributeUInt")
    if(UFE_HAS_API)
        set(UFE_HAS_UNSIGNED_INT TRUE CACHE INTERNAL "ufeHasUnsignedInt")
        message(STATUS "Maya has UFE Attribute type Unsigned Int")
    endif()
endif()

set(UFE_HAS_NATIVE_TYPE_METADATA FALSE CACHE INTERNAL "ufeHasNativeTypeMetadata")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/attributeDef.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/attributeDef.h UFE_HAS_API REGEX "NativeType")
    if(UFE_HAS_API)
        set(UFE_HAS_NATIVE_TYPE_METADATA TRUE CACHE INTERNAL "ufeHasNativeTypeMetadata")
        message(STATUS "Maya has UFE AttributeDef 'NativeType' metadata")
    endif()
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

set(UFE_SCENEITEM_HAS_METADATA FALSE CACHE INTERNAL "getMetadata")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/sceneItem.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/sceneItem.h UFE_HAS_API REGEX "getMetadata")
    if(UFE_HAS_API)
        set(UFE_SCENEITEM_HAS_METADATA TRUE CACHE INTERNAL "ufeSceneItemHasMetaData")
        message(STATUS "Maya has UFE SceneItem's meta data interface")
    endif()
endif()

set(UFE_CONTEXTOPS_HAS_OPTIONBOX FALSE CACHE INTERNAL "kIsOptionBox")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/contextOps.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/contextOps.h UFE_HAS_API REGEX "kIsOptionBox")
    if(UFE_HAS_API)
        set(UFE_CONTEXTOPS_HAS_OPTIONBOX TRUE CACHE INTERNAL "kIsOptionBox")
        message(STATUS "Maya UFE ContextItem has OptionBox meta data")
    endif()
endif()

set(UFE_CAMERA_HAS_RENDERABLE FALSE CACHE INTERNAL "ufeCameraHasRendereable")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/camera.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/camera.h UFE_HAS_API REGEX "renderable")
    if(UFE_HAS_API)
        set(UFE_CAMERA_HAS_RENDERABLE TRUE CACHE INTERNAL "ufeCameraHasRendereable")
        message(STATUS "Maya has UFE Camera renderable interface")
    endif()
endif()

set(UFE_CLIPBOARD_SUPPORT FALSE CACHE INTERNAL "ufeClipboard")
if (UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/clipboardHandler.h")
    set(UFE_CLIPBOARD_SUPPORT TRUE CACHE INTERNAL "ufeClipboard")
    message(STATUS "Maya has UFE clipboard API")
endif()

set(UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH FALSE CACHE INTERNAL "rootSceneSegmentRootPath")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/sceneSegmentHandler.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/sceneSegmentHandler.h UFE_HAS_API REGEX "rootSceneSegmentRootPath")
    if(UFE_HAS_API)
        set(UFE_SCENE_SEGMENT_HANDLER_ROOT_PATH TRUE CACHE INTERNAL "rootSceneSegmentRootPath")
        message(STATUS "Maya has UFE SceneSegmentHandler's rootSceneSegmentRootPath interface")
    endif()
endif()

set(UFE_CAMERAHANDLER_HAS_FINDALL FALSE CACHE INTERNAL "findAll")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/cameraHandler.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/cameraHandler.h UFE_HAS_API REGEX "findAll")
    if(UFE_HAS_API)
        set(UFE_CAMERAHANDLER_HAS_FINDALL TRUE CACHE INTERNAL "ufeCameraHandlerHasFindAll")
        message(STATUS "Maya has UFE CameraHandler's findAll interface")
    endif()
endif()

set(UFE_CAMERA_HAS_COMPUTEDVISIBILITY FALSE CACHE INTERNAL "ufeCameraHasComputedVisiblity")
if(UFE_INCLUDE_DIR AND EXISTS "${UFE_INCLUDE_DIR}/ufe/camera.h")
    file(STRINGS ${UFE_INCLUDE_DIR}/ufe/camera.h UFE_HAS_API REGEX "computedVisibility")
    if(UFE_HAS_API)
        set(UFE_CAMERA_HAS_COMPUTEDVISIBILITY TRUE CACHE INTERNAL "ufeCameraHasComputedVisiblity")
        message(STATUS "Maya has UFE Camera's computed visibility interface")
    endif()
endif()