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

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/Attribute.h>
#include <ufe/Value.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Internal helper class holding a USD attributes for query:
class UsdAttributeHolder
{
public:
    UsdAttributeHolder(const PXR_NS::UsdAttribute& usdAttr);
    ~UsdAttributeHolder() = default;

    virtual bool        isAuthored() const { return isValid() && _usdAttr.IsAuthored(); }
    virtual bool        isValid() const { return _usdAttr.IsValid(); }
    virtual std::string isEditAllowedMsg() const;
    virtual bool        isEditAllowed() const { return isEditAllowedMsg().empty(); }
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
    virtual bool       clearMetadata(const std::string& key);
    virtual bool       hasMetadata(const std::string& key) const;
#endif

    virtual PXR_NS::UsdPrim                      usdPrim() const { return _usdAttr.GetPrim(); }
    virtual PXR_NS::UsdAttribute                 usdAttribute() const { return _usdAttr; }
    virtual PXR_NS::SdfValueTypeName             usdAttributeType() const;
    virtual Ufe::AttributeEnumString::EnumValues getEnumValues() const;

protected:
    PXR_NS::UsdAttribute _usdAttr;
}; // UsdAttributeHolder

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
