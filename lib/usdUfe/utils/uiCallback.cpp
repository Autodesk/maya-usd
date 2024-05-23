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

#include <pxr/base/tf/token.h>

namespace {

UsdUfe::UICallbacks& getRegisteredUICallbacks()
{
    static UsdUfe::UICallbacks registeredUICallbacks;
    return registeredUICallbacks;
}

} // namespace

namespace USDUFE_NS_DEF {

UICallback::~UICallback() { }

void registerUICallback(const PXR_NS::TfToken& operation, const UICallback::Ptr& uiCallback)
{
    getRegisteredUICallbacks()[operation] = uiCallback;
}

UICallback::Ptr getUICallback(const PXR_NS::TfToken& operation)
{
    UsdUfe::UICallbacks& uiCallbacks = getRegisteredUICallbacks();

    auto foundCallback = uiCallbacks.find(operation);
    return (foundCallback == uiCallbacks.end()) ? nullptr : foundCallback->second;
}

} // namespace USDUFE_NS_DEF
