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
#include "UsdTransform3dBase.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

namespace USDUFE_NS_DEF {

// Ensure that UsdTransform3dBase is properly setup.
USDUFE_VERIFY_CLASS_SETUP(UsdTransform3dReadImpl, UsdTransform3dBase);
USDUFE_VERIFY_CLASS_BASE(Ufe::Transform3d, UsdTransform3dBase);

UsdTransform3dBase::UsdTransform3dBase(const UsdSceneItem::Ptr& item)
    : UsdTransform3dReadImpl(item)
    , Transform3d()
{
}

const Ufe::Path& UsdTransform3dBase::path() const { return UsdTransform3dReadImpl::path(); }

Ufe::SceneItem::Ptr UsdTransform3dBase::sceneItem() const
{
    return UsdTransform3dReadImpl::sceneItem();
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateCmd(double /* x */, double /* y */, double /* z */)
{
    return nullptr;
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dBase::rotateCmd(
    double /* x */,
    double /* y */,
    double /* z */
)
{
    return nullptr;
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dBase::scaleCmd(
    double /* x */,
    double /* y */,
    double /* z */
)
{
    return nullptr;
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::rotatePivotCmd(double x, double y, double z)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::rotatePivot() const
{
    // Called by DCC (such as Maya) during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::scalePivotCmd(double x, double y, double z)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::scalePivot() const
{
    // Called by DCC (such as Maya) during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateRotatePivotCmd(double, double, double)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::rotatePivotTranslation() const { return Ufe::Vector3d(0, 0, 0); }

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dBase::translateScalePivotCmd(double, double, double)
{
    return nullptr;
}

Ufe::Vector3d UsdTransform3dBase::scalePivotTranslation() const { return Ufe::Vector3d(0, 0, 0); }

Ufe::SetMatrix4dUndoableCommand::Ptr UsdTransform3dBase::setMatrixCmd(const Ufe::Matrix4d& m)
{
    return nullptr;
}

Ufe::Matrix4d UsdTransform3dBase::matrix() const { return UsdTransform3dReadImpl::matrix(); }

Ufe::Matrix4d UsdTransform3dBase::segmentInclusiveMatrix() const
{
    return UsdTransform3dReadImpl::segmentInclusiveMatrix();
}

Ufe::Matrix4d UsdTransform3dBase::segmentExclusiveMatrix() const
{
    return UsdTransform3dReadImpl::segmentExclusiveMatrix();
}

bool UsdTransform3dBase::isAttributeEditAllowed(const TfToken attrName) const
{
    return isAttributeEditAllowed(&attrName, 1);
}

bool UsdTransform3dBase::isAttributeEditAllowed(const TfToken* attrNames, int count) const
{
    OperationEditRouterContext editContext(EditRoutingTokens->RouteTransform, prim());
    std::string                errMsg;

    for (int i = 0; i < count; ++i) {
        const TfToken&       attrName = attrNames[i];
        PXR_NS::UsdAttribute attr;
        if (!attrName.IsEmpty())
            attr = prim().GetAttribute(attrName);
        if (attr && !UsdUfe::isAttributeEditAllowed(attr, &errMsg)) {
            displayMessage(MessageType::kError, errMsg.c_str());
            return false;
        } else if (!attr) {
            // If the attribute does not exist, we will need to edit the xformOpOrder
            // so check if we would be allowed to do that.
            UsdGeomXformable xformable(prim());
            if (!UsdUfe::isAttributeEditAllowed(xformable.GetXformOpOrderAttr(), &errMsg)) {
                displayMessage(MessageType::kError, errMsg.c_str());
                return false;
            }
        }
    }
    return true;
}

} // namespace USDUFE_NS_DEF
