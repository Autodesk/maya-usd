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
#pragma once

#include <mayaUsd/base/api.h>

#include <ufe/connection.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdConnection : public Ufe::Connection
{
public:
    typedef std::shared_ptr<UsdConnection> Ptr;

    UsdConnection(const Ufe::AttributeInfo& srcAttr, const Ufe::AttributeInfo& dstAttr);
    ~UsdConnection() override;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdConnection(const UsdConnection&) = delete;
    UsdConnection& operator=(const UsdConnection&) = delete;
    UsdConnection(UsdConnection&&) = delete;
    UsdConnection& operator=(UsdConnection&&) = delete;
    //@}

    static UsdConnection::Ptr create(const Ufe::AttributeInfo& srcAttr, const Ufe::AttributeInfo& dstAttr);

}; // UsdConnection

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
