//
// Copyright 2022 Autodesk
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
#pragma once

#include "UsdAttributeHolder.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/utils.h>

#include <ufe/attribute.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Internal helper class holding a SdrShaderProperty, providing services to transparently
// handle it as if it was a native USD attribute found in a regular schema:
class UsdShaderAttributeHolder : public UsdAttributeHolder
{
    typedef UsdAttributeHolder _Base;

    UsdShaderAttributeHolder(
        PXR_NS::UsdPrim                   usdPrim,
        PXR_NS::SdrShaderPropertyConstPtr sdrProp,
        PXR_NS::UsdShadeAttributeType     sdrType);

public:
    static UPtr create(
        PXR_NS::UsdPrim                   usdPrim,
        PXR_NS::SdrShaderPropertyConstPtr sdrProp,
        PXR_NS::UsdShadeAttributeType     sdrType);
    virtual ~UsdShaderAttributeHolder() = default;

    virtual std::string isEditAllowedMsg() const;
    virtual std::string defaultValue() const;
    virtual std::string nativeType() const;
    virtual bool        get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const;
    virtual bool        set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time);

    virtual bool        hasValue() const;
    virtual std::string name() const;
    virtual std::string documentation() const;

#ifdef UFE_V3_FEATURES_AVAILABLE
    virtual Ufe::Value getMetadata(const std::string& key) const;
    virtual bool       setMetadata(const std::string& key, const Ufe::Value& value);
    virtual bool       hasMetadata(const std::string& key) const;
#endif

    virtual PXR_NS::UsdPrim                      usdPrim() const { return _usdAttr.GetPrim(); }
    virtual PXR_NS::SdfValueTypeName             usdAttributeType() const;
    virtual Ufe::AttributeEnumString::EnumValues getEnumValues() const;

private:
    PXR_NS::SdrShaderPropertyConstPtr _sdrProp;
    PXR_NS::UsdShadeAttributeType     _sdrType;

    void _CreateUsdAttribute();
}; // UsdShaderAttributeHolder

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
