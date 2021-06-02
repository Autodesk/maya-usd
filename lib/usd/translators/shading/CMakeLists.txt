# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        usdBlinnReader.cpp
        usdBlinnWriter.cpp
        usdDisplacementShaderWriter.cpp
        usdFileTextureWriter.cpp
        usdLambertReader.cpp
        usdLambertWriter.cpp
        usdMaterialReader.cpp
        usdMaterialWriter.cpp
        usdPhongReader.cpp
        usdPhongWriter.cpp
        usdPhongEWriter.cpp
        usdReflectWriter.cpp
        usdUVTextureReader.cpp
)

if (BUILD_RFM_TRANSLATORS)
    target_sources(${TARGET_NAME}
        PRIVATE
            rfmShaderTranslation.cpp
    )
endif()

if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2020)
    target_sources(${TARGET_NAME}
        PRIVATE
            usdStandardSurfaceReader.cpp
            usdStandardSurfaceWriter.cpp
    )
endif()

if (MAYA_APP_VERSION VERSION_GREATER 2022 AND PXR_VERSION VERSION_GREATER 2102)
    target_sources(${TARGET_NAME}
        PRIVATE
            mtlxBaseWriter.cpp
            mtlxStandardSurfaceWriter.cpp
            mtlxFileTextureWriter.cpp
    )
endif()
