# Currently, we only have 2019/Ufe scripts...

if(NOT MAYA_VERSION LESS 2019 AND HDMAYA_UFE_BUILD)
    install(FILES
            mtoh_al/__init__.py
            mtoh_al/opinionEditor.py
            DESTINATION python/mtoh_al)
endif()
