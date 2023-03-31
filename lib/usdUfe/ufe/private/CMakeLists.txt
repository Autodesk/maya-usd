# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        UfeNotifGuard.cpp
        Utils.cpp
)


# TEMP (UsdUfe)
#
# Temporarily promote the private headers during the UsdUfe transition.
# This is so the files that are still in mayaUsd/ufe can access them.
# Once everything is moved we should be able to remove this promote.
#
# Note: intentionally placed into "mayaUsd/private" folder.
#
set(HEADERS
    UfeNotifGuard.h
    Utils.h
)
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} BASEDIR ${PROJECT_NAME} SUBDIR mayaUsd/private)
