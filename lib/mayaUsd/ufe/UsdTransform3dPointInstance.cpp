//
// Copyright 2021 Autodesk
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
#include "UsdTransform3dPointInstance.h"

#include <mayaUsd/ufe/UsdPointInstanceOrientationModifier.h>
#include <mayaUsd/ufe/UsdPointInstancePositionModifier.h>
#include <mayaUsd/ufe/UsdPointInstanceScaleModifier.h>
#include <mayaUsd/ufe/UsdPointInstanceUndoableCommands.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdTransform3dBase.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/sceneItem.h>
#include <ufe/transform3d.h>
#include <ufe/transform3dHandler.h>
#include <ufe/transform3dUndoableCommands.h>
#include <ufe/types.h>

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dPointInstance::UsdTransform3dPointInstance(const UsdSceneItem::Ptr& item)
    : UsdTransform3dBase(item)
{
    if (item != nullptr) {
        const UsdPrim prim = item->prim();
        const int     instanceIndex = item->instanceIndex();
        _positionModifier.setPrimAndInstanceIndex(prim, instanceIndex);
        _orientationModifier.setPrimAndInstanceIndex(prim, instanceIndex);
        _scaleModifier.setPrimAndInstanceIndex(prim, instanceIndex);
    }
}

/* static */
UsdTransform3dPointInstance::Ptr UsdTransform3dPointInstance::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dPointInstance>(item);
}

/* override */
Ufe::Vector3d UsdTransform3dPointInstance::translation() const
{
    return _positionModifier.getUfeValue();
}

/* override */
Ufe::Vector3d UsdTransform3dPointInstance::rotation() const
{
    return _orientationModifier.getUfeValue();
}

/* override */
Ufe::Vector3d UsdTransform3dPointInstance::scale() const { return _scaleModifier.getUfeValue(); }

/* override */
Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dPointInstance::translateCmd(double x, double y, double z)
{
    return std::make_shared<UsdPointInstanceTranslateUndoableCommand>(
        path(), UsdTimeCode::Default());
}

/* override */
Ufe::RotateUndoableCommand::Ptr UsdTransform3dPointInstance::rotateCmd(double x, double y, double z)
{
    return std::make_shared<UsdPointInstanceRotateUndoableCommand>(path(), UsdTimeCode::Default());
}

/* override */
Ufe::ScaleUndoableCommand::Ptr UsdTransform3dPointInstance::scaleCmd(double x, double y, double z)
{
    return std::make_shared<UsdPointInstanceScaleUndoableCommand>(path(), UsdTimeCode::Default());
}

/* override */
Ufe::SetMatrix4dUndoableCommand::Ptr
UsdTransform3dPointInstance::setMatrixCmd(const Ufe::Matrix4d& m)
{
    TF_CODING_ERROR("Illegal call to unimplemented UsdTransform3dPointInstance::setMatrixCmd()");
    return nullptr;
}

/* override */
Ufe::Matrix4d UsdTransform3dPointInstance::matrix() const
{
    const GfVec3f position = _positionModifier.getUsdValue();
    const GfQuath rotation = _orientationModifier.getUsdValue();
    const GfVec3f scale = _scaleModifier.getUsdValue();

    GfTransform transform;

    transform.SetTranslation(position);
    transform.SetRotation(GfRotation(rotation));
    transform.SetScale(scale);

    const GfMatrix4d matrix = transform.GetMatrix();

    return toUfe(matrix);
}

/* override */
Ufe::Matrix4d UsdTransform3dPointInstance::segmentInclusiveMatrix() const
{
    Ufe::Matrix4d primMatrix = UsdTransform3dBase::segmentInclusiveMatrix();
    return primMatrix * matrix();
}

/* override */
Ufe::Matrix4d UsdTransform3dPointInstance::segmentExclusiveMatrix() const
{
    // The exclusive matrix of a point instance is simply the PointInstancer's
    // inclusive matrix.
    return UsdTransform3dBase::segmentInclusiveMatrix();
}

//------------------------------------------------------------------------------
// UsdTransform3dPointInstanceHandler
//------------------------------------------------------------------------------

UsdTransform3dPointInstanceHandler::UsdTransform3dPointInstanceHandler(
    const Ufe::Transform3dHandler::Ptr& nextHandler)
    : Ufe::Transform3dHandler()
    , _nextHandler(nextHandler)
{
}

/* static */
UsdTransform3dPointInstanceHandler::Ptr
UsdTransform3dPointInstanceHandler::create(const Ufe::Transform3dHandler::Ptr& nextHandler)
{
    return std::make_shared<UsdTransform3dPointInstanceHandler>(nextHandler);
}

/* override */
Ufe::Transform3d::Ptr
UsdTransform3dPointInstanceHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = downcast(item);

    if (!usdItem || !usdItem->isPointInstance()) {
        return _nextHandler->transform3d(item);
    }

    return UsdTransform3dPointInstance::create(usdItem);
}

/* override */
Ufe::Transform3d::Ptr UsdTransform3dPointInstanceHandler::editTransform3d(
    const Ufe::SceneItem::Ptr&      item,
    const Ufe::EditTransform3dHint& hint) const
{
    UsdSceneItem::Ptr usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    if (!usdItem->isPointInstance()) {
        return _nextHandler->editTransform3d(item, hint);
    }

    return UsdTransform3dPointInstance::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
