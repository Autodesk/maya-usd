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
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/base/vt/value.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <ufe/baseUndoableCommands.h>
#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Templated helper class to factor out common code for setting USD values.
// Supports repeated calls to the set() method, invoked during direct
// manipulation.
template <typename Cmd> class UsdSetValueUndoableCmdBase : public Cmd
{
public:
#if UFE_PREVIEW_VERSION_NUM >= 2031
    using BaseUndoableCommand = Ufe::BaseUndoableCommand;
#else
    using BaseUndoableCommand = Ufe::BaseTransformUndoableCommand;
#endif
    using OpFunc = std::function<UsdGeomXformOp(const BaseUndoableCommand&)>;

    UsdSetValueUndoableCmdBase(
        const VtValue&     newOpValue,
        const Ufe::Path&   path,
        OpFunc             opFunc,
        const UsdTimeCode& writeTime_);

    // Ufe::UndoableCommand overrides.
    void execute() override;
    void undo() override;
    void redo() override;

    UsdTimeCode readTime() const;
    UsdTimeCode writeTime() const;

    // Low-level implementation call to set the value onto the attribute.
    // Should not be called directly, as this by-passes undo / redo.
    void setValue(const VtValue& v);

    struct State;
    struct InitialState;
    struct InitialUndoCalledState;
    struct ExecuteState;
    struct UndoneState;
    struct RedoneState;

    const State* _state;

protected:
    // Engine method for derived classes implementing their set() method.
    void handleSet(const VtValue& v);

private:
    const UsdTimeCode _readTime;
    const UsdTimeCode _writeTime;
    VtValue           _newOpValue;
    UsdGeomXformOp    _op;
    OpFunc            _opFunc;
    UsdUndoableItem   _undoableItem;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
