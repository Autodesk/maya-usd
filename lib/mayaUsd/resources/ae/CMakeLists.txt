# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------
if (CMAKE_UFE_V2_FEATURES_AVAILABLE)
    # We need an empty __init__.py file in each subfolder so it is considered a python module.
    install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae)
    install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
    install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes)

    # Maya Attribute Editor python template files
    foreach(_SUBDIR base camera capsule cone cube cylinder material mesh scope shader sphere xform)
        install(FILES __init__.py ${_SUBDIR}/ae_template.py
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes/${_SUBDIR}
        )
    endforeach()
endif()

install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME})
