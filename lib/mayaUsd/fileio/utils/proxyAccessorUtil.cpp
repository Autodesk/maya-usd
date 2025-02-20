//
// Copyright 2024 Autodesk
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

#include "proxyAccessorUtil.h"

#include <mayaUsd/ufe/Utils.h>

#include <maya/MString.h>
#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {
namespace utils {

MStatus ProxyAccessorUndoItem::parentPulledObject(
    const std::string name,
    const MDagPath&   pulledDagPath,
    const Ufe::Path&  ufeParentPath,
    bool              force)
{
    // The "child" is the node that will receive the computed parent
    // transformation, in its offsetParentMatrix attribute.  We are using
    // the pull parent for this purpose, so pop the path of the ufeChild to
    // get to its pull parent.
    const auto ufeChildPath = MayaUsd::ufe::dagPathToUfe(pulledDagPath).pop();

    // Quick workaround to reuse some POC code - to rewrite later
    static const MString kPyTrueLiteral("True");
    static const MString kPyFalseLiteral("False");

    MString pyCommand;
    pyCommand.format(
        "from mayaUsd.lib import proxyAccessor as pa\n"
        "pa.parent('^1s', '^2s', force=^3s)\n",
        Ufe::PathString::string(ufeChildPath).c_str(),
        Ufe::PathString::string(ufeParentPath).c_str(),
        force ? kPyTrueLiteral : kPyFalseLiteral);

    auto item = std::make_unique<ProxyAccessorUndoItem>(std::move(name));

    CHECK_MSTATUS_AND_RETURN_IT(item->_modifier.pythonCommandToExecute(pyCommand));
    CHECK_MSTATUS_AND_RETURN_IT(item->_modifier.doIt());

    auto& undoInfo = OpUndoItemList::instance();
    undoInfo.addItem(std::move(item));
    return MS::kSuccess;
}

bool ProxyAccessorUndoItem::undo() { return _modifier.undoIt() == MS::kSuccess; }
bool ProxyAccessorUndoItem::redo() { return _modifier.doIt() == MS::kSuccess; }

ProxyAccessorUndoItem::~ProxyAccessorUndoItem() { }

} // namespace utils
} // namespace MAYAUSD_NS_DEF
