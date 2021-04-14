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
#ifndef MAYAUSD_UFE_USD_POINT_INSTANCE_POSITION_MODIFIER_H
#define MAYAUSD_UFE_USD_POINT_INSTANCE_POSITION_MODIFIER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdPointInstanceModifierBase.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/types.h>
#else
#include <ufe/transform3d.h>
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

/// Modifier specialization for accessing and modifying a point instance's
/// position.
///
class MAYAUSD_CORE_PUBLIC UsdPointInstancePositionModifier
    : public UsdPointInstanceModifierBase<Ufe::Vector3d, PXR_NS::GfVec3f>
{
public:
    UsdPointInstancePositionModifier()
        : UsdPointInstanceModifierBase<Ufe::Vector3d, PXR_NS::GfVec3f>()
    {
    }

    ~UsdPointInstancePositionModifier() override = default;

    PXR_NS::GfVec3f convertValueToUsd(const Ufe::Vector3d& ufeValue) const override
    {
        return PXR_NS::GfVec3f(ufeValue.x(), ufeValue.y(), ufeValue.z());
    }

    Ufe::Vector3d convertValueToUfe(const PXR_NS::GfVec3f& usdValue) const override
    {
        return Ufe::Vector3d(usdValue[0u], usdValue[1u], usdValue[2u]);
    }

    PXR_NS::GfVec3f getDefaultUsdValue() const override
    {
        return PXR_NS::GfVec3f(0.0f, 0.0f, 0.0f);
    }

    Batches& batches() override;

protected:
    PXR_NS::UsdAttribute _getAttribute() const override;

    PXR_NS::UsdAttribute _createAttribute() override;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
