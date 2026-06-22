//
// Copyright 2026 Autodesk
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

#ifndef USDUFE_UNDO_USDUNDOUTILS_H
#define USDUFE_UNDO_USDUNDOUTILS_H

#include <usdUfe/base/api.h>

#include <pxr/usd/usd/stage.h>

#include <vector>

namespace USDUFE_NS_DEF {

//! Installs a UsdUndoStateDelegate on each stage's current edit-target
//! layer. Repeated calls for the same layer are idempotent.
USDUFE_PUBLIC void trackStagesEditTargets(const std::vector<PXR_NS::UsdStageRefPtr>& stages);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UNDO_USDUNDOUTILS_H
