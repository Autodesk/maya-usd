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
#ifndef USDUFE_USD_POINT_INSTANCE_UNDOABLE_COMMANDS_H
#define USDUFE_USD_POINT_INSTANCE_UNDOABLE_COMMANDS_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/ufe/trf/UsdPointInstanceOrientationModifier.h>
#include <usdUfe/ufe/trf/UsdPointInstancePositionModifier.h>
#include <usdUfe/ufe/trf/UsdPointInstanceScaleModifier.h>

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/path.h>
#include <ufe/transform3d.h>
#include <ufe/transform3dUndoableCommands.h>
#include <ufe/types.h>

#include <memory>

namespace USDUFE_NS_DEF {

/// Templated base class for undoable commands that manipulate USD point instances.
///
template <class BaseUfeUndoableCommand, class PointInstanceModifierType, class UsdValueType>
class USDUFE_PUBLIC UsdPointInstanceUndoableCommandBase : public BaseUfeUndoableCommand
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
        auto item = downcast(this->sceneItem());
        if (!item || !item->isPointInstance()) {
            return;
        }

        _modifier.setSceneItem(item);
        // We're using a modifier to change a point instancer attribute, so
        // batch the reads and writes, for efficiency.
        _modifier.joinBatch();

        _prevValue = _modifier.getUsdValue(_readTime);
        _newValue = _prevValue;
    }

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdPointInstanceUndoableCommandBase);

    // Ufe::UndoableCommand overrides.
    void undo() override
    {
        // Block the USD change notice handling from running in response to the
        // USD authoring we're about to do. We notify afterwards only on the
        // specific point instance scene item being manipulated.
        UsdUfe::InTransform3dChange guard(this->path());
        _modifier.setValue(_prevValue, _writeTime);
        Ufe::Transform3d::notify(this->path());
    }

    void redo() override
    {
        // Block the USD change notice handling from running in response to the
        // USD authoring we're about to do. We notify afterwards only on the
        // specific point instance scene item being manipulated.
        UsdUfe::InTransform3dChange guard(this->path());
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
class USDUFE_PUBLIC UsdPointInstanceTranslateUndoableCommand
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
};

// Ensure that UsdPointInstanceTranslateUndoableCommand is properly setup.
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdPointInstanceTranslateUndoableCommand);
USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdPointInstanceTranslateUndoableCommand);

/// Undoable command for rotating USD point instances.
///
class USDUFE_PUBLIC UsdPointInstanceRotateUndoableCommand
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
};

// Ensure that UsdPointInstanceRotateUndoableCommand is properly setup.
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdPointInstanceRotateUndoableCommand);
USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdPointInstanceRotateUndoableCommand);

/// Undoable command for scaling USD point instances.
///
class USDUFE_PUBLIC UsdPointInstanceScaleUndoableCommand
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
};

// Ensure that UsdPointInstanceScaleUndoableCommand is properly setup.
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdPointInstanceScaleUndoableCommand);
USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdPointInstanceScaleUndoableCommand);

} // namespace USDUFE_NS_DEF

#endif
