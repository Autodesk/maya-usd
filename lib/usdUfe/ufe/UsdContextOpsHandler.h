//
// Copyright 2020 Autodesk
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
#ifndef USDUFE_USDCONTEXTOPSHANDLER_H
#define USDUFE_USDCONTEXTOPSHANDLER_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdContextOps.h>

#include <ufe/contextOpsHandler.h>

namespace USDUFE_NS_DEF {

//! \brief Interface to create a UsdContextOps interface object.
class USDUFE_PUBLIC UsdContextOpsHandler : public Ufe::ContextOpsHandler
{
public:
    typedef std::shared_ptr<UsdContextOpsHandler> Ptr;

    UsdContextOpsHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdContextOpsHandler);

    //! Create a UsdContextOpsHandler.
    static UsdContextOpsHandler::Ptr create();

    // Ufe::ContextOpsHandler overrides
    Ufe::ContextOps::Ptr contextOps(const Ufe::SceneItem::Ptr& item) const override;
}; // UsdContextOpsHandler

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDCONTEXTOPSHANDLER_H
