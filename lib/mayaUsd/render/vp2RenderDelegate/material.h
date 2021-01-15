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
#ifndef HD_VP2_MATERIAL
#define HD_VP2_MATERIAL

#include "shader.h"

#include <pxr/base/gf/vec2f.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/pxr.h>

#include <maya/MShaderManager.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2RenderDelegate;

/*! \brief  A deleter for MTexture, for use with smart pointers.
 */
struct HdVP2TextureDeleter
{
    void operator()(MHWRender::MTexture*);
};

/*! \brief  A MTexture owned by a std unique pointer.
 */
using HdVP2TextureUniquePtr = std::unique_ptr<MHWRender::MTexture, HdVP2TextureDeleter>;

/*! \brief  Information about the texture.
 */
struct HdVP2TextureInfo
{
    HdVP2TextureUniquePtr _texture;                    //!< Unique pointer of the texture
    GfVec2f               _stScale { 1.0f, 1.0f };     //!< UV scale for tiled textures
    GfVec2f               _stOffset { 0.0f, 0.0f };    //!< UV offset for tiled textures
    bool                  _isColorSpaceSRGB { false }; //!< Whether sRGB linearization is needed
};

/*! \brief  An unordered string-indexed map to cache texture information.
 */
using HdVP2TextureMap = std::unordered_map<std::string, HdVP2TextureInfo>;

/*! \brief  A VP2-specific implementation for a Hydra material prim.
    \class  HdVP2Material

    Provides a basic implementation of a Hydra material.
*/
class HdVP2Material final : public HdMaterial
{
public:
    HdVP2Material(HdVP2RenderDelegate*, const SdfPath&);

    //! Destructor.
    ~HdVP2Material() override = default;

    void Sync(HdSceneDelegate*, HdRenderParam*, HdDirtyBits*) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

#if USD_VERSION_NUM < 2011
    void Reload() override {};
#endif

    //! Get the surface shader instance.
    MHWRender::MShaderInstance* GetSurfaceShader() const { return _surfaceShader.get(); }

    //! Get primvar tokens required by this material.
    const TfTokenVector& GetRequiredPrimvars() const { return _requiredPrimvars; }

private:
    void _ApplyVP2Fixes(HdMaterialNetwork& outNet, const HdMaterialNetwork& inNet);
    MHWRender::MShaderInstance* _CreateShaderInstance(const HdMaterialNetwork& mat);
    void                        _UpdateShaderInstance(const HdMaterialNetwork& mat);
    const HdVP2TextureInfo&     _AcquireTexture(const std::string& path);

    HdVP2RenderDelegate* const
        _renderDelegate; //!< VP2 render delegate for which this material was created

    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>
        _nodePathMap; //!< Mapping from authored node paths to VP2-specific simplified pathes

    TfToken _surfaceNetworkToken; //!< Generated token to uniquely identify a material network

    HdVP2ShaderUniquePtr _surfaceShader;    //!< VP2 surface shader instance
    SdfPath              _surfaceShaderId;  //!< Path of the surface shader
    HdVP2TextureMap      _textureMap;       //!< Textures used by this material
    TfTokenVector        _requiredPrimvars; //!< primvars required by this material
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
