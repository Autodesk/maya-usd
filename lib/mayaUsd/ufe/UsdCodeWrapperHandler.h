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
#pragma once

#include <mayaUsd/ufe/UsdBatchOpsHandler.h>

#include <ufe/codeWrapperHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdCodeWrapperHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdCodeWrapperHandler : public UsdBatchOpsHandler
{
public:
    //! Create a UsdCodeWrapperHandler.
    static std::shared_ptr<UsdCodeWrapperHandler> create();

protected:
    // Ufe::CodeWrapperHandler overrides.
    Ufe::CodeWrapper::Ptr
    createCodeWrapper(const Ufe::Selection& selection, const std::string& operationName) override;
}; // UsdCodeWrapperHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
