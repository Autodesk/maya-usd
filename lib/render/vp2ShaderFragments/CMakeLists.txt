# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        shaderFragments.cpp
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
list(APPEND mayaUsdShaderFragments_xmls
    # Copied from pxrUsdPreviewSurface with change to support multiple lights
    Float4ToFloatW.xml
    Float4ToFloatX.xml
    Float4ToFloatY.xml
    Float4ToFloatZ.xml
    lightingContributions.xml
    opacityToTransparency.xml
    scaledDiffusePassThrough.xml
    scaledSpecularPassThrough.xml
    UsdPreviewSurface.xml
    usdPreviewSurfaceCombiner.xml
    usdPreviewSurfaceLighting.xml
    # New fragments
    FallbackCPVShader.xml
    FallbackShader.xml
    Float4ToFloat3.xml
    Float4ToFloat4.xml
    UsdPrimvarReader_float.xml
    UsdPrimvarReader_float2.xml
    UsdPrimvarReader_float3.xml
    UsdPrimvarReader_float4.xml
    UsdUVTexture.xml
    # USD plug info
    plugInfo.json
)

install(FILES ${mayaUsdShaderFragments_xmls}
     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
)
