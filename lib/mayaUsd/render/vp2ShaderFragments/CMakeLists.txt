# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        shaderFragments.cpp
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
list(APPEND shaderFragments_xmls
    # Copied from pxrUsdPreviewSurface with change to support multiple lights
    Float4ToFloatW.xml
    Float4ToFloatX.xml
    Float4ToFloatY.xml
    Float4ToFloatZ.xml
    UsdPreviewSurface.xml
    lightingContributions.xml
    opacityToTransparency.xml
    scaledDiffusePassThrough.xml
    scaledSpecularPassThrough.xml
    usdPreviewSurfaceCombiner.xml
    usdPreviewSurfaceLighting.xml
    # New fragments
    BasisCurvesLinearDomain_GLSL.xml
    BasisCurvesLinearDomain_HLSL.xml
    BasisCurvesLinearDomain_Cg.xml
    BasisCurvesLinearFallbackShader.xml
    BasisCurvesLinearHull.xml
    BasisCurvesCubicDomain_GLSL.xml
    BasisCurvesCubicDomain_HLSL.xml
    BasisCurvesCubicDomain_Cg.xml
    BasisCurvesCubicFallbackShader.xml
    BasisCurvesCubicHull.xml
    FallbackCPVShader.xml
    FallbackShader.xml
    Float4ToFloat3.xml
    Float4ToFloat4.xml
    NwFaceCameraIfNAN.xml
    UsdPrimvarReader_color.xml
    UsdPrimvarReader_float.xml
    UsdPrimvarReader_float2.xml
    UsdPrimvarReader_float3.xml
    UsdPrimvarReader_float4.xml
    UsdPrimvarReader_vector.xml
    UsdUVTexture.xml
    # USD plug info
    plugInfo.json
)

install(FILES ${shaderFragments_xmls}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
)
