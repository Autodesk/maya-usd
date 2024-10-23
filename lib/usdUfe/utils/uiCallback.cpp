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
#include "uiCallback.h"

#include <usdUfe/base/tokens.h>

#include <pxr/base/tf/hashmap.h>

#include <functional>
#include <vector>

namespace {

using UICallbacks = PXR_NS::
    TfHashMap<PXR_NS::TfToken, std::vector<UsdUfe::UICallback::Ptr>, PXR_NS::TfToken::HashFunctor>;

UICallbacks& getRegisteredUICallbacks()
{
    static UICallbacks registeredUICallbacks;
    return registeredUICallbacks;
}

} // namespace

namespace USDUFE_NS_DEF {

UICallback::~UICallback() { }

void registerUICallback(const PXR_NS::TfToken& operation, const UICallback::Ptr& uiCallback)
{
    std::vector<UICallback::Ptr>& callbacks = getRegisteredUICallbacks()[operation];
    callbacks.push_back(uiCallback);
}

void unregisterUICallback(const PXR_NS::TfToken& operation, const UICallback::Ptr& uiCallback)
{
    std::vector<UICallback::Ptr>& callbacks = getRegisteredUICallbacks()[operation];
    std::vector<UICallback::Ptr>  newCallbacks;
    for (const UICallback::Ptr& cb : callbacks)
        if (cb != uiCallback)
            newCallbacks.emplace_back(cb);

    if (newCallbacks.size() > 0)
        getRegisteredUICallbacks()[operation] = newCallbacks;
    else
        getRegisteredUICallbacks().erase(operation);
}

bool isUICallbackRegistered(const PXR_NS::TfToken& operation)
{
    const auto& uiCallbacks = getRegisteredUICallbacks();
    return (uiCallbacks.count(operation) >= 1);
}

void triggerUICallback(
    const PXR_NS::TfToken&      operation,
    const PXR_NS::VtDictionary& context,
    PXR_NS::VtDictionary&       data)
{
    UICallbacks& uiCallbacks = getRegisteredUICallbacks();
    auto foundCallback = uiCallbacks.find(operation);
    if (foundCallback != uiCallbacks.end()) {
        for (auto& cb : foundCallback->second) {
            (*cb)(context, data);
        }
    }
}

} // namespace USDUFE_NS_DEF
