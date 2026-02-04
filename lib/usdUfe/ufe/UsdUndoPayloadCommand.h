//
// Copyright 2023 Autodesk
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
#ifndef USDUFE_USDUNDOPAYLOADCOMMAND_H
#define USDUFE_USDUNDOPAYLOADCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief Undoable command for loading a USD prim.
class USDUFE_PUBLIC UsdUndoLoadUnloadBaseCommand : public Ufe::UndoableCommand
{
protected:
    UsdUndoLoadUnloadBaseCommand(const PXR_NS::UsdPrim& prim, PXR_NS::UsdLoadPolicy policy);
    UsdUndoLoadUnloadBaseCommand(const PXR_NS::UsdPrim& prim);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoLoadUnloadBaseCommand);

    using commandFn = std::function<void(const UsdUndoLoadUnloadBaseCommand*)>;
    void doCommand(commandFn fn, bool undo = false);
    void doLoad() const;
    void doUnload() const;

    void saveModifiedLoadRules() const;

private:
    const PXR_NS::UsdStageWeakPtr _stage;
    const PXR_NS::SdfPath         _primPath;
    PXR_NS::UsdLoadPolicy         _policy;
    PXR_NS::UsdStageLoadRules     _undoRules;
};

//! \brief Undoable command for loading a USD prim.
class USDUFE_PUBLIC UsdUndoLoadPayloadCommand : public UsdUndoLoadUnloadBaseCommand
{
public:
    UsdUndoLoadPayloadCommand(const PXR_NS::UsdPrim& prim, PXR_NS::UsdLoadPolicy policy);

    void redo() override;
    void undo() override;
    UFE_V4(std::string commandString() const override { return "LoadPayload"; })
};

//! \brief Undoable command for unloading a USD prim.
class USDUFE_PUBLIC UsdUndoUnloadPayloadCommand : public UsdUndoLoadUnloadBaseCommand
{
public:
    UsdUndoUnloadPayloadCommand(const PXR_NS::UsdPrim& prim);

    void redo() override;
    void undo() override;
    UFE_V4(std::string commandString() const override { return "UnloadPayload"; })
};

} // namespace USDUFE_NS_DEF

#endif
