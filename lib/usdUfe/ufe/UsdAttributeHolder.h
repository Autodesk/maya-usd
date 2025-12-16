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
#ifndef USDUFE_USDATTRIBUTEHOLDER_H
#define USDUFE_USDATTRIBUTEHOLDER_H

#include <usdUfe/base/api.h>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/relationship.h>

#include <ufe/attribute.h>

namespace USDUFE_NS_DEF {

//! \brief Internal helper class holding a USD attributes for query:
class UsdAttributeHolder
{
public:
    UsdAttributeHolder(const PXR_NS::UsdProperty& usdProp);
    typedef std::unique_ptr<UsdAttributeHolder> UPtr;
    static UPtr                                 create(const PXR_NS::UsdProperty& usdProp);
    virtual ~UsdAttributeHolder() = default;

    virtual bool isAuthored() const { return isValid() && _usdAttr.IsAuthored(); }
    virtual bool isValidAttribute() const
    {
        return isAttribute() ? usdAttribute().IsValid() : false;
    }
    virtual bool isValidRelationship() const
    {
        return isRelationship() ? usdRelationship().IsValid() : false;
    }
    virtual bool        isValid() const { return isValidAttribute() || isValidRelationship(); }
    virtual std::string isEditAllowedMsg() const;
    virtual bool        isEditAllowed() const { return isEditAllowedMsg().empty(); }
    virtual std::string defaultValue() const;
    virtual std::string nativeType() const;
    virtual bool        get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const;
    virtual bool        set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time);
    virtual bool        isDefault();
    virtual void        reset();

    virtual bool        hasValue() const;
    virtual std::string name() const;
    virtual std::string displayName() const;
    virtual std::string documentation() const;

#ifdef UFE_V3_FEATURES_AVAILABLE
    virtual Ufe::Value getMetadata(const std::string& key) const;
    virtual bool       setMetadata(const std::string& key, const Ufe::Value& value);
    virtual bool       clearMetadata(const std::string& key);
    virtual bool       hasMetadata(const std::string& key) const;
#endif

    virtual PXR_NS::UsdPrim usdPrim() const { return _usdAttr.GetPrim(); }
    virtual bool            isAttribute() const { return _usdAttr.Is<PXR_NS::UsdAttribute>(); }
    virtual bool isRelationship() const { return _usdAttr.Is<PXR_NS::UsdRelationship>(); }
    virtual PXR_NS::UsdAttribute usdAttribute() const
    {
        return isAttribute() ? _usdAttr.As<PXR_NS::UsdAttribute>() : PXR_NS::UsdAttribute();
    }
    virtual PXR_NS::UsdRelationship usdRelationship() const
    {
        return isRelationship() ? _usdAttr.As<PXR_NS::UsdRelationship>()
                                : PXR_NS::UsdRelationship();
    }
    virtual PXR_NS::UsdProperty                  usdProperty() const { return _usdAttr; }
    virtual PXR_NS::SdfValueTypeName             usdAttributeType() const;
    virtual Ufe::AttributeEnumString::EnumValues getEnumValues() const;
    using EnumOptions = std::vector<std::pair<std::string, std::string>>;
    virtual EnumOptions getEnums() const;

protected:
    PXR_NS::UsdProperty _usdAttr;
}; // UsdAttributeHolder

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDATTRIBUTEHOLDER_H
