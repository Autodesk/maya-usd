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
#include "UsdUndoAddReferenceCommand.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/path.h>

namespace USDUFE_NS_DEF {

//! \brief Command to add a reference to a prim.
UsdUndoAddReferenceCommand::UsdUndoAddReferenceCommand(
    const PXR_NS::UsdPrim& prim,
    const std::string&     filePath,
    bool                   prepend)
    : _prim(prim)
    , _sdfRef()
    , _filePath(filePath)
    , _listPos(
          prepend ? PXR_NS::UsdListPositionBackOfPrependList
                  : PXR_NS::UsdListPositionBackOfAppendList)
{
}

void UsdUndoAddReferenceCommand::executeImplementation()
{
    if (!_prim.IsValid())
        return;

    _sdfRef = PXR_NS::TfStringEndsWith(_filePath, ".mtlx")
        ? PXR_NS::SdfReference(_filePath, PXR_NS::SdfPath("/MaterialX"))
        : PXR_NS::SdfReference(_filePath);

    PXR_NS::UsdReferences primRefs = _prim.GetReferences();
    primRefs.AddReference(_sdfRef, _listPos);
}

} // namespace USDUFE_NS_DEF
