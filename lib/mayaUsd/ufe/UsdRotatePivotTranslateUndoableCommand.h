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
#pragma once

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/transform3dUndoableCommands.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute translation command of the given prim's rotate pivot.
/*!
    Ability to perform undo to restore the original pivot value.
 */
class MAYAUSD_CORE_PUBLIC UsdRotatePivotTranslateUndoableCommand
    : public Ufe::TranslateUndoableCommand
{
public:
    typedef std::shared_ptr<UsdRotatePivotTranslateUndoableCommand> Ptr;

    UsdRotatePivotTranslateUndoableCommand(const Ufe::Path& path);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdRotatePivotTranslateUndoableCommand);

    //! Create a UsdRotatePivotTranslateUndoableCommand from a USD prim, UFE path and UFE scene
    //! item.
    static UsdRotatePivotTranslateUndoableCommand::Ptr create(const Ufe::Path& path);

    // Ufe::TranslateUndoableCommand overrides
    void undo() override;
    void redo() override;
    bool set(double x, double y, double z) override;

    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(_item != nullptr))
            return _item->prim();
        else
            return PXR_NS::UsdPrim();
    }

private:
    UsdUfe::UsdSceneItem::Ptr sceneItem() const;

private:
    Ufe::Path                         _path;
    mutable UsdUfe::UsdSceneItem::Ptr _item { nullptr };
    PXR_NS::UsdAttribute              _pivotAttrib;
    PXR_NS::GfVec3f                   _prevPivotValue;
    bool                              _noPivotOp;

}; // UsdRotatePivotTranslateUndoableCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
