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
#ifndef MAYAUSD_USDSCALEUNDOABLECOMMAND_H
#define MAYAUSD_USDSCALEUNDOABLECOMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/transform3dUndoableCommands.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute scale command of the given prim.
/*!
        Ability to perform undo to restore the original scale value.
 */
class MAYAUSD_CORE_PUBLIC UsdScaleUndoableCommand
    : public Ufe::ScaleUndoableCommand
    , public UsdTRSUndoableCommandBase<PXR_NS::GfVec3f>
{
public:
    typedef std::shared_ptr<UsdScaleUndoableCommand> Ptr;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdScaleUndoableCommand);

    //! Create a UsdScaleUndoableCommand from a UFE scene path.  The command is
    //! not executed.
    static UsdScaleUndoableCommand::Ptr create(const Ufe::Path& path, double x, double y, double z);

    // Ufe::ScaleUndoableCommand overrides
    void undo() override;
    void redo() override;
    bool set(double x, double y, double z) override;

    Ufe::Path getPath() const override { return path(); }

protected:
    //! Construct a UsdScaleUndoableCommand.  The command is not executed.
    UsdScaleUndoableCommand(const Ufe::Path& path, double x, double y, double z);

private:
    static PXR_NS::TfToken scaleTok;

    PXR_NS::TfToken attributeName() const override { return scaleTok; }
    void            performImp(double x, double y, double z) override;
    void            addEmptyAttribute() override;

}; // UsdScaleUndoableCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDSCALEUNDOABLECOMMAND_H
