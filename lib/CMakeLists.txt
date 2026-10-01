add_subdirectory(usdUfe)
# The layer editor's ui/ and python/ halves need Qt, which is only searched for
# when building MayaUsd. Skip the component in the UsdUfe standalone build;
# 3ds Max builds it from its own wrapper project.
if (Qt_FOUND)
    add_subdirectory(usdLayerEditor)
endif()
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(mayaUsd)
    add_subdirectory(usd)
endif()
if (BUILD_MAYAUSDAPI_LIBRARY)
    add_subdirectory(mayaUsdAPI)
endif()
if (BUILD_LOOKDEVXUSD_LIBRARY)
    add_subdirectory(lookdevXUsd)
endif()
