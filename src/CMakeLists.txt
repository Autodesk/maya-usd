find_package(Maya REQUIRED)
find_package(HdMaya REQUIRED)
find_package(USD REQUIRED)
find_package(Boost REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(TBB REQUIRED COMPONENTS tbb)

link_directories(${MAYA_LIBRARY_DIR})
link_directories(${USD_LIBRARY_DIR})
link_directories(${HDMAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation)

set(HDMAYA_AL_SRC
    alProxyDelegate.cpp
)

add_library(hdmaya_al SHARED ${HDMAYA_AL_SRC})
target_link_libraries(hdmaya_al PUBLIC ${MAYA_LIBS} ${Boost_LIBRARIES} ${TBB_LIBRARIES})
target_link_libraries(hdmaya_al PUBLIC hdmaya)
target_include_directories(hdmaya_al PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${MAYA_INCLUDE_DIRS})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${USD_INCLUDE_DIR})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_include_directories(hdmaya_al SYSTEM PUBLIC ${HDMAYA_INCLUDE_DIR})
target_compile_features(hdmaya_al PUBLIC cxx_std_11)
target_compile_definitions(hdmaya_al PRIVATE "HDMAYA_AL_EXPORT=1")

set_target_properties(hdmaya_al PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)

install(TARGETS hdmaya_al
        DESTINATION lib)
