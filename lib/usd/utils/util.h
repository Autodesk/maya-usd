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
SdfLayerHandle defPrimSpecLayer(const UsdPrim& prim);

//! Hold a layer a path relative to that layer.
struct LayerAndPath
{
    SdfLayerHandle layer;
    SdfPath        path;

    LayerAndPath() = default;
    LayerAndPath(const LayerAndPath&) = default;
    LayerAndPath(const SdfLayerHandle& a_layer, const SdfPath& a_path)
        : layer(a_layer)
        , path(a_path)
    {
    }
};

//! Return all layers where the prim has opinions and the paths relative to that layer.
//  The strongest layer is the first in the list.
//  When the prim is in a reference, that path will not be equal to the path of the input prim.
MAYA_USD_UTILS_PUBLIC
std::vector<LayerAndPath>
getAuthoredLayerAndPaths(const UsdPrim& prim, bool includePayloads = false);

//! Return the layer where the prim is defined and the path relative to that layer.
//  When the prim is in a reference, that path will not be equal to the path of the input prim.
MAYA_USD_UTILS_PUBLIC
LayerAndPath getDefiningLayerAndPath(const UsdPrim& prim);

//! Return a PrimSpec for the argument prim in the layer containing the stage's current edit target.
MAYA_USD_UTILS_PUBLIC
SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim& prim);

//! Convenience function for printing the list of queried composition arcs in order.
MAYA_USD_UTILS_PUBLIC
void printCompositionQuery(const UsdPrim& prim, std::ostream& os);

//! This function automatically updates the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to has changed.
MAYA_USD_UTILS_PUBLIC
bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath);

//! Returns true if reference is internal.
MAYA_USD_UTILS_PUBLIC
bool isInternalReference(const SdfReference&);

} // namespace MayaUsdUtils

#endif // MAYAUSDUTILS_UTIL_H
