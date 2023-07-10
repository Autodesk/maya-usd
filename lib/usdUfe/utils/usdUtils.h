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

#ifndef USDUFE_USDUTILS_H
#define USDUFE_USDUTILS_H

#include <usdUfe/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

//! Return a PrimSpec for the argument prim in the layer containing the stage's current edit target.
USDUFE_PUBLIC
SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim& prim);

//! Convenience function for printing the list of queried composition arcs in order.
USDUFE_PUBLIC
void printCompositionQuery(const UsdPrim& prim, std::ostream& os);

//! This function automatically updates the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to has changed.
USDUFE_PUBLIC
bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath);

//! This function automatically cleans the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to becomes invalid.
USDUFE_PUBLIC
bool cleanReferencedPath(const UsdPrim& deletedPrim);

//! Returns true if reference is internal.
USDUFE_PUBLIC
bool isInternalReference(const SdfReference&);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUTILS_H
