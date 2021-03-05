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
#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

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

#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdRotatePivotTranslateUndoableCommand(const Ufe::Path& path);
#else
    UsdRotatePivotTranslateUndoableCommand(
        const PXR_NS::UsdPrim&     prim,
        const Ufe::Path&           ufePath,
        const Ufe::SceneItem::Ptr& item);
#endif

    ~UsdRotatePivotTranslateUndoableCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdRotatePivotTranslateUndoableCommand(const UsdRotatePivotTranslateUndoableCommand&) = delete;
    UsdRotatePivotTranslateUndoableCommand& operator=(const UsdRotatePivotTranslateUndoableCommand&)
        = delete;
    UsdRotatePivotTranslateUndoableCommand(UsdRotatePivotTranslateUndoableCommand&&) = delete;
    UsdRotatePivotTranslateUndoableCommand& operator=(UsdRotatePivotTranslateUndoableCommand&&)
        = delete;

//! Create a UsdRotatePivotTranslateUndoableCommand from a USD prim, UFE path and UFE scene item.
#ifdef UFE_V2_FEATURES_AVAILABLE
    static UsdRotatePivotTranslateUndoableCommand::Ptr create(const Ufe::Path& path);
#else
    static UsdRotatePivotTranslateUndoableCommand::Ptr
         create(const PXR_NS::UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);
#endif

    // Ufe::TranslateUndoableCommand overrides
    void undo() override;
    void redo() override;
#ifdef UFE_V2_FEATURES_AVAILABLE
    bool set(double x, double y, double z) override;
#else
    bool translate(double x, double y, double z) override;
#endif

    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_AXIOM(fItem != nullptr);
        return fItem->prim();
    }

private:
    UFE_V2(UsdSceneItem::Ptr sceneItem() const;)

private:
#ifndef UFE_V2_FEATURES_AVAILABLE
    PXR_NS::UsdPrim fPrim;
#endif
    Ufe::Path                 fPath;
    mutable UsdSceneItem::Ptr fItem { nullptr };
    PXR_NS::UsdAttribute      fPivotAttrib;
    PXR_NS::GfVec3f           fPrevPivotValue;
    bool                      fNoPivotOp;

}; // UsdRotatePivotTranslateUndoableCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
