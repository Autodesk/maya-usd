find_package(Maya REQUIRED)
find_package(HdMaya REQUIRED)
find_package(USD REQUIRED)
if(NOT USD_MAYA_INCLUDE_DIR OR NOT USD_MAYA_LIBRARY_DIR) 
    message(FATAL "Could not find usdMaya - aborting")
endif(NOT USD_MAYA_INCLUDE_DIR OR NOT USD_MAYA_LIBRARY_DIR) 
find_package(ALUSDMaya REQUIRED)
find_package(Boost REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(TBB REQUIRED COMPONENTS tbb)

link_directories(${MAYA_LIBRARY_DIR})
link_directories(${USD_LIBRARY_DIR} ${USD_MAYA_LIBRARY_DIR})
link_directories(${HDMAYA_LIBRARY_DIR})
link_directories(${ALUSDMAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation)

set(HDMAYA_AL_SRC
    alProxyDelegate.cpp
    debugCodes.cpp
)

add_library(hdmaya_al SHARED ${HDMAYA_AL_SRC})
target_link_libraries(hdmaya_al PUBLIC ${MAYA_LIBS} ${Boost_LIBRARIES} ${TBB_LIBRARIES})
target_link_libraries(hdmaya_al PUBLIC usdMaya)
target_link_libraries(hdmaya_al PUBLIC hdmaya)
target_link_libraries(hdmaya_al PUBLIC AL_USDMaya AL_USDMayaUtils AL_MayaUtils AL_USDUtils AL_EventSystem)
target_include_directories(hdmaya_al PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${MAYA_INCLUDE_DIRS})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${USD_INCLUDE_DIR} ${USD_MAYA_INCLUDE_DIR})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${HDMAYA_INCLUDE_DIR})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${ALUSDMAYA_INCLUDE_DIR})
target_compile_features(hdmaya_al PUBLIC cxx_std_11)
target_compile_definitions(hdmaya_al PRIVATE "HDMAYA_AL_EXPORT=1")


# Wanted to do this:
#   get_target_property(LIB_FULLPATH hdmaya_al LOCATION)
#   get_filename_component(LIB_NAME ${LIB_FULLPATH} NAME)
#   set(PLUG_INFO_LIBRARY_PATH ${LIB_NAME})
# But apparently you can't look up LOCATION in cmake-2.8.0... nor can you use
# the generator expression "$<TARGET_FILE_NAME:hdmaya_al>" outside of special
# commands, like add_custom_command...
# So just building "manually" with PREFIX / SUFFIX
get_target_property(LIB_PREFIX hdmaya_al PREFIX)
if (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)
    set(LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
endif (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)

get_target_property(LIB_SUFFIX hdmaya_al SUFFIX)
if (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)

set(PLUG_INFO_LIBRARY_PATH ${LIB_PREFIX}hdmaya_al${LIB_SUFFIX})

set(PLUG_INFO_PATH "plugInfo.json")


configure_file(
    ${PLUG_INFO_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
)

set_target_properties(hdmaya_al PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)

install(TARGETS hdmaya_al
        DESTINATION lib)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
        DESTINATION lib)