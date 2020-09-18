# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        symmetricShaderWriter.cpp
        usdBlinnReader.cpp
        usdBlinnWriter.cpp
        usdFileTextureWriter.cpp
        usdLambertReader.cpp
        usdLambertWriter.cpp
        usdMaterialReader.cpp
        usdMaterialWriter.cpp
        usdPhongReader.cpp
        usdPhongWriter.cpp
        usdPhongEReader.cpp
        usdPhongEWriter.cpp
        usdReflectWriter.cpp
        usdUVTextureReader.cpp
)

if (MAYA_APP_VERSION VERSION_GREATER_EQUAL 2020)
    target_sources(${TARGET_NAME}
        PRIVATE
            usdStandardSurfaceReader.cpp
            usdStandardSurfaceWriter.cpp
    )
endif()
