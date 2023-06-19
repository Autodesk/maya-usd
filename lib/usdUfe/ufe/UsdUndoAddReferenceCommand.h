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
#ifndef USD_UFE_ADD_REFERENCE_COMMAND
#define USD_UFE_ADD_REFERENCE_COMMAND

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/references.h>

#include <ufe/undoableCommand.h>

#include <string>

namespace USDUFE_NS_DEF {

//! \brief Command to add a reference to a prim.
class USDUFE_PUBLIC UsdUndoAddReferenceCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    UsdUndoAddReferenceCommand(
        const PXR_NS::UsdPrim& prim,
        const std::string&     filePath,
        bool                   prepend);

protected:
    void executeImplementation() override;

private:
    PXR_NS::UsdPrim         _prim;
    PXR_NS::SdfReference    _sdfRef;
    std::string             _filePath;
    PXR_NS::UsdListPosition _listPos;
};

} // namespace USDUFE_NS_DEF

#endif /* USD_UFE_ADD_REFERENCE_COMMAND */
