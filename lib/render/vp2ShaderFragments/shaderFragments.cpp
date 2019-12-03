//
// Copyright 2019 Autodesk
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

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/usdImaging/usdImaging/tokens.h"

#include <maya/MGlobal.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MFragmentManager.h>
#include <maya/MShaderManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (FallbackCPVShader)
    (FallbackShader)

    (Float4ToFloatX)
    (Float4ToFloatY)
    (Float4ToFloatZ)
    (Float4ToFloatW)
    (Float4ToFloat3)
    (Float4ToFloat4)

    (lightingContributions)
    (scaledDiffusePassThrough)
    (scaledSpecularPassThrough)
    (opacityToTransparency)
    (usdPreviewSurfaceLighting)
    (usdPreviewSurfaceCombiner)

    (UsdUVTexture)

    (UsdPrimvarReader_float)
    (UsdPrimvarReader_float2)
    (UsdPrimvarReader_float3)
    (UsdPrimvarReader_float4)

    (UsdPreviewSurface)
);

static const TfTokenVector _FragmentNames = {
    _tokens->UsdUVTexture,

    _tokens->UsdPrimvarReader_float,
    _tokens->UsdPrimvarReader_float2,
    _tokens->UsdPrimvarReader_float3,
    _tokens->UsdPrimvarReader_float4,

    _tokens->Float4ToFloatX,
    _tokens->Float4ToFloatY,
    _tokens->Float4ToFloatZ,
    _tokens->Float4ToFloatW,
    _tokens->Float4ToFloat3,
    _tokens->Float4ToFloat4,

    _tokens->lightingContributions,
    _tokens->scaledDiffusePassThrough,
    _tokens->scaledSpecularPassThrough,
    _tokens->opacityToTransparency,
    _tokens->usdPreviewSurfaceLighting,
    _tokens->usdPreviewSurfaceCombiner
};

static const TfTokenVector _FragmentGraphNames = {
    _tokens->FallbackCPVShader,
    _tokens->FallbackShader,
    _tokens->UsdPreviewSurface
};


// Helper methods
namespace
{
    std::string _GetResourcePath(const std::string& resource)
    {
        static PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginWithName("mayaUsd_ShaderFragments");
        if (!TF_VERIFY(plugin, "Could not get plugin\n")) {
            return std::string();
        }

        const std::string path = PlugFindPluginResource(plugin, resource);
        TF_VERIFY(!path.empty(), "Could not find resource: %s\n", resource.c_str());

        return path;
    }
}

// Fragment registration
MStatus HdVP2ShaderFragments::registerFragments()
{
    // We do not force the renderer to initialize in case we're running in a
    // headless context. If we cannot get a handle to the renderer or the
    // fragment manager, we assume that's the case and simply return success.
    MHWRender::MRenderer* theRenderer =
        MHWRender::MRenderer::theRenderer(/* initializeRenderer = */ false);
    if (!theRenderer) {
        return MS::kSuccess;
    }

    MHWRender::MFragmentManager* fragmentManager =
        theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kSuccess;
    }

    // Register all fragments.
    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (fragmentManager->hasFragment(fragName)) {
            continue;
        }

        const std::string fragXmlFile =
            TfStringPrintf("%s.xml", fragName.asChar());
        const std::string fragXmlPath = _GetResourcePath(fragXmlFile);

        const MString addedName =
            fragmentManager->addShadeFragmentFromFile(
                fragXmlPath.c_str(),
                false);

        if (addedName != fragName) {
            MGlobal::displayError(
                TfStringPrintf("Failed to register fragment '%s' from file: %s",
                    fragName.asChar(),
                    fragXmlPath.c_str()).c_str());
            return MS::kFailure;
        }
    }

    // Register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (fragmentManager->hasFragment(fragGraphName)) {
            continue;
        }

        const std::string fragGraphXmlFile =
            TfStringPrintf("%s.xml", fragGraphName.asChar());
        const std::string fragGraphXmlPath = _GetResourcePath(fragGraphXmlFile);

        const MString addedName =
            fragmentManager->addFragmentGraphFromFile(fragGraphXmlPath.c_str());
        if (addedName != fragGraphName) {
            MGlobal::displayError(
                TfStringPrintf("Failed to register fragment graph '%s' from file: %s",
                    fragGraphName.asChar(),
                    fragGraphXmlPath.c_str()).c_str());
            return MS::kFailure;
        }
    }

    return MS::kSuccess;
}

// Fragment deregistration
MStatus HdVP2ShaderFragments::deregisterFragments()
{
    // Similar to registration, we do not force the renderer to initialize in
    // case we're running in a headless context. If we cannot get a handle to
    // the renderer or the fragment manager, we assume that's the case and
    // simply return success.
    MHWRender::MRenderer* theRenderer =
        MHWRender::MRenderer::theRenderer(/* initializeRenderer = */ false);
    if (!theRenderer) {
        return MS::kSuccess;
    }

    MHWRender::MFragmentManager* fragmentManager =
        theRenderer->getFragmentManager();
    if (!fragmentManager) {
        return MS::kSuccess;
    }

    // De-register all fragment graphs.
    for (const TfToken& fragGraphNameToken : _FragmentGraphNames) {
        const MString fragGraphName(fragGraphNameToken.GetText());

        if (!fragmentManager->removeFragment(fragGraphName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment graph: %s",
                    fragGraphName.asChar()).c_str());
            return MS::kFailure;
        }
    }

    // De-register all fragments.
    for (const TfToken& fragNameToken : _FragmentNames) {
        const MString fragName(fragNameToken.GetText());

        if (!fragmentManager->removeFragment(fragName)) {
            MGlobal::displayWarning(
                TfStringPrintf("Failed to remove fragment: %s",
                    fragName.asChar()).c_str());
            return MS::kFailure;
        }
    }

#if MAYA_API_VERSION >= 201700
    // Clear the shader manager's effect cache as well so that any changes to
    // the fragments will get picked up if they are re-registered.
    const MHWRender::MShaderManager* shaderMgr =
        theRenderer->getShaderManager();
    if (shaderMgr) {
        MStatus status = shaderMgr->clearEffectCache();
        if (status != MS::kSuccess) {
            MGlobal::displayWarning(
                "Failed to clear shader manager effect cache");
            return status;
        }
    }
#endif

    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
