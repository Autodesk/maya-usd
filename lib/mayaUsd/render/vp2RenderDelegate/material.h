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

#include <chrono>
#include <mutex>
#include <set>
#include <unordered_map>

// Workaround for a material consolidation update issue in VP2. Before USD 0.20.11, a Rprim will be
// recreated if its material has any change, so everything gets refreshed and the update issue gets
// masked. Once the update issue is fixed in VP2 we will disable this workaround.
#if PXR_VERSION >= 2011 && MAYA_API_VERSION >= 20210000 && MAYA_API_VERSION < 20230000
#define HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
#endif

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

using HdVP2TextureInfoSharedPtr = std::shared_ptr<HdVP2TextureInfo>;
using HdVP2TextureInfoWeakPtr = std::weak_ptr<HdVP2TextureInfo>;

/*! \brief  An unordered string-indexed map to cache texture information.

    Maya has a global internal texture map but we can't rely on it here, because we miss out
    on the extra information we store, such as _isColorSpaceSRGB. In HdVP2GlobalTextureMap we
    have that additional information.

    In order to correctly delete textures when they are no longer in use the global texture map
    holds only a weak_ptr to the HdVP2TextureInfo. The individual materials hold shared_ptrs to
    the textures they are using, so that when no materials are using a texture it'll be deleted.
 */
using HdVP2LocalTextureMap = std::unordered_map<std::string, HdVP2TextureInfoSharedPtr>;
using HdVP2GlobalTextureMap = std::unordered_map<std::string, HdVP2TextureInfoWeakPtr>;

/*! \brief  A VP2-specific implementation for a Hydra material prim.
    \class  HdVP2Material

    Provides a basic implementation of a Hydra material.
*/
class HdVP2Material final : public HdMaterial
{
public:
    HdVP2Material(HdVP2RenderDelegate*, const SdfPath&);

    //! Destructor.
    ~HdVP2Material() override;

    void Sync(HdSceneDelegate*, HdRenderParam*, HdDirtyBits*) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

#if PXR_VERSION < 2011
    void Reload() override {};
#endif

    //! Get the surface shader instance.
    MHWRender::MShaderInstance* GetSurfaceShader() const { return _surfaceShader.get(); }
    MHWRender::MShaderInstance* GetPointShader() const;

    //! Get primvar tokens required by this material.
    const TfTokenVector& GetRequiredPrimvars() const { return _requiredPrimvars; }

    void EnqueueLoadTextures();
    void ClearPendingTasks();

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
    //! The specified Rprim starts listening to changes on this material.
    void SubscribeForMaterialUpdates(const SdfPath& rprimId);

    //! The specified Rprim stops listening to changes on this material.
    void UnsubscribeFromMaterialUpdates(const SdfPath& rprimId);
#endif

    class TextureLoadingTask;
    friend class TextureLoadingTask;

    static void OnMayaExit();

private:
    void _ApplyVP2Fixes(HdMaterialNetwork& outNet, const HdMaterialNetwork& inNet);
#ifdef WANT_MATERIALX_BUILD
    void _ApplyMtlxVP2Fixes(HdMaterialNetwork2& outNet, const HdMaterialNetwork2& inNet);
    MHWRender::MShaderInstance* _CreateMaterialXShaderInstance(
        SdfPath const&            materialId,
        HdMaterialNetwork2 const& hdNetworkMap);
#endif
    MHWRender::MShaderInstance* _CreateShaderInstance(const HdMaterialNetwork& mat);
    void _UpdateShaderInstance(HdSceneDelegate* sceneDelegate, const HdMaterialNetwork& mat);
    const HdVP2TextureInfo& _AcquireTexture(
        HdSceneDelegate*      sceneDelegate,
        const std::string&    path,
        const HdMaterialNode& node);
    void _UpdateLoadedTexture(
        HdSceneDelegate*     sceneDelegate,
        const std::string&   path,
        MHWRender::MTexture* texture,
        bool                 isColorSpaceSRGB,
        const MFloatArray&   uvScaleOffset);

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
    //! Trigger sync on all Rprims which are listening to changes on this material.
    void _MaterialChanged(HdSceneDelegate* sceneDelegate);
#endif

    static void _ScheduleRefresh();

    static std::mutex                            _refreshMutex;
    static std::chrono::steady_clock::time_point _startTime;
    static std::atomic_size_t                    _runningTasksCounter;

    HdVP2RenderDelegate* const
        _renderDelegate; //!< VP2 render delegate for which this material was created

    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>
        _nodePathMap; //!< Mapping from authored node paths to VP2-specific simplified pathes

    TfToken _surfaceNetworkToken; //!< Generated token to uniquely identify a material network

    HdVP2ShaderUniquePtr         _surfaceShader;    //!< VP2 surface shader instance
    mutable HdVP2ShaderUniquePtr _pointShader;      //!< VP2 point shader instance, if needed
    SdfPath                      _surfaceShaderId;  //!< Path of the surface shader
    static HdVP2GlobalTextureMap _globalTextureMap; //!< Texture in use by all materials in MayaUSD
    HdVP2LocalTextureMap         _localTextureMap;  //!< Textures used by this material
    TfTokenVector                _requiredPrimvars; //!< primvars required by this material

    std::unordered_map<std::string, TextureLoadingTask*> _textureLoadingTasks;

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
    //! Mutex protecting concurrent access to the Rprim set
    std::mutex _materialSubscriptionsMutex;

    //! The set of Rprims listening to changes on this material
    std::set<SdfPath> _materialSubscriptions;
#endif

#ifdef WANT_MATERIALX_BUILD
    // MaterialX-only at the moment, but will be used for UsdPreviewSurface when the upgrade to
    // HdMaterialNetwork2 is complete.
    size_t _topoHash = 0;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
