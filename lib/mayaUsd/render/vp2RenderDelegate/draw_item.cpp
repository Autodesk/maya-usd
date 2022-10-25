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

#include <maya/MUserData.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
class HdVP2DrawItemUserData : public MUserData
{
public:
#if MAYA_API_VERSION >= 20220000
    HdVP2DrawItemUserData() { }
#else
    HdVP2DrawItemUserData()
        : MUserData(true)
    {
    }
#endif
    ~HdVP2DrawItemUserData() override { }

    int increaseUseCount() { return ++_useCount; }
    int decreaseUseCount() { return --_useCount; }

private:
    // A render item can be shared between Reprs. We need to track usage so it only gets removed
    // from the subscene once all references are gone.
    int _useCount = 0;
};

#if MAYA_API_VERSION >= 20220000
using HdVP2DrawItemUserDataPtr = MSharedPtr<HdVP2DrawItemUserData>;
#endif
} // namespace

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
#if MAYA_API_VERSION >= 20220000
                auto sharingData = HdVP2DrawItemUserDataPtr::dynamic_pointer_cast<>(
                    renderItemData._renderItem->getCustomData());
#else
                HdVP2DrawItemUserData* sharingData = dynamic_cast<HdVP2DrawItemUserData*>(
                    renderItemData._renderItem->customData());
#endif
                if (!sharingData || sharingData->decreaseUseCount() == 0) {
                    TF_VERIFY(renderItemData._renderItemName == renderItemData._renderItem->name());
                    subSceneContainer->remove(renderItemData._renderItem->name());
                }
            }
        }
    }
}

HdVP2DrawItem::RenderItemData&
HdVP2DrawItem::AddRenderItem(MHWRender::MRenderItem* item, const HdGeomSubset* geomSubset)
{
    TF_VERIFY(item);

    _renderItems.emplace_back();
    RenderItemData& renderItemData = _renderItems.back();

    renderItemData._renderItem = item;
    renderItemData._renderItemName = item->name();
#if MAYA_API_VERSION >= 20220000
    auto sharingData = HdVP2DrawItemUserDataPtr::dynamic_pointer_cast<>(item->getCustomData());
#else
    HdVP2DrawItemUserData* sharingData
        = dynamic_cast<HdVP2DrawItemUserData*>(renderItemData._renderItem->customData());
#endif
    if (!sharingData) {
        // create the custom data
#if MAYA_API_VERSION >= 20220000
        sharingData = HdVP2DrawItemUserDataPtr(new HdVP2DrawItemUserData());
#else
        sharingData = new HdVP2DrawItemUserData();
#endif
        item->setCustomData(sharingData);
    }
    sharingData->increaseUseCount();
    if (geomSubset) {
        renderItemData._geomSubset = *geomSubset;
    }

    renderItemData._indexBuffer.reset(
        new MHWRender::MIndexBuffer(MHWRender::MGeometry::kUnsignedInt32));

    return renderItemData;
}

/* static */
SdfPath HdVP2DrawItem::RenderItemToPrimPath(const MHWRender::MRenderItem& item)
{
    // Extract id of the owner Rprim. A SdfPath directly created from the render
    // item name could be ill-formed if the render item represents instancing:
    // "/TreePatch/Tree_1.proto_leaves_id0;DrawItem_xxxxxxxx". Thus std::string
    // is used instead to extract Rprim id.
    const std::string renderItemName = item.name().asChar();
    const auto        pos = renderItemName.find_first_of(VP2_RENDER_DELEGATE_SEPARATOR);
    return SdfPath(renderItemName.substr(0, pos));
}

PXR_NAMESPACE_CLOSE_SCOPE
