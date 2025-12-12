//
// Copyright 2021 Autodesk
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
#include "editability.h"

#include <usdUfe/base/tokens.h>

namespace USDUFE_NS_DEF {
namespace Editability {

PXR_NAMESPACE_USING_DIRECTIVE

/*! \brief  Verify if a property is locked.
 */
bool isLocked(PXR_NS::UsdProperty property)
{
    // The reason we treat invalid property as editable is because we don't want
    // to influence editability of things that are not property that are being
    // tested by accident.
    if (!property.IsValid())
        return false;

    PXR_NS::TfToken lock;
    if (!property.GetMetadata(MetadataTokens->Lock, &lock))
        return false;

    if (lock == GenericTokens->Off) {
        return false;
    } else if (lock == GenericTokens->On) {
        return true;
    } else {
        TF_WARN("Invalid token value [%s] for lock will be treated as [off].", lock.data());
        return false;
    }
}

bool isAttributeLocked(const PXR_NS::UsdProperty& prop, std::string* errMsg)
{
    if (isLocked(prop)) {
        if (errMsg) {
            *errMsg = TfStringPrintf(
                "Cannot edit [%s] attribute because its lock metadata is [on].",
                prop.GetBaseName().GetText());
        }
        return true;
    }
    return false;
}

} // namespace Editability
} // namespace USDUFE_NS_DEF
