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

#include <fstream>
#include <regex>
#include <sstream>
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

std::map<std::string, std::string> _textureFragNames;
} // namespace

MString HdVP2ShaderFragments::getUsdUVTextureFragmentName(const MString& workingColorSpace)
{
    auto it = _textureFragNames.find(workingColorSpace.asChar());
    if (it != _textureFragNames.end()) {
        return it->second.c_str();
    }

    MGlobal::displayError(TfStringPrintf(
                              "Could not find a UsdUVTexture shader that outputs to working color "
                              "space %s. Will default to scene-linear Rec 709/sRGB conversion.",
                              workingColorSpace.asChar())
                              .c_str());

    return "UsdUVTexture_to_linrec709";
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

    {
        // This is a temporary fix for Maya 2022 that is quite fragile and will need to be revisited
        // in the near future. It will not handle the custom color space some large scale clients
        // use. It will also not handle any custom OCIO config that could change the color
        // interpolation algorithm.
        //
        // Note: Because the supported color spaces are hard-coded (i.e names and color
        // transformation implementation) it can only support few color spaces from the Maya default
        // configs and it will definitively fail on any custom configs. So, this is a temporary fix
        // for Maya that is quite fragile and will need to be revisited in the near future.
        //
        // TODO: Integrate OCIO in MayaUSD plugin, use the same config file as Maya, and request
        //       custom GPU color correction code to be generated by OCIO to match whichever
        //       rendering space is currently in use by Maya.

        // Register UVTexture readers for some common working spaces:
        std::string uvTextureFile = TfStringPrintf("%s.xml", _tokens->UsdUVTexture.GetText());
        uvTextureFile = _GetResourcePath(uvTextureFile);
        std::ifstream xmlFile(uvTextureFile.c_str());
        std::string   xmlString;
        xmlFile.seekg(0, std::ios::end);
        xmlString.reserve(xmlFile.tellg());
        xmlFile.seekg(0, std::ios::beg);
        xmlString.assign(
            (std::istreambuf_iterator<char>(xmlFile)), std::istreambuf_iterator<char>());

        const std::regex RE_FRAG("UsdUVTexture");
        const std::regex RE_GLSL("TO_MAYA_COLOR_SPACE_GLSL");
        const std::regex RE_HLSL("TO_MAYA_COLOR_SPACE_HLSL");
        const std::regex RE_CG("TO_MAYA_COLOR_SPACE_CG");

        // We create custom UsdUVTexture fragments. We use the original UsdUVTexture.xml as template
        // where we replace four important tokens with custom code:
        //
        //  - UsdUVTexture gets renamed by appending the name of the working space we are targetting
        //  - The three TO_MAYA_COLOR_SPACE_* markers gets replaced by the correct 4x4 matrix
        //  multiplication that will take the color from a "scene-linear Rec 709/sRGB" to the final
        //  color space. The syntax of the multiplication varies by shading language, and the
        //  template is:
        //     GLSL: outColor = mat4( ... ) * outColor;
        //     HLSL: outColor = mul(outColor, float4x4( ... ));
        //       CG: outColor = mul(float4x4( transpose( ... ) ), outColor);
        //    Where the ... denotes a 4x4 matrix expanded from the 3x3 matrix passed in as parameter
        //    to the function (alpha is always left untouched).
        //
        auto registerSpace = [&](const std::string& ocioName,
                                 const std::string& synColorName,
                                 const std::string& fragName,
                                 const float*       convMatrix) {
            if (!fragmentManager->hasFragment(fragName.c_str())) {
                std::string xmlFinal = std::regex_replace(xmlString, RE_FRAG, fragName.c_str());
                if (convMatrix) {
                    std::stringstream op_glsl, op_hlsl, op_cg, op_matrix;
                    op_glsl << "outColor = mat4(";
                    op_hlsl << "outColor = mul(outColor, float4x4(";
                    op_matrix << convMatrix[0];
                    for (int i = 1; i < 16; ++i) {
                        op_matrix << ", " << convMatrix[i];
                    }
                    op_glsl << op_matrix.str() << ") * outColor;";
                    op_hlsl << op_matrix.str() << "));";
                    op_cg << "outColor = mul(float4x4(";
                    for (int i = 0; i < 4; ++i) {
                        // Cg needs transposed matrix:
                        op_cg << convMatrix[i] << ", " << convMatrix[4 + i] << ", "
                              << convMatrix[8 + i] << ", " << convMatrix[12 + i]
                              << ((i < 3) ? ", " : "");
                    }
                    op_cg << "), outColor);";
                    xmlFinal = std::regex_replace(xmlFinal, RE_GLSL, op_glsl.str().c_str());
                    xmlFinal = std::regex_replace(xmlFinal, RE_HLSL, op_hlsl.str().c_str());
                    xmlFinal = std::regex_replace(xmlFinal, RE_CG, op_cg.str().c_str());
                } else {
                    xmlFinal = std::regex_replace(xmlFinal, RE_GLSL, "");
                    xmlFinal = std::regex_replace(xmlFinal, RE_HLSL, "");
                    xmlFinal = std::regex_replace(xmlFinal, RE_CG, "");
                }
                const MString addedName
                    = fragmentManager->addShadeFragmentFromBuffer(xmlFinal.c_str(), false);
                if (addedName == fragName.c_str()) {
                    _textureFragNames.emplace(ocioName, fragName);
                    if (!synColorName.empty()) {
                        _textureFragNames.emplace(synColorName, fragName);
                    }
                } else {
                    MGlobal::displayError(
                        TfStringPrintf(
                            "Failed to register UsdUVTexture fragment graph for color space: %s",
                            ocioName.c_str())
                            .c_str());
                }
            }
        };

        // OpenGL linear is equivalent to "scene-linear Rec 709/sRGB", so we do not need a
        // transformation
        registerSpace(
            "scene-linear Rec.709-sRGB", // OCIO v2
            "scene-linear Rec 709/sRGB", // synColor
            "UsdUVTexture_to_linrec709",
            nullptr);

        // clang-format off
        const float LINREC709_TO_ACESCG[16]
            = { 0.61309740, 0.07019372, 0.02061559, 0.0,
                0.33952315, 0.91635388, 0.10956977, 0.0,
                0.04737945, 0.01345240, 0.86981463, 0.0,
                0.0,        0.0,        0.0,        1.0 };
        // clang-format on
        registerSpace("ACEScg", "", "UsdUVTexture_to_ACEScg", LINREC709_TO_ACESCG);

        // clang-format off
        const float LINREC709_TO_ACES2065_1[16]
            = { 0.43963298, 0.08977644, 0.01754117, 0.0,
                0.38298870, 0.81343943, 0.11154655, 0.0,
                0.17737832, 0.09678413, 0.87091228, 0.0,
                0.0,        0.0,        0.0,        1.0 };
        // clang-format on
        registerSpace("ACES2065-1", "", "UsdUVTexture_to_ACES2065_1", LINREC709_TO_ACES2065_1);

        // clang-format off
        const float LINREC709_TO_SCENE_LINREC709_DCI_P3_D65[16]
            = { 0.82246197, 0.03319420, 0.01708263, 0.0,
                0.17753803, 0.96680580, 0.07239744, 0.0,
                0.,         0.,         0.91051993, 0.0,
                0.0,        0.0,        0.0,        1.0 };
        // clang-format on
        registerSpace(
            "scene-linear DCI-P3 D65",
            "scene-linear DCI-P3",
            "UsdUVTexture_to_lin_DCI_P3_D65",
            LINREC709_TO_SCENE_LINREC709_DCI_P3_D65);

        // clang-format off
        const float LINREC709_TO_SCENE_LINREC709_REC_2020[16]
            = { 0.62740389, 0.06909729, 0.01639144, 0.0,
                0.32928304, 0.91954039, 0.08801331, 0.0,
                0.04331307, 0.01136232, 0.89559525, 0.0,
                0.0,        0.0,        0.0,        1.0 };
        // clang-format on
        registerSpace(
            "scene-linear Rec.2020",
            "scene-linear Rec 2020",
            "UsdUVTexture_to_linrec2020",
            LINREC709_TO_SCENE_LINREC709_REC_2020);
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

    // De-register the various UsdUVTexture fragments:
    for (const auto& txtFrag : _textureFragNames) {
        if (!fragmentManager->removeFragment(txtFrag.second.c_str())) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment graph: %s", txtFrag.second.c_str())
                    .c_str());
            return MS::kFailure;
        }
    }

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
