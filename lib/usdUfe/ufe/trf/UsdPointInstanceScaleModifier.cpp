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

#include <usdUfe/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/pointInstancer.h>

namespace {
// Map of point instance batches under construction, keyed on instancer path.
UsdUfe::UsdPointInstanceScaleModifier::Batches sBatches;
} // namespace

namespace USDUFE_NS_DEF {

// Ensure that UsdPointInstanceScaleModifier is properly setup.
using base = UsdPointInstanceModifierBase<Ufe::Vector3d, PXR_NS::GfVec3f>;
USDUFE_VERIFY_CLASS_BASE(base, UsdPointInstanceScaleModifier);
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdPointInstanceScaleModifier);
USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdPointInstanceScaleModifier);

/* override */
PXR_NS::UsdAttribute UsdPointInstanceScaleModifier::getAttribute() const
{
    PXR_NS::UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return PXR_NS::UsdAttribute();
    }

    return pointInstancer.GetScalesAttr();
}

/* override */
PXR_NS::UsdAttribute UsdPointInstanceScaleModifier::_createAttribute()
{
    PXR_NS::UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return PXR_NS::UsdAttribute();
    }

    return pointInstancer.CreateScalesAttr();
}

UsdPointInstanceScaleModifier::Batches& UsdPointInstanceScaleModifier::batches()
{
    return sBatches;
}

} // namespace USDUFE_NS_DEF
