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

#include "UsdUndoClearReferencesCommand.h"

#include <pxr/usd/usd/references.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(UsdUndoableCommand<Ufe::UndoableCommand>, UsdUndoClearReferencesCommand);

UsdUndoClearReferencesCommand::UsdUndoClearReferencesCommand(const PXR_NS::UsdPrim& prim)
    : _prim(prim)
{
}

void UsdUndoClearReferencesCommand::executeImplementation()
{
    if (!_prim.IsValid())
        return;

    _prim.GetReferences().ClearReferences();
}

} // namespace USDUFE_NS_DEF
