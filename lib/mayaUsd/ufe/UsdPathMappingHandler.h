#ifndef USDPATHMAPPINGHANDLER_H
#define USDPATHMAPPINGHANDLER_H

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

#include <mayaUsd/base/api.h>

#include <ufe/pathMappingHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdPathMappingHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdPathMappingHandler : public Ufe::PathMappingHandler
{
public:
    typedef std::shared_ptr<UsdPathMappingHandler> Ptr;

    UsdPathMappingHandler() = default;
    ~UsdPathMappingHandler() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdPathMappingHandler);

    //! Create a UsdPathMappingHandler.
    static UsdPathMappingHandler::Ptr create();

    // Ufe::PathMappingHandler overrides
    Ufe::Path toHost(const Ufe::Path&) const override;
    Ufe::Path fromHost(const Ufe::Path&) const override;

}; // UsdPathMappingHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDPATHMAPPINGHANDLER_H
