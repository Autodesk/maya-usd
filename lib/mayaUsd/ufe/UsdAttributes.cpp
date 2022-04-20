//
// Copyright 2019 Autodesk
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
#include "UsdAttributes.h"
#include "UsdShaderNodeDefHandler.h"

#include "Utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>

#include <ufe/ufeAssert.h>

// Note: normally we would use this using directive, but here we cannot because
//		 one of our classes is called UsdAttribute which is exactly the same as
//		the one in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

#ifdef UFE_ENABLE_ASSERTS
static constexpr char kErrorMsgUnknown[] = "Unknown UFE attribute type encountered";
static constexpr char kErrorMsgInvalidAttribute[] = "Invalid USDAttribute!";
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

const std::string INPUT_ATTR_PREFIX = "inputs:";
const std::string OUTPUT_ATTR_PREFIX = "outputs:";

UsdAttributes::UsdAttributes(const UsdSceneItem::Ptr& item)
    : Ufe::Attributes()
    , fItem(item)
    , fNodeDef(nullptr)
{
    if (item) {
        fPrim = item->prim();
    }
    UsdShaderNodeDefHandler::Ptr shaderNodeDefHandler = UsdShaderNodeDefHandler::create();
    fNodeDef = shaderNodeDefHandler->definition(item);
}

UsdAttributes::~UsdAttributes() { }

/*static*/
UsdAttributes::Ptr UsdAttributes::create(const UsdSceneItem::Ptr& item)
{
    auto attrs = std::make_shared<UsdAttributes>(item);
    return attrs;
}

//------------------------------------------------------------------------------
// Ufe::Attributes overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdAttributes::sceneItem() const { return fItem; }

Ufe::Attribute::Type UsdAttributes::attributeType(const std::string& name)
{
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    if (usdAttr.IsValid()) {
        return getUfeTypeForAttribute(usdAttr);
    } else {
        if (!fNodeDef) {
            return nullptr;
        }
        Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
        for (auto const& input : inputs) {
            if (INPUT_ATTR_PREFIX + input->name() == name) {
                return input->type();
            }
        }
        Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
        for (auto const& output : outputs) {
            if (INPUT_ATTR_PREFIX + output->name() == name) {
                return output->type();
            }
        }
    }
    return nullptr;
}

Ufe::Attribute::Ptr UsdAttributes::attribute(const std::string& name)
{
    // early return if name is empty.
    if (name.empty()) {
        return nullptr;
    }

    // If we've already created an attribute for this name, just return it.
    auto iter = fAttributes.find(name);
    if (iter != std::end(fAttributes))
        return iter->second;

    // No attribute for the input name was found -> create one.
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    AttrHandle::Ptr attrHandle = nullptr;
    Ufe::Attribute::Type newAttrType;
    if (usdAttr.IsValid()) {
        attrHandle = std::make_shared<AttrHandle>(fPrim, usdAttr);
        newAttrType = getUfeTypeForAttribute(usdAttr);
    } else {
        if (!fNodeDef) {
            return nullptr;
        }
        Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
        for (auto const& input : inputs) {
            if (INPUT_ATTR_PREFIX + input->name() == name) {
                attrHandle = std::make_shared<AttrDefHandle>(fPrim, input);
                newAttrType = input->type();
            }
        }
        if (!attrHandle) {
            Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
            for (auto const& output : outputs) {
                if (INPUT_ATTR_PREFIX + output->name() == name) {
                    attrHandle = std::make_shared<AttrDefHandle>(fPrim, output);
                    newAttrType = output->type();
                }
            }
        }
    }
    if (!attrHandle) {
        return nullptr;
    }

    Ufe::Attribute::Ptr newAttr;
    if (newAttrType == Ufe::Attribute::kBool) {
        newAttr = UsdAttributeBool::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kInt) {
        newAttr = UsdAttributeInt::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kFloat) {
        newAttr = UsdAttributeFloat::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kDouble) {
        newAttr = UsdAttributeDouble::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kString) {
        newAttr = UsdAttributeString::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kColorFloat3) {
        newAttr = UsdAttributeColorFloat3::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kEnumString) {
        newAttr = UsdAttributeEnumString::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kInt3) {
        newAttr = UsdAttributeInt3::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kFloat3) {
        newAttr = UsdAttributeFloat3::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kDouble3) {
        newAttr = UsdAttributeDouble3::create(fItem, attrHandle);
    } else if (newAttrType == Ufe::Attribute::kGeneric) {
        newAttr = UsdAttributeGeneric::create(fItem, attrHandle);
    } else {
        UFE_ASSERT_MSG(false, kErrorMsgUnknown);
    }
    fAttributes[name] = newAttr;
    return newAttr;
}

std::vector<std::string> UsdAttributes::attributeNames() const
{
    std::vector<std::string> names;
    if (fNodeDef) {
        Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
        for (auto const& input : inputs) {
            names.push_back(INPUT_ATTR_PREFIX + input->name());
        }
        Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
        for (auto const& output : outputs) {
            names.push_back(INPUT_ATTR_PREFIX + output->name());
        }
    } else if (fPrim) {
        auto primAttrs = fPrim.GetAttributes();
        names.reserve(primAttrs.size());
        for (const auto& attr : primAttrs) {
            names.push_back(attr.GetName());
        }
    }
    return names;
}

bool UsdAttributes::hasAttribute(const std::string& name) const
{
    PXR_NS::TfToken tkName(name);
    if (fPrim.HasAttribute(tkName)) {
        return true;
    }
    Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
    for (auto const& input : inputs) {
        if (input->name() == tkName.GetString()) {
            return true;
        }
    }
    Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
    for (auto const& output : outputs) {
        if (output->name() == tkName.GetString()) {
            return true;
        }
    }
    return false;
}

Ufe::Attribute::Type
UsdAttributes::getUfeTypeForAttribute(const PXR_NS::UsdAttribute& usdAttr) const
{
    if (usdAttr.IsValid()) {
        const PXR_NS::SdfValueTypeName typeName = usdAttr.GetTypeName();
        Ufe::Attribute::Type           type = usdTypeToUfe(typeName);
        // Special case for TfToken -> Enum. If it doesn't have any allowed
        // tokens, then use String instead.
        if (type == Ufe::Attribute::kEnumString) {
            auto attrDefn = fPrim.GetPrimDefinition().GetSchemaAttributeSpec(usdAttr.GetName());
            if (!attrDefn || !attrDefn->HasAllowedTokens())
                return Ufe::Attribute::kString;
        }
        return type;
    }

    UFE_ASSERT_MSG(false, kErrorMsgInvalidAttribute);
    return Ufe::Attribute::kInvalid;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
