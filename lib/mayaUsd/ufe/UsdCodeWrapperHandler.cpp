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
#include "UsdCodeWrapperHandler.h"

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouterContext.h>

#include <ufe/selection.h>

namespace {

// A code wrapper that does edit routing for a command name by its operation.
// The edit routing decision is cached after the first sub-operation and is
// reused in subsequent sub-operations. This ensures the same edit routing is
// used during a command execution and during undo and redo.
//
// Note: the code wrapper is the same for the command execute, undo and redo,
//       so we don't need the sub-operation name.
class UsdEditRoutingCodeWrapper : public Ufe::CodeWrapper
{
public:
    UsdEditRoutingCodeWrapper(const Ufe::Selection& selection, const std::string& operationName)
        : _prim(findPrimInSelection(selection))
        , _operationName(PXR_NS::TfToken(operationName))
    {
    }

    void prelude(const std::string& /* subOperation */) override
    {
        if (_alreadyRouted) {
            _editRouterContext
                = std::make_unique<UsdUfe::OperationEditRouterContext>(_stage, _layer);
        } else {
            _editRouterContext
                = std::make_unique<UsdUfe::OperationEditRouterContext>(_operationName, _prim);
            _stage = _editRouterContext->getStage();
            _layer = _editRouterContext->getLayer();
            _alreadyRouted = true;
        }
    }

    void cleanup(const std::string& /* subOperation */) override { _editRouterContext.reset(); }

private:
    static PXR_NS::UsdPrim findPrimInSelection(const Ufe::Selection& selection)
    {
        for (const Ufe::SceneItem::Ptr& item : selection) {
            const auto usdItem = UsdUfe::downcast(item);
            if (!usdItem)
                continue;
            return usdItem->prim();
        }

        return {};
    }

    PXR_NS::UsdPrim        _prim;
    PXR_NS::TfToken        _operationName;
    bool                   _alreadyRouted = false;
    PXR_NS::UsdStagePtr    _stage;
    PXR_NS::SdfLayerHandle _layer;

    std::unique_ptr<UsdUfe::OperationEditRouterContext> _editRouterContext;
};

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdBatchOpsHandler, UsdCodeWrapperHandler);

/*static*/
std::shared_ptr<UsdCodeWrapperHandler> UsdCodeWrapperHandler::create()
{
    return std::make_shared<UsdCodeWrapperHandler>();
}

Ufe::CodeWrapper::Ptr UsdCodeWrapperHandler::createCodeWrapper(
    const Ufe::Selection& selection,
    const std::string&    operationName)
{
    return std::make_unique<UsdEditRoutingCodeWrapper>(selection, operationName);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
