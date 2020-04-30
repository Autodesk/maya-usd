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

#ifndef MAYAUSDUTILS_UTIL_H
#define MAYAUSDUTILS_UTIL_H

#include <mayaUsdUtils/Api.h>

#include <mayaUsdUtils/ForwardDeclares.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdUtils {

    //! Return the highest-priority layer where the prim has a def primSpec.
    MAYA_USD_UTILS_PUBLIC
    SdfLayerHandle defPrimSpecLayer(const UsdPrim&);

    //! Check if a layer has any opinions that affects a particular prim
    MAYA_USD_UTILS_PUBLIC
    bool doesLayerHavePrimSpec(const UsdPrim&);

    //! Return the layer that has any opinions on a particular prim
    MAYA_USD_UTILS_PUBLIC
    SdfLayerHandle strongestLayerWithPrimSpec(const UsdPrim&);

} // namespace MayaUsdUtils

#endif // MAYAUSDUTILS_UTIL_H