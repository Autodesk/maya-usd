# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        shadingTokens.cpp
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
            mtlxConverterReader.cpp
            mtlxImageReader.cpp
            mtlxPreviewSurfaceReader.cpp
            mtlxPreviewSurfaceWriter.cpp
            mtlxStandardSurfaceReader.cpp
            mtlxStandardSurfaceWriter.cpp
            mtlxFileTextureWriter.cpp
    )
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
        WANT_MATERIALX_TRANSLATORS=1
    )
endif()
