if(UFE_FOUND)
    add_subdirectory(usdUfe)
endif()

if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(mayaUsd)
    add_subdirectory(usd)
endif()
