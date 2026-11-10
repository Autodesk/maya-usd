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
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
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
//  Connections and relationship targets authored in the stage's local layer
//  stack are updated too. Paths authored inside referenced or payloaded layers
//  are left untouched when composition maps them to the new path. When it does
//  not, as for a path authored in an asset to a prim the stage defines under the
//  referencing prim, they are overridden from the edit target.
USDUFE_PUBLIC
bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath);

//! Same as above, but when allLayers is true, connections and relationship targets
//  are instead updated in place in every layer contributing to the stage. This is
//  for namespace edits that rename the prim in every layer of its prim stack, as in
//  component stages. Nothing is edited, and false is returned, if one of the layers
//  to edit cannot be edited.
USDUFE_PUBLIC
bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath, bool allLayers);

//! This function automatically cleans the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to becomes invalid.
//  Connections and relationship targets authored in the stage's local layer
//  stack are removed too. Paths authored inside referenced or payloaded layers
//  are left untouched, unless the arc that maps them is introduced above the
//  deleted prim, as for a path authored in an asset to a prim the stage defines
//  under the referencing prim: these are removed from the edit target.
USDUFE_PUBLIC
bool cleanReferencedPath(const UsdPrim& deletedPrim);

//! Same as above, but when allLayers is true, connections and relationship targets
//  are instead removed in place in every layer contributing to the stage. This is
//  for namespace edits that delete the prim in every layer of its prim stack, as in
//  component stages.
//  deletedInLayer is a layer outside of the local layer stack the prim is deleted
//  in, as when an edit router sends the delete into a referenced layer. The paths
//  authored in that layer are removed in place too.
//  Nothing is edited, and false is returned, if one of the layers to edit in place
//  cannot be edited.
USDUFE_PUBLIC
bool cleanReferencedPath(
    const UsdPrim&        deletedPrim,
    bool                  allLayers,
    const SdfLayerHandle& deletedInLayer = SdfLayerHandle());

//! Returns true if reference is internal.
USDUFE_PUBLIC
bool isInternalReference(const SdfReference&);

//! Check if the src and dst attributes are connected.
//! \return True, if they are connected.
USDUFE_PUBLIC
bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr);

//! Check if a source connection property is allowed to be removed.
//! \return True, if the property can be removed.
USDUFE_PUBLIC
bool canRemoveSrcProperty(const PXR_NS::UsdAttribute& srcAttr);

//! Check if a destination connection property is allowed to be removed.
//! \return True, if the property can be removed.
USDUFE_PUBLIC
bool canRemoveDstProperty(const PXR_NS::UsdAttribute& dstAttr);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUTILS_H
