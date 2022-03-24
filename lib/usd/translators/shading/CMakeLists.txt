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
        usdPrimvarReaderFloat3Reader.cpp
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

# These import/export classes will work with older versions of Maya and USD if
# you plan to render in usdView or other MaterialX-aware renderers. However,
# getting these materials to appear in the Maya viewport when creating a USD
# stage will definitely require a recent Maya preview release combined with
# a bleeding edge USD with version > 21.05
if (MAYA_APP_VERSION VERSION_GREATER 2022 AND PXR_VERSION VERSION_GREATER 2102)
    target_sources(${TARGET_NAME}
        PRIVATE
            mtlxBaseReader.cpp
            mtlxBaseWriter.cpp
            mtlxConstructorReader.cpp
            mtlxConverterReader.cpp
            mtlxFileTextureReader.cpp
            mtlxImageReader.cpp
            mtlxPlace2dTextureWriter.cpp
            mtlxPreviewSurfaceReader.cpp
            mtlxPreviewSurfaceWriter.cpp
            mtlxStandardSurfaceReader.cpp
            mtlxStandardSurfaceWriter.cpp
            mtlxSymmetricShaderReader.cpp
            mtlxSymmetricShaderWriter.cpp
            mtlxFileTextureWriter.cpp
    )
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
        WANT_MATERIALX_TRANSLATORS=1
    )
endif()
