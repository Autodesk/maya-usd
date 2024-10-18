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
#ifndef USDUFE_USDTRANSLATEUNDOABLECOMMAND_H
#define USDUFE_USDTRANSLATEUNDOABLECOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/trf/UsdTRSUndoableCommandBase.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/transform3dUndoableCommands.h>

namespace USDUFE_NS_DEF {

//! \brief Translation command of the given prim.
/*!
        Ability to perform undo to restore the original translate value.
 */
class USDUFE_PUBLIC UsdTranslateUndoableCommand
    : public Ufe::TranslateUndoableCommand
    , public UsdTRSUndoableCommandBase<PXR_NS::GfVec3d>
{
public:
    typedef std::shared_ptr<UsdTranslateUndoableCommand> Ptr;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdTranslateUndoableCommand);

    //! Create a UsdTranslateUndoableCommand from a UFE scene path. The
    //! command is not executed.
    static UsdTranslateUndoableCommand::Ptr
    create(const Ufe::Path& path, double x, double y, double z);

    // Ufe::TranslateUndoableCommand overrides.  set() sets the command's
    // translation value and executes the command.
    void undo() override;
    void redo() override;
    bool set(double x, double y, double z) override;

    Ufe::Path getPath() const override { return path(); }

protected:
    //! Construct a UsdTranslateUndoableCommand.  The command is not executed.
    UsdTranslateUndoableCommand(const Ufe::Path& path, double x, double y, double z);

private:
    static PXR_NS::TfToken xlate;

    PXR_NS::TfToken attributeName() const override { return xlate; }
    void            performImp(double x, double y, double z) override;
    void            addEmptyAttribute() override;

}; // UsdTranslateUndoableCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDTRANSLATEUNDOABLECOMMAND_H
