//
// Copyright 2021 Autodesk
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
// Adapted from Pixar's USD SdfLayer::Traverse() implementation, so portions
// are copyrighted by Pixar, copyright notice follows:
//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
///
/// \file Sdf/layer.cpp

#include "traverseLayer.h"

#include <pxr/usd/sdf/primSpec.h>

#include <sstream>

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

void _traverseLayer(
    const SdfLayerHandle&           layer,
    const SdfPath&                  path,
    const MayaUsd::TraverseLayerFn& fn);

template <typename ChildPolicy>
void TraverseChildren(
    const SdfLayerHandle&           layer,
    const SdfPath&                  path,
    const MayaUsd::TraverseLayerFn& fn)
{
    std::vector<typename ChildPolicy::FieldType> children
        = layer->GetFieldAs<std::vector<typename ChildPolicy::FieldType>>(
            path, ChildPolicy::GetChildrenToken(path));

    TF_FOR_ALL(i, children) { _traverseLayer(layer, ChildPolicy::GetChildPath(path, *i), fn); }
}

void _traverseLayer(
    const SdfLayerHandle&           layer,
    const SdfPath&                  path,
    const MayaUsd::TraverseLayerFn& fn)
{
    if (!fn(path)) {
        // Prune the traversal as requested by fn.
        return;
    }

    auto primSpec = layer->GetPrimAtPath(path);
    if (!primSpec) {
        std::ostringstream msg;
        msg << "no primSpec at path " << path.GetText() << ".";
        throw MayaUsd::TraversalFailure(msg.str(), path);
    }

    auto fields = primSpec->ListFields();

    TF_FOR_ALL(i, fields)
    {
        if (*i == SdfChildrenKeys->PrimChildren) {
            TraverseChildren<Sdf_PrimChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->PropertyChildren) {
            TraverseChildren<Sdf_PropertyChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->MapperChildren) {
            TraverseChildren<Sdf_MapperChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->MapperArgChildren) {
            TraverseChildren<Sdf_MapperArgChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->VariantChildren) {
            TraverseChildren<Sdf_VariantChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->VariantSetChildren) {
            TraverseChildren<Sdf_VariantSetChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->ConnectionChildren) {
            TraverseChildren<Sdf_AttributeConnectionChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->RelationshipTargetChildren) {
            TraverseChildren<Sdf_RelationshipTargetChildPolicy>(layer, path, fn);
        } else if (*i == SdfChildrenKeys->ExpressionChildren) {
            TraverseChildren<Sdf_ExpressionChildPolicy>(layer, path, fn);
        }
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {

bool traverseLayer(
    const PXR_NS::SdfLayerHandle& layer,
    const PXR_NS::SdfPath&        path,
    const TraverseLayerFn&        fn)
{
    try {
        _traverseLayer(layer, path, fn);
    } catch (const TraversalFailure& e) {
        TF_WARN(
            "Layer traversal failed for path %s: %s", e.path().GetText(), e.reason().c_str());
        return false;
    }
    return true;
}

} // namespace MAYAUSD_NS_DEF
