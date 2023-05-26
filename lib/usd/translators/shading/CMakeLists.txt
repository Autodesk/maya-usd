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
        usdStandardSurfaceReader.cpp
        usdStandardSurfaceWriter.cpp
)

if (BUILD_RFM_TRANSLATORS)
    target_sources(${TARGET_NAME}
        PRIVATE
            rfmShaderTranslation.cpp
    )
endif()

# Edit as Maya requires UFE path mapping.
if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${TARGET_NAME}
        PRIVATE
            materialUpdater.cpp
    )
endif()

if (MAYA_APP_VERSION VERSION_GREATER 2022)
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
