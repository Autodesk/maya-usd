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

#include "Global.h"
#include "Utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usdShade/utils.h>

#include <ufe/runTimeMgr.h>
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

namespace {

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::AttributeDef::ConstPtr
nameToAttrDef(const PXR_NS::TfToken& tokName, const Ufe::NodeDef::Ptr& nodeDef)
{
    auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(tokName);
    Ufe::AttributeDef::ConstPtr attrDef
        = baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input
        ? nodeDef->input(baseNameAndType.first)
        : nodeDef->output(baseNameAndType.first);
    return attrDef;
}

Ufe::NodeDefHandler::Ptr getUsdNodeDefHandler()
{
    static Ufe::NodeDefHandler::Ptr nodeDefHandler = nullptr;
    if (!nodeDefHandler) {
        Ufe::RunTimeMgr& runTimeMgr = Ufe::RunTimeMgr::instance();
        nodeDefHandler = runTimeMgr.nodeDefHandler(getUsdRunTimeId());
    }
    return nodeDefHandler;
}
#endif

} // namespace

UsdAttributes::UsdAttributes(const UsdSceneItem::Ptr& item)
    : Ufe::Attributes()
    , fItem(item)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(item)) {
        throw std::runtime_error("Invalid attributes object");
    }
    fPrim = item->prim();
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

// Returns whether the given op is an inverse operation. i.e, it starts with "!invert!".
static constexpr char   invertPrefix[] = "!invert!";
static constexpr size_t invertPrefixLen(sizeof(invertPrefix) / sizeof(invertPrefix[0]));
static bool             _IsInverseOp(const std::string& name)
{
    return PXR_NS::TfStringStartsWith(name, invertPrefix);
}

static PXR_NS::UsdAttribute _GetAttributeType(const PXR_NS::UsdPrim& prim, const std::string& name)
{
    PXR_NS::TfToken tok(name);
    bool            isInverseOp = _IsInverseOp(tok);
    // If it is an inverse operation, strip off the "!invert!" at the beginning
    // of name to get the associated attribute's name.
    return prim.GetAttribute(isInverseOp ? PXR_NS::TfToken(name.substr(invertPrefixLen)) : tok);
}

Ufe::Attribute::Type UsdAttributes::attributeType(const std::string& name)
{
    // If we've already created an attribute for this name, just return the type.
    auto iter = fAttributes.find(name);
    if (iter != std::end(fAttributes))
        return iter->second->type();

    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = _GetAttributeType(fPrim, name);
    if (usdAttr.IsValid()) {
        return getUfeTypeForAttribute(usdAttr);
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::NodeDef::Ptr nodeDef = UsdAttributes::nodeDef();
    if (!nodeDef) {
        return Ufe::Attribute::kInvalid;
    }
    Ufe::AttributeDef::ConstPtr attrDef = nameToAttrDef(tok, nodeDef);
    if (attrDef) {
        return attrDef->type();
    }
#endif
    return Ufe::Attribute::kInvalid;
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
    PXR_NS::UsdAttribute usdAttr = _GetAttributeType(fPrim, name);
    Ufe::Attribute::Type newAttrType;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::AttributeDef::ConstPtr attributeDef = nullptr;
    Ufe::NodeDef::Ptr           nodeDef = UsdAttributes::nodeDef();
    if (nodeDef) {
        attributeDef = nameToAttrDef(tok, nodeDef);
        if (attributeDef) {
            newAttrType = attributeDef->type();
        }
    }
#endif
    if (usdAttr.IsValid()) {
        newAttrType = attributeType(name);
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else if (!nodeDef) {
        return nullptr;
    }
#endif

    // Use a map of constructors to reduce the number of string comparisons. Since the naming
    // convention is extremely uniform, let's use a macro to simplify definition (and prevent
    // mismatch errors).
#ifdef UFE_V4_FEATURES_AVAILABLE
#define ADD_UFE_USD_CTOR(TYPE)                                            \
    {                                                                     \
        Ufe::Attribute::k##TYPE,                                          \
            [](const UsdSceneItem::Ptr&           si,                     \
               const PXR_NS::UsdPrim&             prim,                   \
               const Ufe::AttributeDef::ConstPtr& attrDef,                \
               const PXR_NS::UsdAttribute&        usdAttr) {                     \
                if (usdAttr) {                                            \
                    return UsdAttribute##TYPE::create(si, usdAttr);       \
                } else {                                                  \
                    return UsdAttribute##TYPE::create(si, prim, attrDef); \
                }                                                         \
            }                                                             \
    }
#else
#define ADD_UFE_USD_CTOR(TYPE)                                                     \
    {                                                                              \
        Ufe::Attribute::k##TYPE,                                                   \
            [](const UsdSceneItem::Ptr& si, const PXR_NS::UsdAttribute& usdAttr) { \
                return UsdAttribute##TYPE::create(si, usdAttr);                    \
            }                                                                      \
    }
#endif
    static const std::unordered_map<
        std::string,
#ifdef UFE_V4_FEATURES_AVAILABLE
        std::function<Ufe::Attribute::Ptr(
            const UsdSceneItem::Ptr&,
            const PXR_NS::UsdPrim&,
            const Ufe::AttributeDef::ConstPtr&,
            const PXR_NS::UsdAttribute&)>>
#else
        std::function<Ufe::Attribute::Ptr(const UsdSceneItem::Ptr&, const PXR_NS::UsdAttribute&)>>
#endif
        ctorMap
        = { ADD_UFE_USD_CTOR(Bool),
            ADD_UFE_USD_CTOR(Int),
            ADD_UFE_USD_CTOR(Float),
            ADD_UFE_USD_CTOR(Double),
            ADD_UFE_USD_CTOR(String),
            ADD_UFE_USD_CTOR(ColorFloat3),
            ADD_UFE_USD_CTOR(EnumString),
            ADD_UFE_USD_CTOR(Int3),
            ADD_UFE_USD_CTOR(Float3),
            ADD_UFE_USD_CTOR(Double3),
            ADD_UFE_USD_CTOR(Generic),
#if (UFE_PREVIEW_VERSION_NUM >= 4015)
            ADD_UFE_USD_CTOR(ColorFloat4),
            ADD_UFE_USD_CTOR(Filename),
            ADD_UFE_USD_CTOR(Float2),
            ADD_UFE_USD_CTOR(Float4),
            ADD_UFE_USD_CTOR(Matrix3d),
            ADD_UFE_USD_CTOR(Matrix4d),
#endif
          };

#undef ADD_UFE_USD_CTOR
    auto                ctorIt = ctorMap.find(newAttrType);
    Ufe::Attribute::Ptr newAttr;
    UFE_ASSERT_MSG(ctorIt != ctorMap.end(), kErrorMsgUnknown);
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (ctorIt != ctorMap.end())
        newAttr = ctorIt->second(fItem, fPrim, attributeDef, usdAttr);
#else
    if (ctorIt != ctorMap.end())
        newAttr = ctorIt->second(fItem, usdAttr);
#endif

    fAttributes[name] = newAttr;
    return newAttr;
}

std::vector<std::string> UsdAttributes::attributeNames() const
{
    std::vector<std::string> names;
    std::set<std::string>    nameSet;
    std::string              name;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::NodeDef::Ptr nodeDef = UsdAttributes::nodeDef();
    if (nodeDef) {
        auto addAttributeNames
            = [&names, &nameSet, &name](
                  Ufe::ConstAttributeDefs attributeDefs, PXR_NS::UsdShadeAttributeType attrType) {
                  for (auto const& attributeDef : attributeDefs) {
                      name = PXR_NS::UsdShadeUtils::GetFullName(
                          PXR_NS::TfToken(attributeDef->name()), attrType);
                      names.push_back(name);
                      nameSet.insert(name);
                  }
              };
        addAttributeNames(nodeDef->inputs(), PXR_NS::UsdShadeAttributeType::Input);
        addAttributeNames(nodeDef->outputs(), PXR_NS::UsdShadeAttributeType::Output);
    }
#endif
    if (fPrim) {
        auto primAttrs = fPrim.GetAttributes();
        for (const auto& attr : primAttrs) {
            name = attr.GetName();
            if (nameSet.find(name) == nameSet.end()) {
                names.push_back(name);
            }
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
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::NodeDef::Ptr nodeDef = UsdAttributes::nodeDef();
    if (nodeDef) {
        Ufe::AttributeDef::ConstPtr port = nameToAttrDef(tkName, nodeDef);
        return port != nullptr;
    }
#endif
    return false;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::NodeDef::Ptr UsdAttributes::nodeDef() const
{
    static auto nodeDefHandler = getUsdNodeDefHandler();
    return nodeDefHandler->definition(fItem);
}
#endif

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
