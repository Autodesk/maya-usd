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

#include "UsdPointInstanceScaleModifier.h"

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/pointInstancer.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

/* override */
UsdAttribute UsdPointInstanceScaleModifier::_getAttribute() const
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return UsdAttribute();
    }

    return pointInstancer.GetScalesAttr();
}

/* override */
UsdAttribute UsdPointInstanceScaleModifier::_createAttribute()
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return UsdAttribute();
    }

    return pointInstancer.CreateScalesAttr();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
