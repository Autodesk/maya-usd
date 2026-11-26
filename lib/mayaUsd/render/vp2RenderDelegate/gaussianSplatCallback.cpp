//
// Copyright 2026 Autodesk
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
#include "gaussianSplatCallback.h"

#include "colorManagementPreferences.h"
#include "gaussianSplatViewCache.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnostic.h>

#include <maya/MDrawContext.h>
#include <maya/MFragmentManager.h>
#include <maya/MViewport2Renderer.h>

#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

//! Fragment names of the splat shader and its geometry stage (registered by shaderFragments.cpp).
const MString kSplatShaderName = "GaussianSplatShader";
const MString kSplatGeometryName = "GaussianSplatGeometry";
const MString kSplatStructOutputName = "outSurfaceFinal";

//! Name of the geometry stage input of the splat shader, and of the stage it is connected to.
const MString kGpuStageName = "GPUStage";

//! Plugin that holds the shader fragment resources, and the color managed graph template.
constexpr const char* kShaderFragmentsPluginName = "mayaUsd_ShaderFragments";
constexpr const char* kColorManagedGraphFile = "GaussianSplatShaderColorManaged.xml";
constexpr const char* kColorManagedGraphPrefix = "GaussianSplatShaderCM";

//! Placeholders of the color managed graph template.
constexpr const char* kGraphNameTag = "@GRAPH_NAME@";
constexpr const char* kOcioFragmentTag = "@OCIO_FRAGMENT@";
constexpr const char* kOcioInputTag = "@OCIO_INPUT@";
constexpr const char* kOcioOutputTag = "@OCIO_OUTPUT@";

//! The pre-draw callback of the splat shader: forwards the draw to the cache of the Rprim that
//! owns the shader clone. Main thread only.
void _PreDrawCallback(
    MHWRender::MDrawContext& context,
    const MHWRender::MRenderItemList&,
    MHWRender::MShaderInstance* shaderInstance)
{
    if (GaussianSplatViewCache* const cache = GaussianSplatViewCache::Find(shaderInstance)) {
        cache->OnDraw(context);
    }
}

//! Acquires a base shader from a fragment graph, with the pre-draw callback and the geometry
//! stage. The base is never attached to an item and never released before Maya exits, like the
//! entries of the shader cache.
MHWRender::MShaderInstance* _AcquireBaseShader(const MString& graphName)
{
    MHWRender::MRenderer* const            renderer = MHWRender::MRenderer::theRenderer();
    const MHWRender::MShaderManager* const shaderMgr
        = renderer ? renderer->getShaderManager() : nullptr;
    if (!shaderMgr) {
        return nullptr;
    }

    MHWRender::MShaderInstance* const base = shaderMgr->getFragmentShader(
        graphName, kSplatStructOutputName, true, _PreDrawCallback, nullptr);
    if (base) {
        base->addInputFragment(kSplatGeometryName, kGpuStageName, kGpuStageName);
    }
    return base;
}

//! The plain base (no color transform), acquired once.
MHWRender::MShaderInstance* _PlainBaseShader()
{
    static MHWRender::MShaderInstance* sBaseShader = nullptr;
    static bool                        sBaseTried = false;

    if (!sBaseTried) {
        sBaseTried = true;
        sBaseShader = _AcquireBaseShader(kSplatShaderName);
        if (!sBaseShader) {
            TF_WARN("The Gaussian splat shader could not be built: splats are not drawn.");
        }
    }
    return sBaseShader;
}

#ifdef HAS_COLOR_MANAGEMENT_SUPPORT_API

//! The text of a shader fragment resource file, empty if it cannot be found or read.
std::string _ReadFragmentResource(const std::string& fileName)
{
    PXR_NS::PlugPluginPtr plugin
        = PXR_NS::PlugRegistry::GetInstance().GetPluginWithName(kShaderFragmentsPluginName);
    if (!plugin) {
        return std::string();
    }
    const std::string path = PXR_NS::PlugFindPluginResource(plugin, fileName, false);
    std::ifstream     file(path.c_str());
    if (path.empty() || !file) {
        return std::string();
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

//! The splat graph with the transform between the geometry color and the Gaussian falloff, built
//! from the GaussianSplatShaderColorManaged.xml template (see the comment in that file).
std::string _ColorManagedGraphXml(
    const std::string& graphName,
    const MString&     ocioFragment,
    const MString&     ocioInput,
    const MString&     ocioOutput)
{
    std::string                               xml = _ReadFragmentResource(kColorManagedGraphFile);
    const std::pair<const char*, std::string> tokens[] = {
        { kGraphNameTag, graphName },
        { kOcioFragmentTag, ocioFragment.asChar() },
        { kOcioInputTag, ocioInput.asChar() },
        { kOcioOutputTag, ocioOutput.asChar() },
    };
    for (const auto& token : tokens) {
        const size_t length = std::strlen(token.first);
        for (size_t pos = xml.find(token.first); pos != std::string::npos;
             pos = xml.find(token.first, pos + token.second.size())) {
            xml.replace(pos, length, token.second);
        }
    }
    return xml;
}

//! Asks Maya for the OCIO fragment from `space` to the rendering space. An unknown space is
//! remembered, so that it is looked up and warned about once.
bool _GetOcioFragment(
    MHWRender::MFragmentManager* fragmentManager,
    const std::string&           space,
    MString&                     fragmentName,
    MString&                     inputName,
    MString&                     outputName)
{
    if (ColorManagementPreferences::isUnknownColorSpace(space)) {
        return false;
    }
    if (fragmentManager->getColorManagementFragmentInfo(
            MString(space.c_str()), fragmentName, inputName, outputName)
        != MStatus::kSuccess) {
        ColorManagementPreferences::addUnknownColorSpace(space);
        TF_WARN(
            "The color space '%s' of a Gaussian splat is unknown to the color management "
            "config: sRGB is used instead.",
            space.c_str());
        return false;
    }
    return true;
}

//! True when a transform to the rendering space must be applied: color management is active,
//! the source space is known and differs from the rendering space.
bool _NeedsColorTransform(bool active, const std::string& source, const std::string& rendering)
{
    return active && !source.empty() && source != rendering;
}

//! The base with the color transform from `declared` (sRGB when empty or unknown), or the plain
//! base when no transform applies. One cached base per source and rendering space pair.
MHWRender::MShaderInstance* _ColorManagedBaseShader(const std::string& declared)
{
    const bool        active = ColorManagementPreferences::Active();
    const std::string rendering = ColorManagementPreferences::RenderingSpaceName().asChar();
    const std::string sRGB = ColorManagementPreferences::sRGBName().asChar();

    // Nothing authored: sRGB, the USD default of splat colors.
    std::string source = declared.empty() ? sRGB : declared;
    if (!_NeedsColorTransform(active, source, rendering)) {
        return nullptr;
    }

    MHWRender::MRenderer* const        renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MFragmentManager* const fragmentManager
        = renderer ? renderer->getFragmentManager() : nullptr;
    if (!fragmentManager) {
        return nullptr;
    }

    MString fragmentName, inputName, outputName;
    if (!_GetOcioFragment(fragmentManager, source, fragmentName, inputName, outputName)) {
        // Unknown space: sRGB instead.
        source = sRGB;
        if (!_NeedsColorTransform(active, source, rendering)
            || !_GetOcioFragment(fragmentManager, source, fragmentName, inputName, outputName)) {
            return nullptr;
        }
    }

    // The cache is keyed by the transform, which depends on both spaces. A failed build is
    // cached too (null), so that it is not retried and warned about for every Rprim.
    static std::map<std::string, MHWRender::MShaderInstance*> sBases;
    const std::string                                         key = source + "\n" + rendering;
    const auto                                                found = sBases.find(key);
    if (found != sBases.end()) {
        return found->second;
    }

    const std::string graphName
        = std::string(kColorManagedGraphPrefix) + std::to_string(sBases.size());
    MHWRender::MShaderInstance* base = nullptr;
    const MString               added = fragmentManager->addFragmentGraphFromBuffer(
        _ColorManagedGraphXml(graphName, fragmentName, inputName, outputName).c_str());
    if (added.length() > 0) {
        base = _AcquireBaseShader(added);
    }
    if (!base) {
        TF_WARN(
            "The color management of the Gaussian splat color space '%s' could not be built: "
            "the color is drawn untransformed.",
            source.c_str());
    }
    sBases[key] = base;
    return base;
}

#endif // HAS_COLOR_MANAGEMENT_SUPPORT_API

} // anonymous namespace

MHWRender::MShaderInstance* CreateSplatShaderClone(const std::string& declaredColorSpace)
{
    MHWRender::MShaderInstance* base = nullptr;
#ifdef HAS_COLOR_MANAGEMENT_SUPPORT_API
    base = _ColorManagedBaseShader(declaredColorSpace);
#else
    (void)declaredColorSpace;
#endif
    if (!base) {
        base = _PlainBaseShader();
    }

    // clone() keeps the callback.
    return base ? base->clone() : nullptr;
}

void ReleaseSplatShader(MHWRender::MShaderInstance* shader)
{
    MHWRender::MRenderer* const            renderer = MHWRender::MRenderer::theRenderer();
    const MHWRender::MShaderManager* const shaderMgr
        = renderer ? renderer->getShaderManager() : nullptr;
    if (shader && shaderMgr) {
        shaderMgr->releaseShader(shader);
    }
}

} // namespace MAYAUSD_NS_DEF
