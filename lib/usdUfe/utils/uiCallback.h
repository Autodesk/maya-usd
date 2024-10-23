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
#ifndef USDUFE_UICALLBACK_H
#define USDUFE_UICALLBACK_H

#include <usdUfe/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/sdf/types.h>

#include <memory>

namespace USDUFE_NS_DEF {

// A callback system that is used to provide hooks for some of the commands as well as some of the
// UI operations to end users.
class USDUFE_PUBLIC UICallback
{
public:
    typedef std::shared_ptr<UICallback> Ptr;

    virtual ~UICallback();

    // Compute the callback data. The context is immutable, and is input to
    // the computation of the callback data. Callback data may be initialized,
    // so that acceptable defaults can be left unchanged.
    virtual void operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& callbackData)
        = 0;
};

// Register an callback for the argument operation.
USDUFE_PUBLIC
void registerUICallback(const PXR_NS::TfToken& operation, const UICallback::Ptr& uiCallback);

// Unregister an callback for the argument operation.
USDUFE_PUBLIC
void unregisterUICallback(const PXR_NS::TfToken& operation, const UICallback::Ptr& uiCallback);

// Is there a callback registered for the argument operation?
USDUFE_PUBLIC
bool isUICallbackRegistered(const PXR_NS::TfToken& operation);

// Trigger the callback(s) for the argument operation with the callback context and data.
USDUFE_PUBLIC
void triggerUICallback(
    const PXR_NS::TfToken&      operation,
    const PXR_NS::VtDictionary& context,
    PXR_NS::VtDictionary&       data);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UICALLBACK_H
