#ifndef HD_VP2_DRAW_ITEM
#define HD_VP2_DRAW_ITEM

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

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

    //! Helper struct providing storage for render item data
    struct RenderItemData {
        //! Render item normals buffer pointer - use when updating data
        std::unique_ptr<MHWRender::MVertexBuffer>   _normalsBuffer;
        //! Render item UV buffer pointer - use when updating data
        std::unique_ptr<MHWRender::MVertexBuffer>   _uvBuffer;
        //! Render item index buffer pointer - use when updating data
        std::unique_ptr<MHWRender::MIndexBuffer>    _indexBuffer;
        
        //! Number of instances currently allocated for render item
        unsigned int                                _instanceCount{ 0 };

        //! Per frame cache
        std::map<UsdTimeCode, CachedData>           _cache;

        //! Does this render item have valid UVs?
        bool    _hasUVs{ false };
    };

public:
    HdVP2DrawItem(HdVP2RenderDelegate* delegate, const HdRprimSharedData* sharedData, const HdMeshReprDesc& desc);

    ~HdVP2DrawItem();

    RenderItemData& GetRenderItemData();

    /*! \brief  Get render item name
    */
    const MString& GetRenderItemName() const { return _renderItemName; }

private:
    HdVP2RenderDelegate* _delegate{ nullptr };  //!< VP2 render delegate for which this draw item was created
    RenderItemData      _mesh;                  //!< VP2 render item data
    MString             _renderItemName;        //!< Unique name. Use this when searching for render item in subscene override container
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
