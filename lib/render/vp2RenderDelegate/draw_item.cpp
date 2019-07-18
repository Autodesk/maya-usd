// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/imaging/hd/mesh.h"

#include "draw_item.h"
#include "render_delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Constructor.

Data holder for its corresponding render item to facilitate parallelized evaluation.

Position buffer is held by HdVP2Mesh and shared among its render items.
*/
HdVP2DrawItem::HdVP2DrawItem(
    HdVP2RenderDelegate* delegate,
    const HdRprimSharedData* sharedData,
    const HdMeshReprDesc& desc)
: HdDrawItem(sharedData)
, _delegate(delegate)
{
    // In the case of instancing, the ID of a proto has an attribute at the end,
    // we keep this info in _renderItemName so if needed we can extract proto ID
    // and use it to figure out Rprim path for each instance. For example:
    //
    //   "/Proxy/TreePatch/Tree_1.proto_leaves_id0"
    //
    std::string uniqueName = GetRprimID().GetText();
    uniqueName += TfStringPrintf("/DrawItem_%p", this);
    _renderItemName = uniqueName.c_str();

    _mesh._indexBuffer.reset(new MHWRender::MIndexBuffer(MHWRender::MGeometry::kUnsignedInt32));

    if (desc.geomStyle != HdMeshGeomStylePoints)
    {
        const MHWRender::MVertexBufferDescriptor nbDesc("",
            MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3);
        _mesh._normalsBuffer.reset(new MHWRender::MVertexBuffer(nbDesc));

        const MHWRender::MVertexBufferDescriptor uvBufferDesc(
            "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 2);
        _mesh._uvBuffer.reset(new MHWRender::MVertexBuffer(uvBufferDesc));
    }
}

//! \brief  Destructor.
HdVP2DrawItem::~HdVP2DrawItem() {
    if (_delegate) {
        auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
        MSubSceneContainer* subSceneContainer = param ? param->GetContainer() : nullptr;
        if(subSceneContainer) {
            subSceneContainer->remove(GetRenderItemName());
        }
    }
}

//! \brief  Get access to render item data.
HdVP2DrawItem::RenderItemData& HdVP2DrawItem::GetRenderItemData() {
    return _mesh;
}

PXR_NAMESPACE_CLOSE_SCOPE
