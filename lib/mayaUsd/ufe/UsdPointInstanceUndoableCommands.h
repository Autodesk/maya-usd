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
#ifndef MAYAUSD_UFE_USD_POINT_INSTANCE_UNDOABLE_COMMANDS_H
#define MAYAUSD_UFE_USD_POINT_INSTANCE_UNDOABLE_COMMANDS_H

#include "private/UfeNotifGuard.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdPointInstanceOrientationModifier.h>
#include <mayaUsd/ufe/UsdPointInstancePositionModifier.h>
#include <mayaUsd/ufe/UsdPointInstanceScaleModifier.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/path.h>
#include <ufe/transform3d.h>
#include <ufe/transform3dUndoableCommands.h>
#include <ufe/types.h>

#include <memory>

namespace MAYAUSD_NS_DEF {
namespace ufe {

/// Templated base class for undoable commands that manipulate USD point instances.
///
template <class BaseUfeUndoableCommand, class PointInstanceModifierType, class UsdValueType>
class MAYAUSD_CORE_PUBLIC UsdPointInstanceUndoableCommandBase : public BaseUfeUndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdPointInstanceUndoableCommandBase>;

    UsdPointInstanceUndoableCommandBase(const Ufe::Path& path, const PXR_NS::UsdTimeCode& writeTime)
        : BaseUfeUndoableCommand(path)
        , _readTime(getTime(path))
        , _writeTime(writeTime)
        , _prevValue(_modifier.getDefaultUsdValue())
        , _newValue(_prevValue)
    {
        UsdSceneItem::Ptr item = downcast(this->sceneItem());
        if (!item || !item->isPointInstance()) {
            return;
        }

        _modifier.setPrimAndInstanceIndex(item->prim(), item->instanceIndex());

        _prevValue = _modifier.getUsdValue(_readTime);
        _newValue = _prevValue;
    }

    ~UsdPointInstanceUndoableCommandBase() override = default;

    // Ufe::UndoableCommand overrides.
    void undo() override
    {
        // Block the USD change notice handling from running in response to the
        // USD authoring we're about to do. We notify afterwards only on the
        // specific point instance scene item being manipulated.
        InTransform3dChange guard(this->path());
        _modifier.setValue(_prevValue, _writeTime);
        Ufe::Transform3d::notify(this->path());
    }

    void redo() override
    {
        // Block the USD change notice handling from running in response to the
        // USD authoring we're about to do. We notify afterwards only on the
        // specific point instance scene item being manipulated.
        InTransform3dChange guard(this->path());
        _modifier.setValue(_newValue, _writeTime);
        Ufe::Transform3d::notify(this->path());
    }

    bool set(double x, double y, double z) override
    {
        const Ufe::Vector3d ufeValue(x, y, z);
        _newValue = _modifier.convertValueToUsd(ufeValue);
        redo();
        return true;
    }

protected:
    PointInstanceModifierType _modifier;
    const PXR_NS::UsdTimeCode _readTime;
    const PXR_NS::UsdTimeCode _writeTime;
    UsdValueType              _prevValue;
    UsdValueType              _newValue;
}; // UsdPointInstanceUndoableCommandBase

/// Undoable command for translating USD point instances.
///
class MAYAUSD_CORE_PUBLIC UsdPointInstanceTranslateUndoableCommand
    : public UsdPointInstanceUndoableCommandBase<
          Ufe::TranslateUndoableCommand,
          UsdPointInstancePositionModifier,
          PXR_NS::GfVec3f>
{
public:
    using BaseClass = UsdPointInstanceUndoableCommandBase<
        Ufe::TranslateUndoableCommand,
        UsdPointInstancePositionModifier,
        PXR_NS::GfVec3f>;

    using Ptr = std::shared_ptr<UsdPointInstanceTranslateUndoableCommand>;

    UsdPointInstanceTranslateUndoableCommand(
        const Ufe::Path&           path,
        const PXR_NS::UsdTimeCode& writeTime)
        : BaseClass(path, writeTime)
    {
    }

    ~UsdPointInstanceTranslateUndoableCommand() override = default;
};

/// Undoable command for rotating USD point instances.
///
class MAYAUSD_CORE_PUBLIC UsdPointInstanceRotateUndoableCommand
    : public UsdPointInstanceUndoableCommandBase<
          Ufe::RotateUndoableCommand,
          UsdPointInstanceOrientationModifier,
          PXR_NS::GfQuath>
{
public:
    using BaseClass = UsdPointInstanceUndoableCommandBase<
        Ufe::RotateUndoableCommand,
        UsdPointInstanceOrientationModifier,
        PXR_NS::GfQuath>;

    using Ptr = std::shared_ptr<UsdPointInstanceRotateUndoableCommand>;

    UsdPointInstanceRotateUndoableCommand(
        const Ufe::Path&           path,
        const PXR_NS::UsdTimeCode& writeTime)
        : BaseClass(path, writeTime)
    {
    }

    ~UsdPointInstanceRotateUndoableCommand() override = default;
};

/// Undoable command for scaling USD point instances.
///
class MAYAUSD_CORE_PUBLIC UsdPointInstanceScaleUndoableCommand
    : public UsdPointInstanceUndoableCommandBase<
          Ufe::ScaleUndoableCommand,
          UsdPointInstanceScaleModifier,
          PXR_NS::GfVec3f>
{
public:
    using BaseClass = UsdPointInstanceUndoableCommandBase<
        Ufe::ScaleUndoableCommand,
        UsdPointInstanceScaleModifier,
        PXR_NS::GfVec3f>;

    using Ptr = std::shared_ptr<UsdPointInstanceScaleUndoableCommand>;

    UsdPointInstanceScaleUndoableCommand(
        const Ufe::Path&           path,
        const PXR_NS::UsdTimeCode& writeTime)
        : BaseClass(path, writeTime)
    {
    }

    ~UsdPointInstanceScaleUndoableCommand() override = default;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
