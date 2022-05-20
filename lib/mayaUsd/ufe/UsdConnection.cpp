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

#include "UsdConnection.h"

#include <pxr/base/tf/diagnostic.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdConnection::UsdConnection(const Ufe::AttributeInfo& srcAttr, const Ufe::AttributeInfo& dstAttr)
    : Ufe::Connection(srcAttr, dstAttr)
{
}

UsdConnection::~UsdConnection()
{
}

UsdConnection::Ptr UsdConnection::create(const Ufe::AttributeInfo& srcAttr, const Ufe::AttributeInfo& dstAttr)
{
    if (srcAttr.runTimeId()!=dstAttr.runTimeId()) {
        TF_RUNTIME_ERROR("Cannot create a connection between two different data models.");
        return UsdConnection::Ptr();
    }

    return std::make_shared<UsdConnection>(srcAttr, dstAttr);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
