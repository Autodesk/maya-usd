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

#include "debugCodes.h"
#include "material.h"
#include "render_delegate.h"

#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdHydra/tokens.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/diagnostic.h"

#include <maya/MProfiler.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MUintArray.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MFragmentManager.h>
#include <maya/MShaderManager.h>
#include <maya/MTextureManager.h>

#include <boost/filesystem.hpp>

#include <iostream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (file)
    (opacity)
    (st)
    (varname)

    (input)
    (output)

    (rgb)
    (r)
    (g)
    (b)
    (a)

    (xyz)
    (x)
    (y)
    (z)
    (w)

    (Float4ToFloatX)
    (Float4ToFloatY)
    (Float4ToFloatZ)
    (Float4ToFloatW)
    (Float4ToFloat3)
);

//! Helper utility function to test whether a node is a UsdShade primvar reader.
bool _IsUsdPrimvarReader(const HdMaterialNode& node)
{
    const TfToken& id = node.identifier;
    return (id == UsdImagingTokens->UsdPrimvarReader_float ||
            id == UsdImagingTokens->UsdPrimvarReader_float2 ||
            id == UsdImagingTokens->UsdPrimvarReader_float3 ||
            id == UsdImagingTokens->UsdPrimvarReader_float4);
}

//! Helper utility function to test whether a node is a UsdShade UV texture.
inline bool _IsUsdUVTexture(const HdMaterialNode& node)
{
    return (node.identifier == UsdImagingTokens->UsdUVTexture);
}

//! Helper utility function to print nodes, connections and primvars in the
//! specified material network.
void _PrintMaterialNetwork(
    const std::string& label,
    const SdfPath& id,
    const HdMaterialNetwork& mat)
{
    std::cout << label << " material network for " << id << "\n";

    std::cout << "  --Node--\n";
    for (HdMaterialNode const& node: mat.nodes) {
        std::cout << "    " << node.path << "\n";
        std::cout << "    " << node.identifier << "\n";

        for (auto const& entry: node.parameters) {
            std::cout << "      param " << entry.first << ": "
                << TfStringify(entry.second) << "\n";
        }
    }

    std::cout << "  --Connections--\n";
    for (const HdMaterialRelationship& rel: mat.relationships) {
        std::cout << "    " << rel.inputId << "." << rel.inputName
            << "->" << rel.outputId << "." << rel.outputName << "\n";
    }

    std::cout << "  --Primvars--\n";
    for (TfToken const& primvar: mat.primvars) {
        std::cout << "    " << primvar << "\n";
    }
}

//! Helper utility function to apply VP2-specific fixes to the material network.
//! - Add passthrough nodes to read vector component(s).
//! - Fix UsdImagingMaterialAdapter issue for not producing primvar requirements.
//! - Temporary workaround of missing support for normal map.
void _ApplyVP2Fixes(HdMaterialNetwork& outNet, const HdMaterialNetwork& inNet)
{
    unsigned int numPassThroughNodes = 0;

    // To avoid relocation, reserve enough space for possible maximal size. The
    // output network is temporary C++ object that will be released after use.
    const size_t numNodes = inNet.nodes.size();
    const size_t numRelationships = inNet.relationships.size();
    outNet.nodes.reserve(numNodes + numRelationships);
    outNet.relationships.reserve(numRelationships * 2);
    outNet.primvars.reserve(numNodes);

    for (const HdMaterialNode &node : inNet.nodes)
    {
        outNet.nodes.push_back(node);

        // Copy outgoing connections and if needed add passthrough node/connection.
        for (const HdMaterialRelationship& rel : inNet.relationships) {
            if (rel.inputId != node.path) {
                continue;
            }

            TfToken passThroughId;
            if (rel.inputName == _tokens->rgb || rel.inputName == _tokens->xyz) {
                passThroughId = _tokens->Float4ToFloat3;
            }
            else if (rel.inputName == _tokens->r || rel.inputName == _tokens->x) {
                passThroughId = _tokens->Float4ToFloatX;
            }
            else if (rel.inputName == _tokens->g || rel.inputName == _tokens->y) {
                passThroughId = _tokens->Float4ToFloatY;
            }
            else if (rel.inputName == _tokens->b || rel.inputName == _tokens->z) {
                passThroughId = _tokens->Float4ToFloatZ;
            }
            else if (rel.inputName == _tokens->a || rel.inputName == _tokens->w) {
                passThroughId = _tokens->Float4ToFloatW;
            }
            else {
                outNet.relationships.push_back(rel);
                continue;
            }

            const SdfPath passThroughPath = rel.inputId.ReplaceName(TfToken(
                TfStringPrintf("HdVP2PassThrough%d", numPassThroughNodes++)));

            const HdMaterialNode passThroughNode = {
                passThroughPath, passThroughId, {}
            };
            outNet.nodes.push_back(passThroughNode);

            HdMaterialRelationship newRel = {
                rel.inputId, _tokens->output, passThroughPath, _tokens->input
            };
            outNet.relationships.push_back(newRel);

            newRel = {
                passThroughPath, _tokens->output, rel.outputId, rel.outputName
            };
            outNet.relationships.push_back(newRel);
        }

        // Normal map is not supported yet. For now primvars:normals is used for
        // shading, which is also the current behavior of USD/Hydra.
        // https://groups.google.com/d/msg/usd-interest/7epU16C3eyY/X9mLW9VFEwAJ
        if (node.identifier == UsdImagingTokens->UsdPreviewSurface) {
            outNet.primvars.push_back(HdTokens->normals);
        }
        // UsdImagingMaterialAdapter doesn't create primvar requirements as
        // expected. Workaround by manually looking up "varname" parameter.
        // https://groups.google.com/forum/#!msg/usd-interest/z-14AgJKOcU/1uJJ1thXBgAJ
        else if (_IsUsdPrimvarReader(node)) {
            auto it = node.parameters.find(_tokens->varname);
            if (it != node.parameters.end()) {
                outNet.primvars.push_back(TfToken(TfStringify(it->second)));
            }
        }
    }
}

//! Helper utility function to convert Hydra texture addressing token to VP2 enum.
MHWRender::MSamplerState::TextureAddress _ConvertToTextureSamplerAddressEnum(
    const TfToken& token)
{
    MHWRender::MSamplerState::TextureAddress address;

    if (token == UsdHydraTokens->clamp) {
        address = MHWRender::MSamplerState::kTexClamp;
    }
    else if (token == UsdHydraTokens->mirror) {
        address = MHWRender::MSamplerState::kTexMirror;
    }
    else if (token == UsdHydraTokens->black) {
        address = MHWRender::MSamplerState::kTexBorder;
    }
    else {
        address = MHWRender::MSamplerState::kTexWrap;
    }

    return address;
}

//! Get sampler state description as required by the material node.
MHWRender::MSamplerStateDesc _GetSamplerStateDesc(const HdMaterialNode& node)
{
    TF_VERIFY(_IsUsdUVTexture(node));

    MHWRender::MSamplerStateDesc desc;
    desc.filter = MHWRender::MSamplerState::kMinMagMipLinear;

    auto it = node.parameters.find(UsdHydraTokens->wrapS);
    if (it != node.parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<TfToken>()) {
            const TfToken& token = value.UncheckedGet<TfToken>();
            desc.addressU = _ConvertToTextureSamplerAddressEnum(token);
        }
    }

    it = node.parameters.find(UsdHydraTokens->wrapT);
    if (it != node.parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<TfToken>()) {
            const TfToken& token = value.UncheckedGet<TfToken>();
            desc.addressV = _ConvertToTextureSamplerAddressEnum(token);
        }
    }

    return desc;
}

//! Load texture from the specified path
MHWRender::MTexture* _LoadTexture(
    const std::string& path,
    bool& isColorSpaceSRGB)
{
    isColorSpaceSRGB = false;

    MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr =
        renderer ? renderer->getTextureManager() : nullptr;
    if (!TF_VERIFY(textureMgr)) {
        return nullptr;
    }

    GlfImageSharedPtr image = GlfImage::OpenForReading(path);
    if (!TF_VERIFY(image)) {
        return nullptr;
    }

    // GlfImage is used for loading pixel data from usdz only and should
    // not trigger any OpenGL call. VP2RenderDelegate will transfer the
    // texels to GPU memory with VP2 API which is 3D API agnostic.
    GlfImage::StorageSpec spec;
    spec.width = image->GetWidth();
    spec.height = image->GetHeight();
    spec.depth = 1;
    spec.format = image->GetFormat();
    spec.type = image->GetType();
    spec.flipped = false;

    const int bpp = image->GetBytesPerPixel();
    const int bytesPerRow = spec.width * bpp;
    const int bytesPerSlice = bytesPerRow * spec.height;

    std::vector<unsigned char> storage(bytesPerSlice);
    spec.data = storage.data();

    if (!image->Read(spec)) {
        return nullptr;
    }

    MHWRender::MTexture* texture = nullptr;

    MHWRender::MTextureDescription desc;
    desc.setToDefault2DTexture();
    desc.fWidth = spec.width;
    desc.fHeight = spec.height;
    desc.fBytesPerRow = bytesPerRow;
    desc.fBytesPerSlice = bytesPerSlice;

    switch (spec.format)
    {
    case GL_RED:
        desc.fFormat = (spec.type == GL_FLOAT ?
            MHWRender::kR32_FLOAT : MHWRender::kR8_UNORM);
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case GL_RGB:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32_FLOAT;
            texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        }
        else {
            // R8G8B8 is not supported by VP2. Converted to R8G8B8A8.
            constexpr int bpp_4 = 4;

            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            desc.fBytesPerRow = spec.width * bpp_4;
            desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

            std::vector<unsigned char> texels(desc.fBytesPerSlice);

            for (int y = 0; y < spec.height; y++) {
                for (int x = 0; x < spec.width; x++) {
                    const int t = spec.width * y + x;
                    texels[t*bpp_4]     = storage[t*bpp];
                    texels[t*bpp_4 + 1] = storage[t*bpp + 1];
                    texels[t*bpp_4 + 2] = storage[t*bpp + 2];
                    texels[t*bpp_4 + 3] = 255;
                }
            }

            texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        break;
    case GL_RGBA:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        }
        else {
            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    default:
        break;
    }

    return texture;
}

} //anonymous namespace

/*! \brief  Releases the reference to the shader owned by a smart pointer.
*/
void
HdVP2ShaderDeleter::operator()(MHWRender::MShaderInstance* shader)
{
    MRenderer* const renderer = MRenderer::theRenderer();
    const MShaderManager* const shaderMgr =
        renderer ? renderer->getShaderManager() : nullptr;
    if (TF_VERIFY(shaderMgr)) {
        shaderMgr->releaseShader(shader);
    }
}

/*! \brief  Releases the reference to the texture owned by a smart pointer.
*/
void
HdVP2TextureDeleter::operator()(MHWRender::MTexture* texture)
{
    MRenderer* const renderer = MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr =
        renderer ? renderer->getTextureManager() : nullptr;
    if (TF_VERIFY(textureMgr)) {
        textureMgr->releaseTexture(texture);
    }
}

/*! \brief  Constructor
*/
HdVP2Material::HdVP2Material(HdVP2RenderDelegate* renderDelegate, const SdfPath& id)
    : HdMaterial(id)
    , _renderDelegate(renderDelegate)
{
}

/*! \brief  Synchronize VP2 state with scene delegate state based on dirty bits
*/
void HdVP2Material::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* /*renderParam*/,
    HdDirtyBits* dirtyBits)
{
    if (*dirtyBits & (HdMaterial::DirtyResource | HdMaterial::DirtyParams)) {
        const SdfPath& id = GetId();
        VtValue vtMatResource = sceneDelegate->GetMaterialResource(id);

        if (vtMatResource.IsHolding<HdMaterialNetworkMap>()) {
            const HdMaterialNetworkMap& networkMap =
                vtMatResource.UncheckedGet<HdMaterialNetworkMap>();

            HdMaterialNetwork bxdfNet, dispNet;
            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->surface, &bxdfNet);
            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->displacement, &dispNet);

            if (*dirtyBits & HdMaterial::DirtyResource) {
                // Apply VP2 fixes to the material network
                HdMaterialNetwork vp2BxdfNet;
                _ApplyVP2Fixes(vp2BxdfNet, bxdfNet);

                // Create a shader instance for the material network.
                _surfaceShader.reset(_CreateShaderInstance(vp2BxdfNet));

                if (TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                    _PrintMaterialNetwork("BXDF", id, bxdfNet);
                    _PrintMaterialNetwork("BXDF (with VP2 fixes)", id, vp2BxdfNet);
                    _PrintMaterialNetwork("Displacement", id, dispNet);

                    if (_surfaceShader) {
                        auto tmpDir = boost::filesystem::temp_directory_path();
                        tmpDir /= "HdVP2Material_";
                        tmpDir += id.GetName();
                        tmpDir += ".txt";
                        _surfaceShader->writeEffectSourceToFile(tmpDir.c_str());

                        std::cout << "BXDF generated shader code for " << id << ":\n";
                        std::cout << "  " << tmpDir << "\n";
                    }
                }

                // Store primvar requirements.
                _requiredPrimvars = std::move(vp2BxdfNet.primvars);
            }

            _UpdateShaderInstance(bxdfNet);
        }
        else {
            TF_WARN("Expected material resource for <%s> to hold HdMaterialNetworkMap,"
                "but found %s instead.",
                id.GetText(), vtMatResource.GetTypeName().c_str());
        }
    }

    *dirtyBits = HdMaterial::Clean;
}

/*! \brief  Reload the shader
*/
void HdVP2Material::Reload() {
}

/*! \brief  Returns the minimal set of dirty bits to place in the
change tracker for use in the first sync of this prim.
*/
HdDirtyBits
HdVP2Material::GetInitialDirtyBitsMask() const {
    return HdMaterial::AllDirty;
}

/*! \brief  Creates a shader instance for the surface shader.
*/
MHWRender::MShaderInstance*
HdVP2Material::_CreateShaderInstance(const HdMaterialNetwork& mat) {
    MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
    if (!TF_VERIFY(renderer)) {
        return nullptr;
    }

    const MHWRender::MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr)) {
        return nullptr;
    }

    MHWRender::MShaderInstance* shaderInstance = nullptr;

    const auto rend = mat.nodes.rend();

    // Conditional compilation due to Maya API gaps.
#if MAYA_API_VERSION >= 20200000

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
            _surfaceShaderId = node.path;

            if (shaderInstance == nullptr) {
                TF_WARN("Failed to create shader instance for %s", nodeId.asChar());
                break;
            }

            continue;
        }

        MStringArray outputNames, inputNames;

        for (const HdMaterialRelationship& rel : mat.relationships) {
            if (rel.inputId == node.path) {
                MString outputName = rel.inputName.GetText();
                outputNames.append(outputName);

                if (rel.outputId != _surfaceShaderId) {
                    std::string str = rel.outputId.GetName();
                    str += rel.outputName.GetString();
                    inputNames.append(str.c_str());
                }
                else {
                    inputNames.append(rel.outputName.GetText());

                    if (rel.outputName == _tokens->opacity) {
                        shaderInstance->setIsTransparent(true);
                    }
                }
            }
        }

        if (outputNames.length() > 0) {
            MUintArray invalidParamIndices;
            MStatus status = shaderInstance->addInputFragmentForMultiParams(
                nodeId, nodeName, outputNames, inputNames, &invalidParamIndices);

            if (!status && TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                TF_WARN("Error %s happened when connecting shader %s",
                    status.errorString().asChar(), node.path.GetText());

                const unsigned int len = invalidParamIndices.length();
                for (unsigned int i = 0; i < len; i++) {
                    unsigned int index = invalidParamIndices[i];
                    const MString& outputName = outputNames[index];
                    const MString& inputName = inputNames[index];
                    TF_WARN("  %s -> %s", outputName.asChar(), inputName.asChar());
                }
            }

            if (_IsUsdPrimvarReader(node)) {
                auto it = node.parameters.find(_tokens->varname);
                if (it != node.parameters.end()) {
                    const MString paramName = HdTokens->primvar.GetText();
                    const MString varname = TfStringify(it->second).c_str();
                    shaderInstance->renameParameter(paramName, varname);
                }
            }
        }
        else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL).Msg("Failed to connect shader %s\n",
                node.path.GetText());
        }
    }

#elif MAYA_API_VERSION >= 20190000

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
            _surfaceShaderId = node.path;

            if (shaderInstance == nullptr) {
                TF_WARN("Failed to create shader instance for %s", nodeId.asChar());
                break;
            }

            continue;
        }

        MStringArray outputNames, inputNames;

        std::string primvarname;

        for (const HdMaterialRelationship& rel : mat.relationships) {
            if (rel.inputId == node.path) {
                outputNames.append(rel.inputName.GetText());
                inputNames.append(rel.outputName.GetText());

                if (rel.outputName == _tokens->opacity) {
                    shaderInstance->setIsTransparent(true);
                }
            }

            if (_IsUsdUVTexture(node)) {
                if (rel.outputId == node.path &&
                    rel.outputName == _tokens->st) {
                    for (const HdMaterialNode& n : mat.nodes) {
                        if (n.path == rel.inputId && _IsUsdPrimvarReader(n)) {
                            auto it = n.parameters.find(_tokens->varname);
                            if (it != n.parameters.end()) {
                                primvarname = TfStringify(it->second);
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Without multi-connection support for MShaderInstance, this code path
        // can only support common patterns of UsdShade material network, i.e.
        // a UsdUVTexture is connected to a single input of a USD Preview Surface.
        // More generic fix is coming.
        if (outputNames.length() == 1) {
            MStatus status = shaderInstance->addInputFragment(
                nodeId, outputNames[0], inputNames[0]);

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL).Msg(
                    "Error %s happened when connecting shader %s\n",
                    status.errorString().asChar(), node.path.GetText());
            }

            if (_IsUsdUVTexture(node)) {
                const MString paramNames[] = {
                    "file", "fileSampler", "isColorSpaceSRGB", "fallback", "scale", "bias"
                };

                for (const MString& paramName : paramNames) {
                    const MString resolvedName = nodeName + paramName;
                    shaderInstance->renameParameter(paramName, resolvedName);
                }

                const MString paramName = _tokens->st.GetText();
                shaderInstance->setSemantic(paramName, "uvCoord");
                shaderInstance->setAsVarying(paramName, true);
                shaderInstance->renameParameter(paramName, primvarname.c_str());
            }
        }
        else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL).Msg(
                "Failed to connect shader %s\n", node.path.GetText());
        }
    }

#endif

    return shaderInstance;
}

/*! \brief  Updates parameters for the surface shader.
*/
void HdVP2Material::_UpdateShaderInstance(const HdMaterialNetwork& mat)
{
    if (!_surfaceShader) {
        return;
    }

    for (const HdMaterialNode& node : mat.nodes) {
        const MString nodeName =
            node.path != _surfaceShaderId ? node.path.GetName().c_str() : "";

        MStatus samplerStatus = MStatus::kFailure;

        if (_IsUsdUVTexture(node)) {
            const MHWRender::MSamplerStateDesc desc = _GetSamplerStateDesc(node);
            const MHWRender::MSamplerState* sampler =
                _renderDelegate->GetSamplerState(desc);
            if (sampler) {
                const MString paramName = nodeName + "fileSampler";
                samplerStatus = _surfaceShader->setParameter(paramName, *sampler);
            }
        }

        for (auto const& entry: node.parameters) {
            const TfToken& token = entry.first;
            const VtValue& value = entry.second;

            MString paramName = nodeName + token.GetText();

            MStatus status = MStatus::kFailure;

            if (value.IsHolding<float>()) {
                const float& val = value.UncheckedGet<float>();
                status = _surfaceShader->setParameter(paramName, val);

                // The opacity parameter can be found and updated only when it
                // has no connection. In this case, transparency of the shader
                // is solely determined by the opacity value.
                if (nodeName.length() == 0 && token == _tokens->opacity) {
                    _surfaceShader->setIsTransparent(!status || val < 0.999f);
                }
            }
            else if (value.IsHolding<GfVec2f>()) {
                const float* val = value.UncheckedGet<GfVec2f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            }
            else if (value.IsHolding<GfVec3f>()) {
                const float* val = value.UncheckedGet<GfVec3f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            }
            else if (value.IsHolding<GfVec4f>()) {
                const float* val = value.UncheckedGet<GfVec4f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            }
            else if (value.IsHolding<TfToken>()) {
                // The two parameters have been converted to sampler state
                // before entering this loop.
                if (_IsUsdUVTexture(node) &&
                    (token == UsdHydraTokens->wrapS ||
                     token == UsdHydraTokens->wrapT)) {
                    status = samplerStatus;
                }
            }
            else if (value.IsHolding<SdfAssetPath>()) {
                const SdfAssetPath& val = value.UncheckedGet<SdfAssetPath>();
                const std::string& resolvedPath = val.GetResolvedPath();
                const std::string& assetPath = val.GetAssetPath();
                if (_IsUsdUVTexture(node) && token == _tokens->file) {
                    const HdVP2TextureInfo& info = _AcquireTexture(
                        !resolvedPath.empty() ? resolvedPath : assetPath);

                    MHWRender::MTextureAssignment assignment;
                    assignment.texture = info._texture.get();
                    status = _surfaceShader->setParameter(paramName, assignment);

                    if (status) {
                        paramName = nodeName + "isColorSpaceSRGB";
                        status = _surfaceShader->setParameter(paramName,
                            info._isColorSpaceSRGB);
                    }
                }
            }
            else if (value.IsHolding<int>()) {
                const int& val = value.UncheckedGet<int>();
                status = _surfaceShader->setParameter(paramName, val);
            }
            else if (value.IsHolding<bool>()) {
                const bool& val = value.UncheckedGet<bool>();
                status = _surfaceShader->setParameter(paramName, val);
            }
            else if (value.IsHolding<GfMatrix4d>()) {
                MMatrix matrix;
                value.UncheckedGet<GfMatrix4d>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
            }
            else if (value.IsHolding<GfMatrix4f>()) {
                MFloatMatrix matrix;
                value.UncheckedGet<GfMatrix4f>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
            }

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL).Msg(
                    "Failed to set shader parameter %s\n", paramName.asChar());
            }
        }
    }
}

/*! \brief  Acquires a texture for the given image path.
*/
const HdVP2TextureInfo&
HdVP2Material::_AcquireTexture(const std::string& path)
{
    const auto it = _textureMap.find(path);
    if (it != _textureMap.end()) {
        return it->second;
    }

    bool isSRGB = false;
    MHWRender::MTexture* texture = _LoadTexture(path, isSRGB);

    HdVP2TextureInfo& info = _textureMap[path];
    info._texture.reset(texture);
    info._isColorSpaceSRGB = isSRGB;
    return info;
}

PXR_NAMESPACE_CLOSE_SCOPE
