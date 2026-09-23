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
#ifndef USDUFE_USDSHADERATTRIBUTEHOLDER_H
#define USDUFE_USDSHADERATTRIBUTEHOLDER_H

#include <usdUfe/ufe/UsdAttributeHolder.h>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/utils.h>

#include <ufe/attribute.h>

namespace USDUFE_NS_DEF {

//! \brief Internal helper class holding a SdrShaderProperty, providing services to transparently
// handle it as if it was a native USD attribute found in a regular schema:
class UsdShaderAttributeHolder : public UsdAttributeHolder
{
    typedef UsdAttributeHolder _Base;

public:
    UsdShaderAttributeHolder(
        PXR_NS::UsdPrim                   usdPrim,
        PXR_NS::SdrShaderPropertyConstPtr sdrProp,
        PXR_NS::UsdShadeAttributeType     sdrType);

    static UPtr create(
        PXR_NS::UsdPrim                   usdPrim,
        PXR_NS::SdrShaderPropertyConstPtr sdrProp,
        PXR_NS::UsdShadeAttributeType     sdrType);
    virtual ~UsdShaderAttributeHolder() = default;

    std::string     isEditAllowedMsg() const override;
    PXR_NS::VtValue defaultValue() const override;
    std::string     defaultValueAsString() const override;
    std::string     nativeType() const override;
    bool            get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const override;
    bool            set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) override;

    bool        hasValue() const override;
    std::string name() const override;
    std::string displayName() const override;
    std::string documentation() const override;

#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::Value getMetadata(const std::string& key) const override;
    bool       setMetadata(const std::string& key, const Ufe::Value& value) override;
    bool       hasMetadata(const std::string& key) const override;
#endif

    PXR_NS::UsdPrim                      usdPrim() const override { return _usdAttr.GetPrim(); }
    PXR_NS::SdfValueTypeName             usdAttributeType() const override;
    Ufe::AttributeEnumString::EnumValues getEnumValues() const override;
    EnumOptions                          getEnums() const override;

private:
    PXR_NS::SdrShaderPropertyConstPtr _sdrProp;
    PXR_NS::UsdShadeAttributeType     _sdrType;

    void _CreateUsdAttribute();
}; // UsdShaderAttributeHolder

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDSHADERATTRIBUTEHOLDER_H
