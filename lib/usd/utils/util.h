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

    //! Return a list of layers in no strength order that can contribute to the argument prim.
    MAYA_USD_UTILS_PUBLIC
    std::set<SdfLayerHandle> layersWithContribution(const UsdPrim&);

    //! Check if a layer has any contributions towards the argument prim.
    MAYA_USD_UTILS_PUBLIC
    bool doesEditTargetLayerContribute(const UsdPrim&);

    //! Return the strongest layer that can contribute to the argument prim.
    MAYA_USD_UTILS_PUBLIC
    SdfLayerHandle strongestContributingLayer(const UsdPrim&);

    //! Return a PrimSpec for the argument prim in the layer containing the stage's current edit target.
    MAYA_USD_UTILS_PUBLIC
    SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim&);

    //! Returns true if the prim spec has an internal reference.
    MAYA_USD_UTILS_PUBLIC
    bool isInternalReference(const SdfPrimSpecHandle&);

} // namespace MayaUsdUtils

#endif // MAYAUSDUTILS_UTIL_H