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
#include "draw_item.h"

#include "render_delegate.h"

#include <pxr/imaging/hd/mesh.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Constructor.

Data holder for its corresponding render item to facilitate parallelized evaluation.
*/
HdVP2DrawItem::HdVP2DrawItem(HdVP2RenderDelegate* delegate, const HdRprimSharedData* sharedData)
    : HdDrawItem(sharedData)
    , _delegate(delegate)
{
    // In the case of instancing, the ID of a proto has an attribute at the end,
    // we keep this info in _drawItemName so if needed we can extract proto ID
    // and use it to figure out Rprim path for each instance. For example:
    //
    //   "/Proxy/TreePatch/Tree_1.proto_leaves_id0"
    //
    _drawItemName = GetRprimID().GetText();
    _drawItemName += std::string(1, VP2_RENDER_DELEGATE_SEPARATOR).c_str();
    _drawItemName += TfStringPrintf("DrawItem_%p", this).c_str();
}

//! \brief  Destructor.
HdVP2DrawItem::~HdVP2DrawItem()
{
    if (_delegate) {
        auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
        MSubSceneContainer* subSceneContainer = param ? param->GetContainer() : nullptr;
        if (subSceneContainer) {
            for (const auto& renderItemData : _renderItems) {
                subSceneContainer->remove(renderItemData._renderItemName);
            }
        }
    }
}

HdVP2DrawItem::RenderItemData&
HdVP2DrawItem::AddRenderItem(MHWRender::MRenderItem* item, const HdGeomSubset* geomSubset)
{
    _renderItems.emplace_back();
    RenderItemData& renderItemData = _renderItems.back();

    renderItemData._renderItem = item;
    renderItemData._renderItemName = _drawItemName;
    if (geomSubset) {
        renderItemData._geomSubset = *geomSubset;
        renderItemData._renderItemName += std::string(1, VP2_RENDER_DELEGATE_SEPARATOR).c_str();
        renderItemData._renderItemName += geomSubset->id.GetString().c_str();
    }

    renderItemData._indexBuffer.reset(
        new MHWRender::MIndexBuffer(MHWRender::MGeometry::kUnsignedInt32));

    return renderItemData;
}

PXR_NAMESPACE_CLOSE_SCOPE
