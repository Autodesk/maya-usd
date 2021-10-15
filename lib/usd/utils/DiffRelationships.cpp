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
#include "DiffCore.h"
#include "DiffPrims.h"

namespace MayaUsdUtils {

using UsdRelationship = PXR_NS::UsdRelationship;
using TfToken = PXR_NS::TfToken;
using SdfPath = PXR_NS::SdfPath;

namespace {

SdfPathVector GetRelTargets(const UsdRelationship& rel)
{
    SdfPathVector targets;
    // Note: we can receive invalid relationship when comparing all relationships of prims.
    // Note: we don't check the return value because an empty relationship returns false.
    if (rel.IsValid())
        rel.GetTargets(&targets);
    return targets;
}

} // namespace

DiffResultPerPath
compareRelationships(const UsdRelationship& modified, const UsdRelationship& baseline)
{
    const SdfPathVector baselineTargets = GetRelTargets(baseline);
    const SdfPathVector modifiedTargets = GetRelTargets(modified);

    return compareLists<SdfPath>(modifiedTargets, baselineTargets);
}

} // namespace MayaUsdUtils
