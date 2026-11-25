//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_MAYAUSDBATCHOPSHANDLER_H
#define MAYAUSD_MAYAUSDBATCHOPSHANDLER_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdBatchOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya USD BatchOps handler: in-place duplicates also replicate the Maya extras.
class MAYAUSD_CORE_PUBLIC MayaUsdBatchOpsHandler : public UsdUfe::UsdBatchOpsHandler
{
public:
    typedef std::shared_ptr<MayaUsdBatchOpsHandler> Ptr;

    MayaUsdBatchOpsHandler() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdBatchOpsHandler);

    //! Create a MayaUsdBatchOpsHandler.
    static MayaUsdBatchOpsHandler::Ptr create();

protected:
    Ufe::SelectionUndoableCommand::Ptr createDuplicateInPlaceCmd(
        const Ufe::Selection&       selection,
        const Ufe::ValueDictionary& duplicateOptions) override;
}; // MayaUsdBatchOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYAUSDBATCHOPSHANDLER_H
