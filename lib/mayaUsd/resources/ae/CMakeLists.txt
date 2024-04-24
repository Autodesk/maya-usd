# ---------------------------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------------------------

# In Python 2 we need an __init__.py file in the ufe_ae folder to declare it as a module.
# In Python 3 we must not use an __init__.py file in that folder to enable the "Implicit Namespace
# Packages" feature as defined in Python PEP 420, which allows other plugins to also be able to
# declare an ufe_ae module that would be merged with this one. See https://peps.python.org/pep-0420/
# for details.
if (NOT BUILD_WITH_PYTHON_3)
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae)
endif()
# We need an empty __init__.py file in each subfolder so it is considered a python module.
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
install(FILES ae_utils.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd)
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes)

# Maya Attribute Editor python template - just uses a single schema base template file.
set(MAYAUSD_AE_TEMPLATES usdschemabase)

foreach(_SUBDIR ${MAYAUSD_AE_TEMPLATES})
    install(FILES
        ${_SUBDIR}/__init__.py
        ${_SUBDIR}/custom_image_control.py
        ${_SUBDIR}/custom_enum_control.py
        ${_SUBDIR}/ae_template.py
        ${_SUBDIR}/attribute_custom_control.py
        ${_SUBDIR}/material_custom_control.py
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/ufe_ae/usd/nodes/${_SUBDIR}
    )
endforeach()

install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME})
