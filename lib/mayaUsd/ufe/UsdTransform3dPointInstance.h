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
#ifndef MAYAUSD_UFE_USD_TRANSFORM3D_POINT_INSTANCE_H
#define MAYAUSD_UFE_USD_TRANSFORM3D_POINT_INSTANCE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdPointInstanceOrientationModifier.h>
#include <mayaUsd/ufe/UsdPointInstancePositionModifier.h>
#include <mayaUsd/ufe/UsdPointInstanceScaleModifier.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdTransform3dBase.h>

#include <ufe/transform3dHandler.h>
#include <ufe/transform3dUndoableCommands.h>
#include <ufe/types.h>

#include <memory>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to transform point instances in 3D.
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dPointInstance : public UsdTransform3dBase
{
public:
    using Ptr = std::shared_ptr<UsdTransform3dPointInstance>;

    UsdTransform3dPointInstance(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dPointInstance() override = default;

    //! Create a UsdTransform3dPointInstance.
    static UsdTransform3dPointInstance::Ptr create(const UsdSceneItem::Ptr& item);

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr    rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr     scaleCmd(double x, double y, double z) override;

    Ufe::SetMatrix4dUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;
    Ufe::Matrix4d                        matrix() const override;

    Ufe::Matrix4d segmentInclusiveMatrix() const override;
    Ufe::Matrix4d segmentExclusiveMatrix() const override;

private:
    UsdPointInstancePositionModifier    _positionModifier;
    UsdPointInstanceOrientationModifier _orientationModifier;
    UsdPointInstanceScaleModifier       _scaleModifier;
}; // UsdTransform3dPointInstance

//! \brief Factory to create a UsdTransform3dPointInstance interface object.
//
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dPointInstanceHandler : public Ufe::Transform3dHandler
{
public:
    using Ptr = std::shared_ptr<UsdTransform3dPointInstanceHandler>;

    UsdTransform3dPointInstanceHandler(const Ufe::Transform3dHandler::Ptr& nextHandler);

    //! Create a UsdTransform3dPointInstanceHandler.
    static Ptr create(const Ufe::Transform3dHandler::Ptr& nextHandler);

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Transform3d::Ptr editTransform3d(
        const Ufe::SceneItem::Ptr&      item,
        const Ufe::EditTransform3dHint& hint) const override;

private:
    Ufe::Transform3dHandler::Ptr _nextHandler;

}; // UsdTransform3dPointInstanceHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
