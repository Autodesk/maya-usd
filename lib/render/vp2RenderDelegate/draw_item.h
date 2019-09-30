//
// Copyright 2019 Autodesk, Inc. All rights reserved.
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

#ifndef HD_VP2_DRAW_ITEM
#define HD_VP2_DRAW_ITEM

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MHWGeometry.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMeshReprDesc;
class HdVP2RenderDelegate;

/*! \brief  Draw Item holds information necessary for accessing and updating VP2 render items
    \class  HdVP2DrawItem
*/
class HdVP2DrawItem final : public HdDrawItem
{
public:
    //! Helper struct providing storage for per frame cache data
    struct CachedData {
        MBoundingBox        _boundingBox;   //!< Bounding box cache
        VtArray<GfVec3f>    _normals;       //!< Normals cache
    };

    //! A primvar vertex buffer map indexed by primvar name.
    using PrimvarBufferMap = std::unordered_map<
        TfToken,
        std::unique_ptr<MHWRender::MVertexBuffer>,
        TfToken::HashFunctor
    >;

    //! Helper struct providing storage for render item data
    struct RenderItemData {
        //! Render item color buffer - use when updating data
        std::unique_ptr<MHWRender::MVertexBuffer>   _colorBuffer;
        //! Render item normals buffer - use when updating data
        std::unique_ptr<MHWRender::MVertexBuffer>   _normalsBuffer;
        //! Render item primvar buffers - use when updating data
        PrimvarBufferMap                            _primvarBuffers;
        //! Render item index buffer - use when updating data
        std::unique_ptr<MHWRender::MIndexBuffer>    _indexBuffer;

        //! Number of instances currently allocated for render item
        unsigned int                                _instanceCount{ 0 };

        //! Per frame cache
        std::map<UsdTimeCode, CachedData>           _cache;
    };

public:
    HdVP2DrawItem(HdVP2RenderDelegate* delegate, const HdRprimSharedData* sharedData, const HdMeshReprDesc& desc);

    ~HdVP2DrawItem();

    RenderItemData& GetRenderItemData();

    /*! \brief  Get render item name
    */
    const MString& GetRenderItemName() const { return _renderItemName; }

    /*! \brief  Whether the draw item is enabled.
    */
    bool IsEnabled() const { return _enabled; }

    /*! \brief  Enable or disable the draw item.
    */
    void Enable(bool v) { _enabled = v; }

    /*! \brief  Get the repr desc for which the draw item was created.
    */
    const HdMeshReprDesc& GetReprDesc() const { return _reprDesc; }

private:
    HdVP2RenderDelegate* _delegate{ nullptr };  //!< VP2 render delegate for which this draw item was created
    const HdMeshReprDesc _reprDesc;             //!< The repr desc for which the draw item was created.
    RenderItemData       _mesh;                 //!< VP2 render item data
    MString              _renderItemName;       //!< Unique name. Use this when searching for render item in subscene override container
    bool                 _enabled{ true };      //!< Whether the draw item is enabled.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
