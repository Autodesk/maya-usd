#ifndef USDBATCHOPSHANDLER_H
#define USDBATCHOPSHANDLER_H

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

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdUndoDuplicateFixupsCommand.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/batchOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdBatchOpsHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdBatchOpsHandler : public Ufe::BatchOpsHandler
{
public:
    typedef std::shared_ptr<UsdBatchOpsHandler> Ptr;

    UsdBatchOpsHandler();
    ~UsdBatchOpsHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdBatchOpsHandler(const UsdBatchOpsHandler&) = delete;
    UsdBatchOpsHandler& operator=(const UsdBatchOpsHandler&) = delete;
    UsdBatchOpsHandler(UsdBatchOpsHandler&&) = delete;
    UsdBatchOpsHandler& operator=(UsdBatchOpsHandler&&) = delete;

    //! Create a UsdBatchOpsHandler.
    static UsdBatchOpsHandler::Ptr create();

    // Ufe::BatchOpsHandler overrides.
    void beginDuplicationGuard_(const Ufe::ValueDictionary& duplicateOptions) override;
    Ufe::UndoableCommand::Ptr endDuplicationGuardCmd_() override;

    // Post-process a duplicated item:
    static void
    postProcessDuplicatedItem(const PXR_NS::UsdPrim& srcPrim, const PXR_NS::UsdPrim& dstPrim);

private:
    Ufe::ValueDictionary               _duplicateOptions;
    UsdUndoDuplicateFixupsCommand::Ptr _fixupCommand;
    bool                               _inBatchedDuplicate = false;

    // Clear all guard data.
    void _clearGuardData();
}; // UsdBatchOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDBATCHOPSHANDLER_H
