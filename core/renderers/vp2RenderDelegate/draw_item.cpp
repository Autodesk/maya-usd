// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "draw_item.h"


PXR_NAMESPACE_OPEN_SCOPE

//! \brief  Constructor. Will allocate vertex, normals and index buffers for render item.
HdVP2DrawItem::HdVP2DrawItem(HdVP2RenderDelegate* delegate, const HdRprimSharedData* sharedData)
 : HdDrawItem(sharedData)
 , _delegate(delegate) {
    _renderItemId = GetRprimID().GetPrimPath().AppendChild(TfToken(TfStringPrintf("DrawItem_%p", this)));
    _renderItemName = MString(GetRenderItemId().GetText());

    {
        const MHWRender::MVertexBufferDescriptor
            vbDesc("", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);
        _mesh._positionsBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
    }

    _mesh._indexBuffer.reset(new MHWRender::MIndexBuffer(MHWRender::MGeometry::kUnsignedInt32));

    {
        const MHWRender::MVertexBufferDescriptor
            nbDesc("", MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3);
        _mesh._normalsBuffer.reset(new MHWRender::MVertexBuffer(nbDesc));
    }

    {
        const MHWRender::MVertexBufferDescriptor
            uvBufferDesc("", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 2);
        _mesh._uvBuffer.reset(new MHWRender::MVertexBuffer(uvBufferDesc));
    }
}

//! \brief  Destructor.
HdVP2DrawItem::~HdVP2DrawItem() {
}

//! \brief  Get access to render item data.
HdVP2DrawItem::RenderItemData& HdVP2DrawItem::GetRenderItemData() {
    return _mesh;
}

PXR_NAMESPACE_CLOSE_SCOPE
