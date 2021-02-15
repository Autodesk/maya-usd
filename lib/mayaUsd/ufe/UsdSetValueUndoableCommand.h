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

#include "UsdUndoableCommand.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/base/vt/value.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Templated helper class to factor out common code for setting USD values.
// Supports repeated calls to the set() method, invoked during direct
// manipulation.
template <typename Cmd> class UsdSetValueUndoableCmdBase : public UsdUndoableCmdBase<Cmd>
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
        const UsdTimeCode& writeTime);

    UsdSetValueUndoableCmdBase(
        const VtValue&        newOpValue,
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime);

protected:
    using State = typename UsdUndoableCmdBase<Cmd>::State;

    // Engine method for derived classes implementing their set() method.
    void handleSet(State previousState, State newState, const VtValue& v) override;

private:
    UsdGeomXformOp _op;
    OpFunc         _opFunc;
};

// Implementation of the set-value command base class.

template <typename Cmd>
inline UsdSetValueUndoableCmdBase<Cmd>::UsdSetValueUndoableCmdBase(
    const VtValue&        newOpValue,
    const Ufe::Path&      path,
    const UsdGeomXformOp& op,
    const UsdTimeCode&    writeTime)
    : UsdUndoableCmdBase<Cmd>(newOpValue, path, writeTime)
    , _op(op)
    , _opFunc()
{
}

template <typename Cmd>
inline UsdSetValueUndoableCmdBase<Cmd>::UsdSetValueUndoableCmdBase(
    const VtValue&     newOpValue,
    const Ufe::Path&   path,
    OpFunc             opFunc,
    const UsdTimeCode& writeTime)
    : UsdUndoableCmdBase<Cmd>(newOpValue, path, writeTime)
    , _op()
    , _opFunc(std::move(opFunc))
{
}

template <typename Cmd>
inline void
UsdSetValueUndoableCmdBase<Cmd>::handleSet(State previousState, State newState, const VtValue& v)
{
    // Only need to initialize the transform operation on the first execution.
    if (previousState == State::Initial && _opFunc)
        _op = _opFunc(*this);

    _op.GetAttr().Set(v, writeTime());
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
