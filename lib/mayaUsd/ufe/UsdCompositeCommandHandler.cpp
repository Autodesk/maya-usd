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
#include "UsdCompositeCommandHandler.h"

#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/utils/editRouterContext.h>

#include <ufe/sceneItem.h>
#include <ufe/selection.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {
class UsdCodeWrapper : public Ufe::CodeWrapper
{
public:
    UsdCodeWrapper(const Ufe::Selection& selection, const std::string& operationName)
        : _prim(findPrimInSelection(selection))
        , _operationName(PXR_NS::TfToken(operationName))
    {
    }

    void preCall() override
    {
        _editRouterContext = std::make_unique<OperationEditRouterContext>(_operationName, _prim);
    }

    void postCall() override { _editRouterContext.reset(); }

private:
    static PXR_NS::UsdPrim findPrimInSelection(const Ufe::Selection& selection)
    {
        for (const Ufe::SceneItem::Ptr& item : selection) {
            const auto usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
            if (!usdItem)
                continue;
            return usdItem->prim();
        }

        return {};
    }

    PXR_NS::UsdPrim                             _prim;
    PXR_NS::TfToken                             _operationName;
    std::unique_ptr<OperationEditRouterContext> _editRouterContext;
};

class UsdCompositeCommandWrapper : public Ufe::CompositeCommandWrapper
{
public:
    UsdCompositeCommandWrapper(const Ufe::Selection& selection, const std::string& operationName)
        : _wrapper(selection, operationName)
    {
    }

    Ufe::CodeWrapper& executeWrapper() override { return _wrapper; }

private:
    UsdCodeWrapper _wrapper;
};
} // namespace

/*static*/
std::shared_ptr<UsdCompositeCommandHandler> UsdCompositeCommandHandler::create()
{
    return std::make_shared<UsdCompositeCommandHandler>();
}

Ufe::CompositeCommandWrapper::Ptr UsdCompositeCommandHandler::createCompositeCommandWrapper_(
    const Ufe::Selection& selection,
    const std::string&    operationName)
{
    return std::make_unique<UsdCompositeCommandWrapper>(selection, operationName);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
