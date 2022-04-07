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
list(APPEND SHADERFRAGMENTS_XMLS
    # Copied from pxrUsdPreviewSurface with change to support multiple lights
    Float4ToFloatW.xml
    Float4ToFloatX.xml
    Float4ToFloatY.xml
    Float4ToFloatZ.xml
    UsdPreviewSurfaceLightAPI1.xml
    UsdPreviewSurfaceLightAPI2.xml
    UsdPreviewSurfaceLightAPI3.xml
    lightingContributions.xml
    opacityToTransparency.xml
    scaledDiffusePassThrough.xml
    scaledSpecularPassThrough.xml
    usdPreviewSurfaceCombiner.xml
    usdPreviewSurfaceLightingAPI1.xml
    usdPreviewSurfaceLightingAPI2.xml
    usdPreviewSurfaceLightingAPI3.xml
    # New fragments
    BasisCurvesCubicColorDomain.xml
    BasisCurvesCubicCPVHull.xml
    BasisCurvesCubicCPVPassing.xml
    BasisCurvesCubicCPVShader.xml
    BasisCurvesCubicDomain_GLSL.xml
    BasisCurvesCubicDomain_HLSL.xml
    BasisCurvesCubicDomain_Cg.xml
    BasisCurvesCubicFallbackShader.xml
    BasisCurvesCubicHull.xml
    BasisCurvesLinearColorDomain.xml
    BasisCurvesLinearCPVHull.xml
    BasisCurvesLinearCPVPassing.xml
    BasisCurvesLinearCPVShader.xml
    BasisCurvesLinearDomain_GLSL.xml
    BasisCurvesLinearDomain_HLSL.xml
    BasisCurvesLinearDomain_Cg.xml
    BasisCurvesLinearFallbackShader.xml
    BasisCurvesLinearHull.xml
    FallbackCPVShader.xml
    FallbackShader.xml
    Float4ToFloat3.xml
    Float4ToFloat4.xml
    NwFaceCameraIfNAN.xml
    PointsFallbackShader.xml
    PointsFallbackCPVShader.xml
    PointsGeometry.xml
    UsdDrawModeCards.xml
    UsdPrimvarColor.xml
    UsdPrimvarReader_color.xml
    UsdPrimvarReader_float.xml
    UsdPrimvarReader_float2.xml
    UsdPrimvarReader_float3.xml
    UsdPrimvarReader_float4.xml
    UsdPrimvarReader_vector.xml
    UsdTransform2d.xml
    UsdUVTexture.xml
    # USD plug info
    plugInfo.json
)

set(HEADERS
    shaderFragments.h
)

# -----------------------------------------------------------------------------
# promote headers
# -----------------------------------------------------------------------------
mayaUsd_promoteHeaderList(HEADERS ${HEADERS} SUBDIR render/vp2ShaderFragments)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(FILES ${SHADERFRAGMENTS_XMLS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaUsd_ShaderFragments/resources
)

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/render/vp2ShaderFragments
)
