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

#include "contextOps.h"

#include <usdUfe/ufe/UsdContextOps.h>

namespace MAYAUSDAPI_NS_DEF {

bool isBulkEdit(const Ufe::ContextOps::Ptr& ptr)
{
    if (auto usdContextOps = std::dynamic_pointer_cast<UsdUfe::UsdContextOps>(ptr)) {
        return usdContextOps->isBulkEdit();
    }
    return false;
}

const std::string bulkEditType(const Ufe::ContextOps::Ptr& ptr)
{
    if (auto usdContextOps = std::dynamic_pointer_cast<UsdUfe::UsdContextOps>(ptr)) {
        return usdContextOps->bulkEditType();
    }
    return "";
}

void addBulkEditHeader(const Ufe::ContextOps::Ptr& ptr, Ufe::ContextOps::Items& items)
{
    if (auto usdContextOps = std::dynamic_pointer_cast<UsdUfe::UsdContextOps>(ptr)) {
        usdContextOps->addBulkEditHeader(items);
    }
}

} // namespace MAYAUSDAPI_NS_DEF
