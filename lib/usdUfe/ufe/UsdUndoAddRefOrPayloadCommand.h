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
#ifndef USD_UFE_ADD_REF_OR_PAYLOAD_COMMAND
#define USD_UFE_ADD_REF_OR_PAYLOAD_COMMAND

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <string>

namespace USDUFE_NS_DEF {

//! \brief Command to add a reference or payload to a prim.
class USDUFE_PUBLIC UsdUndoAddRefOrPayloadCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    UsdUndoAddRefOrPayloadCommand(
        const PXR_NS::UsdPrim&  prim,
        const std::string&      filePath,
        const std::string&      primPath,
        PXR_NS::UsdListPosition listPos,
        bool                    isPayload);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoAddRefOrPayloadCommand);

protected:
    static PXR_NS::UsdListPosition getListPosition(bool prepend);

    void executeImplementation() override;

private:
    PXR_NS::UsdPrim         _prim;
    std::string             _filePath;
    std::string             _primPath;
    PXR_NS::UsdListPosition _listPos;
    bool                    _isPayload;
};

} // namespace USDUFE_NS_DEF

#endif /* USD_UFE_ADD_REF_OR_PAYLOAD_COMMAND */
