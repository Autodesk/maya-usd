set(TARGET_NAME holdoutSpike)

add_library(${TARGET_NAME} MODULE)

target_sources(${TARGET_NAME}
    PRIVATE
        holdoutRenderOverrideSpike.cpp
)

mayaUsd_compile_config(${TARGET_NAME})
maya_set_plugin_properties(${TARGET_NAME})

target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd          # brings in Maya render libs + USD garch transitively
)

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/holdout/plugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/plugin/holdout/plugin
)