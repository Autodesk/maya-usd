//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "shaderFragments.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/thisPlugin.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFragmentManager.h>
#include <maya/MGlobal.h>
#include <maya/MShaderManager.h>
#include <maya/MViewport2Renderer.h>

#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdVP2ShaderFragmentsTokens, MAYAUSD_CORE_PUBLIC_USD_PREVIEW_SURFACE_TOKENS);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (BasisCurvesCubicColorDomain)
    (BasisCurvesCubicCPVHull)
    (BasisCurvesCubicCPVPassing)
    (BasisCurvesCubicCPVShader)
    (BasisCurvesCubicDomain)
    (BasisCurvesCubicFallbackShader)
    (BasisCurvesCubicHull)
    (BasisCurvesLinearColorDomain)
    (BasisCurvesLinearCPVHull)
    (BasisCurvesLinearCPVPassing)
    (BasisCurvesLinearCPVShader)
    (BasisCurvesLinearDomain)
    (BasisCurvesLinearFallbackShader)
    (BasisCurvesLinearHull)

    (FallbackCPVShader)
    (FallbackShader)

    (Float4ToFloatX)
    (Float4ToFloatY)
    (Float4ToFloatZ)
    (Float4ToFloatW)
    (Float4ToFloat3)
    (Float4ToFloat4)

    (NwFaceCameraIfNAN)

    (lightingContributions)
    (scaledDiffusePassThrough)
    (scaledSpecularPassThrough)
    (opacityToTransparency)
    (UsdDrawModeCards)
    (usdPreviewSurfaceLightingAPI1)
    (usdPreviewSurfaceLightingAPI2)
    (usdPreviewSurfaceCombiner)

    (UsdPrimvarColor)

    (UsdUVTexture)
    (NwToNv)

    (UsdPrimvarReader_color)
    (UsdPrimvarReader_float)
    (UsdPrimvarReader_float2)
    (UsdPrimvarReader_float3)
    (UsdPrimvarReader_float4)
    (UsdPrimvarReader_vector)

    // Graph:
    (UsdPreviewSurfaceLightAPI1)
    (UsdPreviewSurfaceLightAPI2)
);
// clang-format on

static const TfTokenVector _LanguageSpecificFragmentNames
    = { _tokens->BasisCurvesLinearDomain, _tokens->BasisCurvesCubicDomain };

static const TfTokenVector _FragmentNames = { _tokens->BasisCurvesCubicColorDomain,
                                              _tokens->BasisCurvesCubicCPVHull,
                                              _tokens->BasisCurvesCubicCPVPassing,
                                              _tokens->BasisCurvesCubicHull,
                                              _tokens->BasisCurvesLinearColorDomain,
                                              _tokens->BasisCurvesLinearCPVHull,
                                              _tokens->BasisCurvesLinearCPVPassing,
                                              _tokens->BasisCurvesLinearHull,

                                              _tokens->UsdPrimvarColor,

                                              _tokens->UsdUVTexture,
                                              _tokens->NwToNv,

                                              _tokens->UsdPrimvarReader_color,
                                              _tokens->UsdPrimvarReader_float,
                                              _tokens->UsdPrimvarReader_float2,
                                              _tokens->UsdPrimvarReader_float3,
                                              _tokens->UsdPrimvarReader_float4,
                                              _tokens->UsdPrimvarReader_vector,

                                              _tokens->Float4ToFloatX,
                                              _tokens->Float4ToFloatY,
                                              _tokens->Float4ToFloatZ,
                                              _tokens->Float4ToFloatW,
                                              _tokens->Float4ToFloat3,
                                              _tokens->Float4ToFloat4,

                                              _tokens->NwFaceCameraIfNAN,

                                              _tokens->lightingContributions,
                                              _tokens->scaledDiffusePassThrough,
                                              _tokens->scaledSpecularPassThrough,
                                              _tokens->opacityToTransparency,
                                              _tokens->UsdDrawModeCards,
                                              _tokens->usdPreviewSurfaceLightingAPI1,
                                              _tokens->usdPreviewSurfaceLightingAPI2,
                                              _tokens->usdPreviewSurfaceCombiner };

static const TfTokenVector _FragmentGraphNames
    = { _tokens->BasisCurvesCubicCPVShader,  _tokens->BasisCurvesCubicFallbackShader,
        _tokens->BasisCurvesLinearCPVShader, _tokens->BasisCurvesLinearFallbackShader,
        _tokens->FallbackCPVShader,          _tokens->FallbackShader };

namespace {
//! Get the file path of the shader fragment.
std::string _GetResourcePath(const std::string& resource)
{
    static PlugPluginPtr plugin
        = PlugRegistry::GetInstance().GetPluginWithName("mayaUsd_ShaderFragments");
    if (!TF_VERIFY(plugin, "Could not get plugin\n")) {
        return std::string();
    }

    const std::string path = PlugFindPluginResource(plugin, resource);
    TF_VERIFY(!path.empty(), "Could not find resource: %s\n", resource.c_str());

    return path;
}

#if MAYA_API_VERSION >= 20210000

//! Structure for Automatic shader stage input parameter to register in VP2.
struct AutomaticShaderStageInput
{
    MHWRender::MFragmentManager::ShaderStage  _shaderStage;
    MString                                   _parameterName;
    MString                                   _parameterSemantic;
    MHWRender::MShaderInstance::ParameterType _parameterType;
    bool                                      _isVaryingInput;
};

//! List of automatic shader stage input parameters to register in VP2.
std::vector<AutomaticShaderStageInput> _automaticShaderStageInputs
    = { { MHWRender::MFragmentManager::kVertexShader,
          "UsdPrimvarColor",
          "COLOR0",
          MHWRender::MShaderInstance::kFloat4,
          true },
        { MHWRender::MFragmentManager::kHullShader,
          "UsdPrimvarColor",
          "COLOR0",
          MHWRender::MShaderInstance::kFloat4,
          true },
        { MHWRender::MFragmentManager::kDomainShader,
          "UsdPrimvarColor",
          "COLOR0",
          MHWRender::MShaderInstance::kFloat4,
          false },
        { MHWRender::MFragmentManager::kPixelShader,
          "BasisCurvesCubicColor",
          "COLOR0",
          MHWRender::MShaderInstance::kFloat4,
          true },
        { MHWRender::MFragmentManager::kPixelShader,
          "BasisCurvesLinearColor",
          "COLOR0",
          MHWRender::MShaderInstance::kFloat4,
          true } };

//! Name mapping between a parameter and a desired domain shader fragment to register in VP2.
std::vector<std::pair<MString, MString>> _domainShaderInputNameMappings
    = { { "BasisCurvesCubicColor", "BasisCurvesCubicColorDomain" },
        { "BasisCurvesLinearColor", "BasisCurvesLinearColorDomain" } };

#endif

} // anonymous namespace

namespace {
int _registrationCount = 0;
}

// Fragment registration should be done after VP2 has been initialized, to avoid any errors from
// headless configurations or command-line renders.
MStatus HdVP2ShaderFragments::registerFragments()
{
    // If we're already registered, do nothing.
    if (_registrationCount > 0) {
        _registrationCount++;
        return MS::kSuccess;
    }

    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) {
        return MS::kFailure;
    }

    MHWRender::MFragmentManager* fragmentManager = theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kFailure;
    }

    std::string language;

    switch (theRenderer->drawAPI()) {
    case MHWRender::kOpenGLCoreProfile: language = "GLSL"; break;
    case MHWRender::kDirectX11: language = "HLSL"; break;
    case MHWRender::kOpenGL: language = "Cg"; break;
    default: MGlobal::displayError("Unknown draw API"); break;
    }

    // Register all fragments.
    for (const TfToken& fragNameToken : _LanguageSpecificFragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (fragmentManager->hasFragment(fragName)) {
            continue;
        }

        const std::string fragXmlFile
            = TfStringPrintf("%s_%s.xml", fragName.asChar(), language.c_str());
        const std::string fragXmlPath = _GetResourcePath(fragXmlFile);

        const MString addedName
            = fragmentManager->addShadeFragmentFromFile(fragXmlPath.c_str(), false);

        if (addedName != fragName) {
            MGlobal::displayError(TfStringPrintf(
                                      "Failed to register fragment '%s' from file: %s",
                                      fragName.asChar(),
                                      fragXmlPath.c_str())
                                      .c_str());
            return MS::kFailure;
        }
    }

    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (fragmentManager->hasFragment(fragName)) {
            continue;
        }

        const std::string fragXmlFile = TfStringPrintf("%s.xml", fragName.asChar());
        const std::string fragXmlPath = _GetResourcePath(fragXmlFile);

        const MString addedName
            = fragmentManager->addShadeFragmentFromFile(fragXmlPath.c_str(), false);

        if (addedName != fragName) {
            MGlobal::displayError(TfStringPrintf(
                                      "Failed to register fragment '%s' from file: %s",
                                      fragName.asChar(),
                                      fragXmlPath.c_str())
                                      .c_str());
            return MS::kFailure;
        }
    }

    // Register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (fragmentManager->hasFragment(fragGraphName)) {
            continue;
        }

        const std::string fragGraphXmlFile = TfStringPrintf("%s.xml", fragGraphName.asChar());
        const std::string fragGraphXmlPath = _GetResourcePath(fragGraphXmlFile);

        const MString addedName
            = fragmentManager->addFragmentGraphFromFile(fragGraphXmlPath.c_str());
        if (addedName != fragGraphName) {
            MGlobal::displayError(TfStringPrintf(
                                      "Failed to register fragment graph '%s' from file: %s",
                                      fragGraphName.asChar(),
                                      fragGraphXmlPath.c_str())
                                      .c_str());
            return MS::kFailure;
        }
    }

    // Register a UsdPreviewSurface shader graph:
    {
        const MString fragGraphName(HdVP2ShaderFragmentsTokens->SurfaceFragmentGraphName.GetText());
#ifdef MAYA_LIGHTAPI_VERSION_2
        const MString fragGraphFileName(_tokens->UsdPreviewSurfaceLightAPI2.GetText());
#else
        const MString fragGraphFileName(_tokens->UsdPreviewSurfaceLightAPI1.GetText());
#endif
        if (!fragmentManager->hasFragment(fragGraphName)) {
            const std::string fragGraphXmlFile
                = TfStringPrintf("%s.xml", fragGraphFileName.asChar());
            const std::string fragGraphXmlPath = _GetResourcePath(fragGraphXmlFile);

            const MString addedName
                = fragmentManager->addFragmentGraphFromFile(fragGraphXmlPath.c_str());
            if (addedName != fragGraphName) {
                MGlobal::displayError(TfStringPrintf(
                                          "Failed to register fragment graph '%s' from file: %s",
                                          fragGraphName.asChar(),
                                          fragGraphXmlPath.c_str())
                                          .c_str());
                return MS::kFailure;
            }
        }
    }

#if MAYA_API_VERSION >= 20210000

    // Register automatic shader stage input parameters.
    for (const auto& input : _automaticShaderStageInputs) {
        fragmentManager->addAutomaticShaderStageInput(
            input._shaderStage,
            input._parameterName,
            input._parameterSemantic,
            input._parameterType,
            input._isVaryingInput);
    }

    // Register a desired domain shader fragment for each input parameter.
    for (const auto& mapping : _domainShaderInputNameMappings) {
        fragmentManager->addDomainShaderInputNameMapping(mapping.first, mapping.second);
    }

#endif

    _registrationCount++;

    return MS::kSuccess;
}

// Fragment deregistration
MStatus HdVP2ShaderFragments::deregisterFragments()
{
    // If it was never registered, leave as-is:
    if (_registrationCount == 0) {
        return MS::kSuccess;
    }

    // If more than one plugin still has us registered, do nothing.
    if (_registrationCount > 1) {
        _registrationCount--;
        return MS::kSuccess;
    }
    MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer) {
        return MS::kFailure;
    }

    MHWRender::MFragmentManager* fragmentManager = theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kFailure;
    }

#if MAYA_API_VERSION >= 20210000

    // De-register a desired domain shader fragment for each input parameter.
    for (const auto& mapping : _domainShaderInputNameMappings) {
        fragmentManager->removeDomainShaderInputNameMapping(mapping.first);
    }

    // De-register automatic shader stage input parameters.
    for (const auto& input : _automaticShaderStageInputs) {
        fragmentManager->removeAutomaticShaderStageInput(input._shaderStage, input._parameterName);
    }

#endif

    // De-register UsdPreviewsurface graph:
    if (!fragmentManager->removeFragment(
            HdVP2ShaderFragmentsTokens->SurfaceFragmentGraphName.GetText())) {
        MGlobal::displayWarning("Failed to remove fragment graph: UsdPreviewsurface");
        return MS::kFailure;
    }

    // De-register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (!fragmentManager->removeFragment(fragGraphName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment graph: %s", fragGraphName.asChar())
                    .c_str());
            return MS::kFailure;
        }
    }

    // De-register all fragments.
    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (!fragmentManager->removeFragment(fragName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment: %s", fragName.asChar()).c_str());
            return MS::kFailure;
        }
    }

    _registrationCount--;

    // Clear the shader manager's effect cache as well so that any changes to
    // the fragments will get picked up if they are re-registered.
    const MHWRender::MShaderManager* shaderMgr = theRenderer->getShaderManager();
    if (shaderMgr) {
        MStatus status = shaderMgr->clearEffectCache();
        if (status != MS::kSuccess) {
            MGlobal::displayWarning("Failed to clear shader manager effect cache");
            return status;
        }
    }

    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
