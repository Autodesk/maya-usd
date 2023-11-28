# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

# We need an empty __init__.py file in each subfolder so it is considered a python module.
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae)
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
install(FILES ae_utils.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes)

# Maya Attribute Editor python template - just uses a single schema base template file.
set(MAYAUSD_AE_TEMPLATES usdschemabase)

foreach(_SUBDIR ${MAYAUSD_AE_TEMPLATES})
    install(FILES
        ${_SUBDIR}/__init__.py
        ${_SUBDIR}/ae_template.py
        ${_SUBDIR}/ae_auto_template.py
        ${_SUBDIR}/array_custom_control.py
        ${_SUBDIR}/attribute_custom_control.py
        ${_SUBDIR}/connections_custom_control.py
        ${_SUBDIR}/enum_custom_control.py
        ${_SUBDIR}/image_custom_control.py
        ${_SUBDIR}/metadata_custom_control.py
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes/${_SUBDIR}
    )
endforeach()

install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME})
