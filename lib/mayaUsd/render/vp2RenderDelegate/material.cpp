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
#include "material.h"

#include "debugCodes.h"
#include "render_delegate.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFragmentManager.h>
#include <maya/MProfiler.h>
#include <maya/MShaderManager.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTextureManager.h>
#include <maya/MUintArray.h>
#include <maya/MViewport2Renderer.h>

#include <ghc/filesystem.hpp>

#include <iostream>
#include <string>

#if PXR_VERSION >= 2102
#include <pxr/imaging/hdSt/udimTextureObject.h>
#else
#include <pxr/imaging/glf/udimTexture.h>
#endif

#if PXR_VERSION >= 2102
#include <pxr/imaging/hio/image.h>
#else
#include <pxr/imaging/glf/image.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// clang-format off
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

    (UsdPrimvarReader_color)
    (UsdPrimvarReader_vector)
);
// clang-format on

//! Helper utility function to test whether a node is a UsdShade primvar reader.
bool _IsUsdPrimvarReader(const HdMaterialNode& node)
{
    const TfToken& id = node.identifier;
    return (
        id == UsdImagingTokens->UsdPrimvarReader_float
        || id == UsdImagingTokens->UsdPrimvarReader_float2
        || id == UsdImagingTokens->UsdPrimvarReader_float3
        || id == UsdImagingTokens->UsdPrimvarReader_float4
        || id == _tokens->UsdPrimvarReader_vector);
}

//! Helper utility function to test whether a node is a UsdShade UV texture.
inline bool _IsUsdUVTexture(const HdMaterialNode& node)
{
    return (node.identifier == UsdImagingTokens->UsdUVTexture);
}

//! Helper function to generate a XML string about nodes, relationships and primvars in the
//! specified material network.
std::string _GenerateXMLString(const HdMaterialNetwork& materialNetwork, bool includeParams = true)
{
    std::string result;

    if (ARCH_LIKELY(!materialNetwork.nodes.empty())) {
        // Reserve enough memory to avoid memory reallocation.
        result.reserve(1024);

        result += "<nodes>\n";

        if (includeParams) {
            for (const HdMaterialNode& node : materialNetwork.nodes) {
                result += "  <node path=\"";
                result += node.path.GetString();
                result += "\" id=\"";
                result += node.identifier;
                result += "\">\n";

                result += "    <params>\n";

                for (auto const& parameter : node.parameters) {
                    result += "      <param name=\"";
                    result += parameter.first;
                    result += "\" value=\"";
                    result += TfStringify(parameter.second);
                    result += "\"/>\n";
                }

                result += "    </params>\n";

                result += "  </node>\n";
            }
        } else {
            for (const HdMaterialNode& node : materialNetwork.nodes) {
                result += "  <node path=\"";
                result += node.path.GetString();
                result += "\" id=\"";
                result += node.identifier;
                result += "\"/>\n";
            }
        }

        result += "</nodes>\n";

        if (!materialNetwork.relationships.empty()) {
            result += "<relationships>\n";

            for (const HdMaterialRelationship& rel : materialNetwork.relationships) {
                result += "  <rel from=\"";
                result += rel.inputId.GetString();
                result += ".";
                result += rel.inputName;
                result += "\" to=\"";
                result += rel.outputId.GetString();
                result += ".";
                result += rel.outputName;
                result += "\"/>\n";
            }

            result += "</relationships>\n";
        }

        if (!materialNetwork.primvars.empty()) {
            result += "<primvars>\n";

            for (TfToken const& primvar : materialNetwork.primvars) {
                result += "  <primvar name=\"";
                result += primvar;
                result += "\"/>\n";
            }

            result += "</primvars>\n";
        }
    }

    return result;
}

//! Return true if the surface shader has its opacity attribute connected to a node which isn't
//! a USD primvar reader.
bool _IsTransparent(const HdMaterialNetwork& network)
{
    const HdMaterialNode& surfaceShader = network.nodes.back();

    for (const HdMaterialRelationship& rel : network.relationships) {
        if (rel.outputName == _tokens->opacity && rel.outputId == surfaceShader.path) {
            for (const HdMaterialNode& node : network.nodes) {
                if (node.path == rel.inputId) {
                    return !_IsUsdPrimvarReader(node);
                }
            }
        }
    }

    return false;
}

//! Helper utility function to convert Hydra texture addressing token to VP2 enum.
MHWRender::MSamplerState::TextureAddress _ConvertToTextureSamplerAddressEnum(const TfToken& token)
{
    MHWRender::MSamplerState::TextureAddress address;

    if (token == UsdHydraTokens->clamp) {
        address = MHWRender::MSamplerState::kTexClamp;
    } else if (token == UsdHydraTokens->mirror) {
        address = MHWRender::MSamplerState::kTexMirror;
    } else if (token == UsdHydraTokens->black) {
        address = MHWRender::MSamplerState::kTexBorder;
    } else {
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

MHWRender::MTexture*
_LoadUdimTexture(const std::string& path, bool& isColorSpaceSRGB, MFloatArray& uvScaleOffset)
{
    /*
        For this method to work path needs to be an absolute file path, not an asset path.
        That means that this function depends on the changes in 4e426565 to materialAdapther.cpp
        to work. As of my writing this 4e426565 is not in the USD that MayaUSD normally builds
        against so this code will fail, because UsdImaging_GetUdimTiles won't file the tiles
        because we don't know where on disk to look for them.

        https://github.com/PixarAnimationStudios/USD/commit/4e42656543f4e3a313ce31a81c27477d4dcb64b9
    */

    // test for a UDIM texture
#if PXR_VERSION >= 2102
    if (!HdStIsSupportedUdimTexture(path))
        return nullptr;
#else
    if (!GlfIsSupportedUdimTexture(path))
        return nullptr;
#endif

    /*
        Maya's tiled texture support is implemented quite differently from Usd's UDIM support.
        In Maya the texture tiles get combined into a single big texture, downscaling each tile
        if necessary, and filling in empty regions of a non-square tile with the undefined color.

        In USD the UDIM textures are stored in a texture array that the shader uses to draw.
    */

    MHWRender::MRenderer* const       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
    if (!TF_VERIFY(textureMgr)) {
        return nullptr;
    }

    // HdSt sets the tile limit to the max number of textures in an array of 2d textures. OpenGL
    // says the minimum number of layers in 2048 so I'll use that.
    int                                   tileLimit = 2048;
    std::vector<std::tuple<int, TfToken>> tiles = UsdImaging_GetUdimTiles(path, tileLimit);
    if (tiles.size() == 0) {
        TF_WARN("Unable to find UDIM tiles for %s", path.c_str());
        return nullptr;
    }

    // I don't think there is a downside to setting a very high limit.
    // Maya will clamp the texture size to the VP2 texture clamp resolution and the hardware's
    // max texture size. And Maya doesn't make the tiled texture unnecessarily large. When I
    // try loading two 1k textures I end up with a tiled texture that is 2k x 1k.
    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    renderer->GPUmaximumOutputTargetSize(maxWidth, maxHeight);

    // Open the first image and get it's resolution. Assuming that all the tiles have the same
    // resolution, warn the user if Maya's tiled texture implementation is going to result in
    // a loss of texture data.
    {
#if PXR_VERSION >= 2102
        HioImageSharedPtr image = HioImage::OpenForReading(std::get<1>(tiles[0]).GetString());
#else
        GlfImageSharedPtr image = GlfImage::OpenForReading(std::get<1>(tiles[0]).GetString());
#endif
        if (!TF_VERIFY(image)) {
            return nullptr;
        }
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        unsigned int tileWidth = image->GetWidth();
        unsigned int tileHeight = image->GetHeight();

        int maxTileId = std::get<0>(tiles.back());
        int maxU = maxTileId % 10;
        int maxV = (maxTileId - maxU) / 10;
        if ((tileWidth * maxU > maxWidth) || (tileHeight * maxV > maxHeight))
            TF_WARN(
                "UDIM texture %s creates a tiled texture larger than the maximum texture size. Some"
                "resolution will be lost.",
                path.c_str());
    }

    MString textureName(
        path.c_str()); // used for caching, using the string with <UDIM> in it is fine
    MStringArray tilePaths;
    MFloatArray  tilePositions;
    for (auto& tile : tiles) {
        tilePaths.append(MString(std::get<1>(tile).GetText()));

#if PXR_VERSION >= 2102
        HioImageSharedPtr image = HioImage::OpenForReading(std::get<1>(tile).GetString());
#else
        GlfImageSharedPtr image = GlfImage::OpenForReading(std::get<1>(tile).GetString());
#endif
        if (!TF_VERIFY(image)) {
            return nullptr;
        }
        if (isColorSpaceSRGB != image->IsColorSpaceSRGB()) {
            TF_WARN(
                "UDIM texture %s color space doesn't match %s color space",
                std::get<1>(tile).GetText(),
                std::get<1>(tiles[0]).GetText());
        }

        // The image labeled 1001 will have id 0, 1002 will have id 1, 1011 will have id 10.
        // image 1001 starts with UV (0.0f, 0.0f), 1002 is (1.0f, 0.0f) and 1011 is (0.0f, 1.0f)
        int   tileId = std::get<0>(tile);
        float u = (float)(tileId % 10);
        float v = (float)((tileId - u) / 10);
        tilePositions.append(u);
        tilePositions.append(v);
    }

    MColor               undefinedColor(0.0f, 1.0f, 0.0f, 1.0f);
    MStringArray         failedTilePaths;
    MHWRender::MTexture* texture = textureMgr->acquireTiledTexture(
        textureName,
        tilePaths,
        tilePositions,
        undefinedColor,
        maxWidth,
        maxHeight,
        failedTilePaths,
        uvScaleOffset);

    for (unsigned int i = 0; i < failedTilePaths.length(); i++) {
        TF_WARN("Failed to load <UDIM> texture tile %s", failedTilePaths[i].asChar());
    }

    return texture;
}

//! Load texture from the specified path
MHWRender::MTexture*
_LoadTexture(const std::string& path, bool& isColorSpaceSRGB, MFloatArray& uvScaleOffset)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "LoadTexture", path.c_str());

    // If it is a UDIM texture we need to modify the path before calling OpenForReading
#if PXR_VERSION >= 2102
    if (HdStIsSupportedUdimTexture(path))
        return _LoadUdimTexture(path, isColorSpaceSRGB, uvScaleOffset);
#else
    if (GlfIsSupportedUdimTexture(path))
        return _LoadUdimTexture(path, isColorSpaceSRGB, uvScaleOffset);
#endif

    MHWRender::MRenderer* const       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
    if (!TF_VERIFY(textureMgr)) {
        return nullptr;
    }

#if PXR_VERSION >= 2102
    HioImageSharedPtr image = HioImage::OpenForReading(path);
#else
    GlfImageSharedPtr     image = GlfImage::OpenForReading(path);
#endif
    if (!TF_VERIFY(image)) {
        return nullptr;
    }

    // This image is used for loading pixel data from usdz only and should
    // not trigger any OpenGL call. VP2RenderDelegate will transfer the
    // texels to GPU memory with VP2 API which is 3D API agnostic.
#if PXR_VERSION >= 2102
    HioImage::StorageSpec spec;
#else
    GlfImage::StorageSpec spec;
#endif
    spec.width = image->GetWidth();
    spec.height = image->GetHeight();
    spec.depth = 1;
#if PXR_VERSION >= 2102
    spec.format = image->GetFormat();
#elif PXR_VERSION > 2008
    spec.hioFormat = image->GetHioFormat();
#else
    spec.format = image->GetFormat();
    spec.type = image->GetType();
#endif
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

#if PXR_VERSION > 2008
#if PXR_VERSION >= 2102
    switch (spec.format) {
#else
    switch (spec.hioFormat) {
#endif
    // Single Channel
    case HioFormatFloat32:
        desc.fFormat = MHWRender::kR32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8:
        desc.fFormat = MHWRender::kR8_UNORM;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;

    // 3-Channel
    case HioFormatFloat32Vec3:
        desc.fFormat = MHWRender::kR32G32B32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8Vec3:
    case HioFormatUNorm8Vec3srgb: {
        // R8G8B8 is not supported by VP2. Converted to R8G8B8A8.
        constexpr int bpp_4 = 4;

        desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
        desc.fBytesPerRow = spec.width * bpp_4;
        desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

        std::vector<unsigned char> texels(desc.fBytesPerSlice);

        for (int y = 0; y < spec.height; y++) {
            for (int x = 0; x < spec.width; x++) {
                const int t = spec.width * y + x;
                texels[t * bpp_4] = storage[t * bpp];
                texels[t * bpp_4 + 1] = storage[t * bpp + 1];
                texels[t * bpp_4 + 2] = storage[t * bpp + 2];
                texels[t * bpp_4 + 3] = 255;
            }
        }

        texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        break;
    }

    // 4-Channel
    case HioFormatFloat32Vec4:
        desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8Vec4:
    case HioFormatUNorm8Vec4srgb:
        desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    default: break;
    }
#else
    switch (spec.format) {
    case GL_RED:
        desc.fFormat = (spec.type == GL_FLOAT ? MHWRender::kR32_FLOAT : MHWRender::kR8_UNORM);
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case GL_RGB:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32_FLOAT;
            texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        } else {
            // R8G8B8 is not supported by VP2. Converted to R8G8B8A8.
            constexpr int bpp_4 = 4;

            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            desc.fBytesPerRow = spec.width * bpp_4;
            desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

            std::vector<unsigned char> texels(desc.fBytesPerSlice);

            for (int y = 0; y < spec.height; y++) {
                for (int x = 0; x < spec.width; x++) {
                    const int t = spec.width * y + x;
                    texels[t * bpp_4] = storage[t * bpp];
                    texels[t * bpp_4 + 1] = storage[t * bpp + 1];
                    texels[t * bpp_4 + 2] = storage[t * bpp + 2];
                    texels[t * bpp_4 + 3] = 255;
                }
            }

            texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        break;
    case GL_RGBA:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        } else {
            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    default: break;
    }
#endif

    return texture;
}

} // anonymous namespace

/*! \brief  Releases the reference to the texture owned by a smart pointer.
 */
void HdVP2TextureDeleter::operator()(MHWRender::MTexture* texture)
{
    MRenderer* const                  renderer = MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
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

        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            "HdVP2Material::Sync",
            id.GetText());

        VtValue vtMatResource = sceneDelegate->GetMaterialResource(id);

        if (vtMatResource.IsHolding<HdMaterialNetworkMap>()) {
            const HdMaterialNetworkMap& networkMap
                = vtMatResource.UncheckedGet<HdMaterialNetworkMap>();

            HdMaterialNetwork bxdfNet, dispNet, vp2BxdfNet;

            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->surface, &bxdfNet);
            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->displacement, &dispNet);

            _ApplyVP2Fixes(vp2BxdfNet, bxdfNet);

            // Generate a XML string from the material network and convert it to a token for faster
            // hashing and comparison.
            const TfToken token(_GenerateXMLString(vp2BxdfNet, false));

            // Skip creating a new shader instance if the token is unchanged. There is no plan to
            // implement fine-grain dirty bit in Hydra for the same purpose:
            // https://groups.google.com/g/usd-interest/c/xytT2azlJec/m/22Tnw4yXAAAJ
            if (_surfaceNetworkToken != token) {
                MProfilingScope subProfilingScope(
                    HdVP2RenderDelegate::sProfilerCategory,
                    MProfiler::kColorD_L2,
                    "CreateShaderInstance");

                // Remember the path of the surface shader for special handling: unlike other
                // fragments, the parameters of the surface shader fragment can't be renamed.
                _surfaceShaderId = vp2BxdfNet.nodes.back().path;

                MHWRender::MShaderInstance* shader;

#ifndef HDVP2_DISABLE_SHADER_CACHE
                // Acquire a shader instance from the shader cache. If a shader instance has
                // been cached with the same token, a clone of the shader instance will be
                // returned. Multiple clones of a shader instance will share the same shader
                // effect, thus reduce compilation overhead and enable material consolidation.
                shader = _renderDelegate->GetShaderFromCache(token);

                // If the shader instance is not found in the cache, create one from the material
                // network and add a clone to the cache for reuse.
                if (!shader) {
                    shader = _CreateShaderInstance(vp2BxdfNet);

                    if (shader) {
                        _renderDelegate->AddShaderToCache(token, *shader);
                    }
                }
#else
                shader = _CreateShaderInstance(vp2BxdfNet);
#endif

                // The shader instance is owned by the material solely.
                _surfaceShader.reset(shader);

                if (TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                    std::cout << "BXDF material network for " << id << ":\n"
                              << _GenerateXMLString(bxdfNet) << "\n"
                              << "BXDF (with VP2 fixes) material network for " << id << ":\n"
                              << _GenerateXMLString(vp2BxdfNet) << "\n"
                              << "Displacement material network for " << id << ":\n"
                              << _GenerateXMLString(dispNet) << "\n";

                    if (_surfaceShader) {
                        auto tmpDir = ghc::filesystem::temp_directory_path();
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

                // The token is saved and will be used to determine whether a new shader instance
                // is needed during the next sync.
                _surfaceNetworkToken = token;

                // If the surface shader has its opacity attribute connected to a node which isn't
                // a primvar reader, it is set as transparent. If the opacity attr is connected to
                // a primvar reader, the Rprim side will determine the transparency state
                // according to the primvars:displayOpacity data. If the opacity attr isn't
                // connected, the transparency state will be set in _UpdateShaderInstance()
                // according to the opacity value.
                if (shader) {
                    shader->setIsTransparent(_IsTransparent(bxdfNet));
                }
            }

            _UpdateShaderInstance(bxdfNet);

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
            _MaterialChanged(sceneDelegate);
#endif
        } else {
            TF_WARN(
                "Expected material resource for <%s> to hold HdMaterialNetworkMap,"
                "but found %s instead.",
                id.GetText(),
                vtMatResource.GetTypeName().c_str());
        }
    }

    *dirtyBits = HdMaterial::Clean;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2Material::GetInitialDirtyBitsMask() const { return HdMaterial::AllDirty; }

/*! \brief  Applies VP2-specific fixes to the material network.
 */
void HdVP2Material::_ApplyVP2Fixes(HdMaterialNetwork& outNet, const HdMaterialNetwork& inNet)
{
    // To avoid relocation, reserve enough space for possible maximal size. The
    // output network is temporary C++ object that will be released after use.
    const size_t numNodes = inNet.nodes.size();
    const size_t numRelationships = inNet.relationships.size();

    size_t nodeCounter = 0;

    _nodePathMap.clear();
    _nodePathMap.reserve(numNodes);

    HdMaterialNetwork tmpNet;
    tmpNet.nodes.reserve(numNodes);
    tmpNet.relationships.reserve(numRelationships);

    // Replace the authored node paths with simplified paths in the form of "node#". By doing so
    // we will be able to reuse shader effects among material networks which have the same node
    // identifiers and relationships but different node paths, reduce shader compilation overhead
    // and enable material consolidation for faster rendering.
    for (const HdMaterialNode& node : inNet.nodes) {
        tmpNet.nodes.push_back(node);

        HdMaterialNode& outNode = tmpNet.nodes.back();
        outNode.path = SdfPath(outNode.identifier.GetString() + std::to_string(++nodeCounter));

        _nodePathMap[node.path] = outNode.path;
    }

    // Update the relationships to use the new node paths.
    for (const HdMaterialRelationship& rel : inNet.relationships) {
        tmpNet.relationships.push_back(rel);

        HdMaterialRelationship& outRel = tmpNet.relationships.back();
        outRel.inputId = _nodePathMap[outRel.inputId];
        outRel.outputId = _nodePathMap[outRel.outputId];
    }

    outNet.nodes.reserve(numNodes + numRelationships);
    outNet.relationships.reserve(numRelationships * 2);
    outNet.primvars.reserve(numNodes);

    // Add passthrough nodes for vector component access.
    for (const HdMaterialNode& node : tmpNet.nodes) {
        TfToken primvarToRead;

        const bool isUsdPrimvarReader = _IsUsdPrimvarReader(node);
        if (isUsdPrimvarReader) {
            auto it = node.parameters.find(_tokens->varname);
            if (it != node.parameters.end()) {
                primvarToRead = TfToken(TfStringify(it->second));
            }
        }

        outNet.nodes.push_back(node);

        // If the primvar reader is reading color or opacity, replace it with
        // UsdPrimvarReader_color which can create COLOR stream requirement
        // instead of generic TEXCOORD stream.
        if (primvarToRead == HdTokens->displayColor || primvarToRead == HdTokens->displayOpacity) {
            HdMaterialNode& nodeToChange = outNet.nodes.back();
            nodeToChange.identifier = _tokens->UsdPrimvarReader_color;
        }

        // Copy outgoing connections and if needed add passthrough node/connection.
        for (const HdMaterialRelationship& rel : tmpNet.relationships) {
            if (rel.inputId != node.path) {
                continue;
            }

            TfToken passThroughId;
            if (rel.inputName == _tokens->rgb || rel.inputName == _tokens->xyz) {
                passThroughId = _tokens->Float4ToFloat3;
            } else if (rel.inputName == _tokens->r || rel.inputName == _tokens->x) {
                passThroughId = _tokens->Float4ToFloatX;
            } else if (rel.inputName == _tokens->g || rel.inputName == _tokens->y) {
                passThroughId = _tokens->Float4ToFloatY;
            } else if (rel.inputName == _tokens->b || rel.inputName == _tokens->z) {
                passThroughId = _tokens->Float4ToFloatZ;
            } else if (rel.inputName == _tokens->a || rel.inputName == _tokens->w) {
                passThroughId = _tokens->Float4ToFloatW;
            } else if (primvarToRead == HdTokens->displayColor) {
                passThroughId = _tokens->Float4ToFloat3;
            } else if (primvarToRead == HdTokens->displayOpacity) {
                passThroughId = _tokens->Float4ToFloatW;
            } else {
                outNet.relationships.push_back(rel);
                continue;
            }

            const SdfPath passThroughPath(
                passThroughId.GetString() + std::to_string(++nodeCounter));

            const HdMaterialNode passThroughNode = { passThroughPath, passThroughId, {} };
            outNet.nodes.push_back(passThroughNode);

            HdMaterialRelationship newRel
                = { rel.inputId, _tokens->output, passThroughPath, _tokens->input };
            outNet.relationships.push_back(newRel);

            newRel = { passThroughPath, _tokens->output, rel.outputId, rel.outputName };
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
        else if (isUsdPrimvarReader) {
            if (!primvarToRead.IsEmpty()) {
                outNet.primvars.push_back(primvarToRead);
            }
        }
    }
}

/*! \brief  Creates a shader instance for the surface shader.
 */
MHWRender::MShaderInstance* HdVP2Material::_CreateShaderInstance(const HdMaterialNetwork& mat)
{
    MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
    if (!TF_VERIFY(renderer)) {
        return nullptr;
    }

    const MHWRender::MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr)) {
        return nullptr;
    }

    MHWRender::MShaderInstance* shaderInstance = nullptr;

    // MShaderInstance supports multiple connections between shaders on Maya 2018.7, 2019.3, 2020
    // and above.
#if (MAYA_API_VERSION >= 20190300) \
    || ((MAYA_API_VERSION >= 20180700) && (MAYA_API_VERSION < 20190000))

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    const auto rend = mat.nodes.rend();
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
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
                } else {
                    inputNames.append(rel.outputName.GetText());
                }
            }
        }

        if (outputNames.length() > 0) {
            MUintArray invalidParamIndices;
            MStatus    status = shaderInstance->addInputFragmentForMultiParams(
                nodeId, nodeName, outputNames, inputNames, &invalidParamIndices);

            if (!status && TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                TF_WARN(
                    "Error %s happened when connecting shader %s",
                    status.errorString().asChar(),
                    node.path.GetText());

                const unsigned int len = invalidParamIndices.length();
                for (unsigned int i = 0; i < len; i++) {
                    unsigned int   index = invalidParamIndices[i];
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
        } else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                .Msg("Failed to connect shader %s\n", node.path.GetText());
        }
    }

#elif MAYA_API_VERSION >= 20190000

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    const auto rend = mat.nodes.rend();
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
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
            }

            if (_IsUsdUVTexture(node)) {
                if (rel.outputId == node.path && rel.outputName == _tokens->st) {
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
            MStatus status
                = shaderInstance->addInputFragment(nodeId, outputNames[0], inputNames[0]);

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg(
                        "Error %s happened when connecting shader %s\n",
                        status.errorString().asChar(),
                        node.path.GetText());
            }

            if (_IsUsdUVTexture(node)) {
                const MString paramNames[]
                    = { "file", "fileSampler", "isColorSpaceSRGB", "fallback", "scale", "bias" };

                for (const MString& paramName : paramNames) {
                    const MString resolvedName = nodeName + paramName;
                    shaderInstance->renameParameter(paramName, resolvedName);
                }

                const MString paramName = _tokens->st.GetText();
                shaderInstance->setSemantic(paramName, "uvCoord");
                shaderInstance->setAsVarying(paramName, true);
                shaderInstance->renameParameter(paramName, primvarname.c_str());
            }
        } else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                .Msg("Failed to connect shader %s\n", node.path.GetText());

            if (outputNames.length() > 1) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg("MShaderInstance doesn't support "
                         "multiple connections between shaders on the current Maya version.\n");
            }
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

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "UpdateShaderInstance");

    for (const HdMaterialNode& node : mat.nodes) {
        // Find the simplified path for the authored node path from the map which has been created
        // when applying VP2-specific fixes.
        const auto it = _nodePathMap.find(node.path);
        if (it == _nodePathMap.end()) {
            continue;
        }

        // The simplified path has only one token which is the node name.
        const SdfPath& nodePath = it->second;
        const MString  nodeName(nodePath != _surfaceShaderId ? nodePath.GetText() : "");

        MStatus samplerStatus = MStatus::kFailure;

        if (_IsUsdUVTexture(node)) {
            const MHWRender::MSamplerStateDesc desc = _GetSamplerStateDesc(node);
            const MHWRender::MSamplerState*    sampler = _renderDelegate->GetSamplerState(desc);
            if (sampler) {
                const MString paramName = nodeName + "fileSampler";
                samplerStatus = _surfaceShader->setParameter(paramName, *sampler);
            }
        }

        for (auto const& entry : node.parameters) {
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
                if (status && nodeName.length() == 0 && token == _tokens->opacity) {
                    _surfaceShader->setIsTransparent(val < 0.999f);
                }
            } else if (value.IsHolding<GfVec2f>()) {
                const float* val = value.UncheckedGet<GfVec2f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfVec3f>()) {
                const float* val = value.UncheckedGet<GfVec3f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfVec4f>()) {
                const float* val = value.UncheckedGet<GfVec4f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<TfToken>()) {
                // The two parameters have been converted to sampler state
                // before entering this loop.
                if (_IsUsdUVTexture(node)
                    && (token == UsdHydraTokens->wrapS || token == UsdHydraTokens->wrapT)) {
                    status = samplerStatus;
                }
            } else if (value.IsHolding<SdfAssetPath>()) {
                const SdfAssetPath& val = value.UncheckedGet<SdfAssetPath>();
                const std::string&  resolvedPath = val.GetResolvedPath();
                const std::string&  assetPath = val.GetAssetPath();
                if (_IsUsdUVTexture(node) && token == _tokens->file) {
                    const HdVP2TextureInfo& info
                        = _AcquireTexture(!resolvedPath.empty() ? resolvedPath : assetPath);

                    MHWRender::MTextureAssignment assignment;
                    assignment.texture = info._texture.get();
                    status = _surfaceShader->setParameter(paramName, assignment);

                    if (status) {
                        paramName = nodeName + "isColorSpaceSRGB";
                        status = _surfaceShader->setParameter(paramName, info._isColorSpaceSRGB);
                    }
                    if (status) {
                        paramName = nodeName + "stScale";
                        status = _surfaceShader->setParameter(paramName, info._stScale.data());
                    }
                    if (status) {
                        paramName = nodeName + "stOffset";
                        status = _surfaceShader->setParameter(paramName, info._stOffset.data());
                    }
                }
            } else if (value.IsHolding<int>()) {
                const int& val = value.UncheckedGet<int>();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<bool>()) {
                const bool& val = value.UncheckedGet<bool>();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfMatrix4d>()) {
                MMatrix matrix;
                value.UncheckedGet<GfMatrix4d>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
            } else if (value.IsHolding<GfMatrix4f>()) {
                MFloatMatrix matrix;
                value.UncheckedGet<GfMatrix4f>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
            }

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg("Failed to set shader parameter %s\n", paramName.asChar());
            }
        }
    }
}

/*! \brief  Acquires a texture for the given image path.
 */
const HdVP2TextureInfo& HdVP2Material::_AcquireTexture(const std::string& path)
{
    const auto it = _textureMap.find(path);
    if (it != _textureMap.end()) {
        return it->second;
    }

    bool                 isSRGB = false;
    MFloatArray          uvScaleOffset;
    MHWRender::MTexture* texture = _LoadTexture(path, isSRGB, uvScaleOffset);

    HdVP2TextureInfo& info = _textureMap[path];
    info._texture.reset(texture);
    info._isColorSpaceSRGB = isSRGB;
    if (uvScaleOffset.length() > 0) {
        TF_VERIFY(uvScaleOffset.length() == 4);
        info._stScale.Set(uvScaleOffset[0], uvScaleOffset[1]); // The first 2 elements are the scale
        info._stOffset.Set(
            uvScaleOffset[2], uvScaleOffset[3]); // The next two elements are the offset
    }
    return info;
}

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND

void HdVP2Material::SubscribeForMaterialUpdates(const SdfPath& rprimId)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    _materialSubscriptions.insert(rprimId);
}

void HdVP2Material::UnsubscribeFromMaterialUpdates(const SdfPath& rprimId)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    _materialSubscriptions.erase(rprimId);
}

void HdVP2Material::_MaterialChanged(HdSceneDelegate* sceneDelegate)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    HdChangeTracker& changeTracker = sceneDelegate->GetRenderIndex().GetChangeTracker();
    for (const SdfPath& rprimId : _materialSubscriptions) {
        changeTracker.MarkRprimDirty(rprimId, HdChangeTracker::DirtyMaterialId);
    }
}

#endif

PXR_NAMESPACE_CLOSE_SCOPE
