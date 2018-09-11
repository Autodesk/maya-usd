# In case we need boost, USD still needs the dynamic ones.
# This is the default value.
# set(Boost_USE_STATIC_LIBS OFF)

find_package(Maya)
find_package(OpenGL REQUIRED)
find_package(USD REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)

link_directories(${USD_LIBRARY_DIR})
link_directories(${USD_MAYA_LIBRARY_DIR})

set(MAYA_LIBS
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaUI_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY})

add_subdirectory(hdmaya)
add_subdirectory(hdview)
