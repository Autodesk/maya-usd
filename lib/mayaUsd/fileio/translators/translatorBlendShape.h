//
// Copyright 2024 Autodesk
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
#ifndef PXRUSDMAYA_TRANSLATOR_LIGHT_H
#define PXRUSDMAYA_TRANSLATOR_LIGHT_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderRegistry.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE
class UsdSkelSkinningQuery;
class UsdSkelAnimQuery;
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
struct UsdMayaTranslatorBlendShape
{
    MAYAUSD_CORE_PUBLIC
    static bool Read(
        const UsdPrim&            meshPrim,
        UsdMayaPrimReaderContext* context);

    MAYAUSD_CORE_PUBLIC
    static bool ReadAnimations(
        const UsdSkelSkinningQuery&     skinningQuery,
        const UsdSkelAnimQuery&         animQuery,
        const UsdMayaPrimReaderContext& context);
};
} // namespace MAYAUSD_NS_DEF
#endif
