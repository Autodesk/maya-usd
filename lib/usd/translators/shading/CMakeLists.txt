# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        usdBlinnWriter.cpp
        usdFileTextureWriter.cpp
        usdLambertWriter.cpp
        usdMaterialWriter.cpp
        usdPhongWriter.cpp
        usdPhongEWriter.cpp
        usdReflectWriter.cpp
        usdStandardSurfaceWriter.cpp
        usdUVTextureReader.cpp
)
