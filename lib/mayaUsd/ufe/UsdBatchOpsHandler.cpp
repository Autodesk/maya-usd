//
// Copyright 2022 Autodesk
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
#include "UsdBatchOpsHandler.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/relationship.h>

#include <ufe/runTimeMgr.h>
#include <ufe/value.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdBatchOpsHandler::UsdBatchOpsHandler()
    : Ufe::BatchOpsHandler()
{
}

UsdBatchOpsHandler::~UsdBatchOpsHandler() { }

/*static*/
UsdBatchOpsHandler::Ptr UsdBatchOpsHandler::create()
{
    return std::make_shared<UsdBatchOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::BatchOpsHandler overrides
//------------------------------------------------------------------------------

void UsdBatchOpsHandler::beginDuplicationGuard_(const Ufe::ValueDictionary& duplicateOptions)
{
    _clearGuardData();
    _duplicateOptions = duplicateOptions;
    _inBatchedDuplicate = true;

    // We currently only understand and support the "inputConnections" option:
    const auto itInputConnections = _duplicateOptions.find("inputConnections");
    if (itInputConnections != _duplicateOptions.end() && itInputConnections->second.get<bool>()) {
        _fixupCommand = UsdUndoDuplicateFixupsCommand::create();
    }
}

Ufe::UndoableCommand::Ptr UsdBatchOpsHandler::endDuplicationGuardCmd_()
{
    Ufe::UndoableCommand::Ptr retVal = _fixupCommand;
    _clearGuardData();
    return retVal;
}

void UsdBatchOpsHandler::_clearGuardData()
{
    _duplicateOptions.clear();
    _fixupCommand.reset();
}

void UsdBatchOpsHandler::postProcessDuplicatedItem(
    const PXR_NS::UsdPrim& srcPrim,
    const PXR_NS::UsdPrim& dstPrim)
{
    UsdBatchOpsHandler::Ptr handler = std::dynamic_pointer_cast<UsdBatchOpsHandler>(
        Ufe::RunTimeMgr::instance().batchOpsHandler(getUsdRunTimeId()));

    if (!handler->_inBatchedDuplicate) {
        return;
    }

    if (handler->_fixupCommand) {
        // Keep track of the paths and layers because we need to fixup connections later.
        handler->_fixupCommand->trackDuplicates(srcPrim, dstPrim);
    } else {
        // Cleanup relationships and connections directly on the duplicate.
        // This code is executed under the UsdUndoDuplicateCommand undo block, so will be correctly
        // tracked for undo/redo purpose.
        for (auto p : UsdPrimRange(dstPrim)) {
            for (auto& prop : p.GetProperties()) {
                if (prop.Is<PXR_NS::UsdAttribute>()) {
                    PXR_NS::UsdAttribute attr = prop.As<PXR_NS::UsdAttribute>();
                    attr.ClearConnections();
                    if (!attr.HasValue()) {
                        p.RemoveProperty(prop.GetName());
                    }
                    // If we wanted to be thorough, we would also make sure all relationships are
                    // cleared as well, but it does not create a good workflow.
                    //} else if (prop.Is<UsdRelationship>()) {
                    //    UsdRelationship rel = prop.As<UsdRelationship>();
                    //    rel.ClearTargets(true);
                }
            }
        }
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
