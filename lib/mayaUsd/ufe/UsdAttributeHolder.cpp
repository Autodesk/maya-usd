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
#include "UsdAttributeHolder.h"

#include "Utils.h"
#include "private/UfeNotifGuard.h"

#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/editRouterContext.h>
#include <mayaUsd/utils/util.h>
#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/base/tokens.h>
#endif

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace {
#ifdef UFE_V3_FEATURES_AVAILABLE

static constexpr char kErrorMsgInvalidValueType[] = "Unexpected Ufe::Value type";

bool setUsdAttrMetadata(
    const PXR_NS::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           value)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    MayaUsd::ufe::InSetAttribute inSetAttr;

    // Special cases for known Ufe metadata keys.

    // Note: we allow the locking attribute to be changed even if attribute is locked
    //       since that is how you unlock.
    if (key == Ufe::Attribute::kLocked) {
        return attr.SetMetadata(
            MayaUsdMetadata->Lock, value.get<bool>() ? MayaUsdTokens->On : MayaUsdTokens->Off);
    }

    // If attribute is locked don't allow setting Metadata.
    MayaUsd::ufe::enforceAttributeEditAllowed(attr);

    MAYAUSD_NS_DEF::AttributeEditRouterContext ctx(attr.GetPrim(), attr.GetName());

    PXR_NS::TfToken tok(key);
    if (PXR_NS::UsdShadeNodeGraph(attr.GetPrim())) {
        if (PXR_NS::UsdShadeInput::IsInput(attr)) {
            PXR_NS::UsdShadeInput input(attr);
            input.SetSdrMetadataByKey(tok, value.get<std::string>());
            return true;
        } else if (PXR_NS::UsdShadeOutput::IsOutput(attr)) {
            PXR_NS::UsdShadeOutput output(attr);
            output.SetSdrMetadataByKey(tok, value.get<std::string>());
            return true;
        }
    }

    // We must convert the Ufe::Value to VtValue for storage in Usd.
    // Figure out the type of the input Ufe Value and create proper Usd VtValue.
    PXR_NS::VtValue usdValue;
    if (value.isType<bool>())
        usdValue = value.get<bool>();
    else if (value.isType<int>())
        usdValue = value.get<int>();
    else if (value.isType<float>())
        usdValue = value.get<float>();
    else if (value.isType<double>())
        usdValue = value.get<double>();
    else if (value.isType<std::string>())
        usdValue = value.get<std::string>();
    else {
        TF_CODING_ERROR(kErrorMsgInvalidValueType);
    }
    if (!usdValue.IsEmpty()) {
        PXR_NS::TfToken tok(key);
        return attr.SetMetadata(tok, usdValue);
    }
    return false;
}
#endif

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// UsdAttributeHolder:
//------------------------------------------------------------------------------

UsdAttributeHolder::UsdAttributeHolder(const PXR_NS::UsdAttribute& usdAttr)
    : _usdAttr(usdAttr)
{
}

UsdAttributeHolder::UPtr UsdAttributeHolder::create(const PXR_NS::UsdAttribute& usdAttr)
{
    return std::unique_ptr<UsdAttributeHolder>(new UsdAttributeHolder(usdAttr));
}

std::string UsdAttributeHolder::isEditAllowedMsg() const
{
    if (isValid()) {
        PXR_NS::UsdPrim prim = _usdAttr.GetPrim();

        // Edit routing is done by a user-provided implementation that can raise exceptions.
        // In particular, they can raise an exception to prevent the execution of the associated
        // command. This is directly relevant for this check of allowed edits.
        try {
            std::string                errMsg;
            AttributeEditRouterContext ctx(prim, _usdAttr.GetName());
            isAttributeEditAllowed(_usdAttr, &errMsg);
            return errMsg;
        } catch (std::exception&) {
            return "Editing has been prevented by edit router.";
        }
    } else {
        return "Editing is not allowed.";
    }
}

std::string UsdAttributeHolder::defaultValue() const { return std::string(); }

std::string UsdAttributeHolder::nativeType() const
{
    return usdAttributeType().GetType().GetTypeName();
}

bool UsdAttributeHolder::get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const
{
    if (hasValue()) {
        return _usdAttr.Get(&value, time);
    } else {
        return false;
    }
}

bool UsdAttributeHolder::set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time)
{
    if (!isValid()) {
        PXR_NS::VtValue currentValue;
        get(currentValue, time);
        if (currentValue == value) {
            return true;
        } else {
            return false;
        }
    }

    AttributeEditRouterContext ctx(_usdAttr.GetPrim(), _usdAttr.GetName());

    InSetAttribute inSetAttr;
    return _usdAttr.Set(value, time);
}

bool UsdAttributeHolder::hasValue() const { return isValid() ? _usdAttr.HasValue() : false; }

std::string UsdAttributeHolder::name() const
{
    if (isValid()) {
        return _usdAttr.GetName().GetString();
    } else {
        return std::string();
    }
}

std::string UsdAttributeHolder::documentation() const
{
    if (isValid()) {
        return _usdAttr.GetDocumentation();
    } else {
        return std::string();
    }
}

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::Value UsdAttributeHolder::getMetadata(const std::string& key) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (isValid()) {
        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            PXR_NS::TfToken lock;
            bool            ret = _usdAttr.GetMetadata(MayaUsdMetadata->Lock, &lock);
            if (ret)
                return Ufe::Value((lock == MayaUsdTokens->On) ? true : false);
            return Ufe::Value();
        }
        PXR_NS::TfToken tok(key);
        if (PXR_NS::UsdShadeNodeGraph(usdPrim())) {
            if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                const PXR_NS::UsdShadeInput input(_usdAttr);
                std::string                 metadata = input.GetSdrMetadataByKey(tok);
                if (metadata.empty() && key == MayaUsdMetadata->UIName) {
                    // Strip and prettify:
                    metadata = UsdMayaUtil::prettifyName(input.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                const PXR_NS::UsdShadeOutput output(_usdAttr);
                std::string                  metadata = output.GetSdrMetadataByKey(tok);
                if (metadata.empty() && key == MayaUsdMetadata->UIName) {
                    // Strip and prettify:
                    metadata = UsdMayaUtil::prettifyName(output.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            }
        }
        if (key == MayaUsdMetadata->UIName) {
            std::string niceName;
            // Non-shader case, but we still have light inputs and outputs to deal with:
            if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                const PXR_NS::UsdShadeInput input(_usdAttr);
                niceName = input.GetBaseName().GetString();
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                const PXR_NS::UsdShadeOutput output(_usdAttr);
                niceName = output.GetBaseName().GetString();
            } else {
                niceName = _usdAttr.GetName().GetString();
            }

            const bool isNamespaced = niceName.find(":") != std::string::npos;
            niceName = UsdMayaUtil::prettifyName(niceName);

            if (!isNamespaced) {
                return Ufe::Value(niceName);
            }

#if PXR_VERSION > 2203
            // We can further prettify the nice name by removing all prefix that are an exact copy
            // of the applied schema name.
            //
            // For example an attribute named ui:nodegraph:node:pos found in UsdUINodeGraphNodeAPI
            // can be further simplified to "Pos".
            using DefVec = std::vector<std::pair<TfToken, const UsdPrimDefinition*>>;
            DefVec defsToExplore;

            const UsdSchemaRegistry& schemaReg = UsdSchemaRegistry::GetInstance();
            for (auto&& name : _usdAttr.GetPrim().GetAppliedSchemas()) {

                std::pair<TfToken, TfToken> typeNameAndInstance
                    = schemaReg.GetTypeNameAndInstance(name);

                const UsdPrimDefinition* primDef
                    = schemaReg.FindAppliedAPIPrimDefinition(typeNameAndInstance.first);
                if (!primDef) {
                    primDef = schemaReg.FindConcretePrimDefinition(typeNameAndInstance.first);
                }
                if (!primDef) {
                    continue;
                }
                defsToExplore.emplace_back(name, primDef);
            }

            // Sort the schemas by number of applied schemas to make sure we associate the attribute
            // to the smallest schema that defines it.
            std::sort(
                defsToExplore.begin(),
                defsToExplore.end(),
                [](const DefVec::value_type& a, const DefVec::value_type& b) {
                    return a.second->GetAppliedAPISchemas().size()
                        < b.second->GetAppliedAPISchemas().size();
                });

            for (auto&& nameAndPrimDef : defsToExplore) {
                std::pair<TfToken, TfToken> typeNameAndInstance
                    = schemaReg.GetTypeNameAndInstance(nameAndPrimDef.first);
                if (typeNameAndInstance.second.IsEmpty()) {
                    const auto& names = nameAndPrimDef.second->GetPropertyNames();
                    if (std::find(names.cbegin(), names.cend(), _usdAttr.GetName())
                        == names.cend()) {
                        continue;
                    }
                } else {
                    // Multi-apply schema. There is a nice gymnastic to do if we want to prove the
                    // attribute belongs to that schema.
                    const auto& names = nameAndPrimDef.second->GetPropertyNames();
                    if (names.empty()) {
                        continue;
                    }

                    // Get the template from the first attribute name to build the instance prefix.
                    // USD currently uses __INSTANCE_NAME__, but there is no way to programmatically
                    // get that string. Look for a double underscore instead.
                    std::string prefix = names.front();
                    size_t      dunderPos = prefix.find("__");
                    if (dunderPos == std::string::npos) {
                        continue;
                    }

                    prefix = prefix.substr(0, dunderPos) + typeNameAndInstance.second.GetString()
                        + ":";

                    // If the parameter name does not start with the template, it does not belong to
                    // this API:
                    if (_usdAttr.GetName().GetString().rfind(prefix, 0) == std::string::npos) {
                        continue;
                    }
                }

                // Now we want to strip any token sequence found in the schema API name.
                //
                // A few examples:
                //
                //   Namespaced name:                  | API name                 | Nice name:
                //   ----------------------------------+--------------------------+-----------
                //   shaping:cone:angle                | ShapingAPI               | Cone Angle
                //   ui:nodegraph:node:pos             | NodeGraphNodeAPI         | Ui Pos
                //   collections:lightLink:includeRoot | CollectionAPI(LightLink) | Include Root
                //
                // You can already note two issues with NodeGraph. First the namespace begins with
                // "ui", and second, "nodegraph" is not camelCased which means it will prettify
                // as a single token instead of two. We somehow need to include these observations
                // when pruning schema name tokens.

                // If the schema name ends with API, trim that.
                std::string schemaName = typeNameAndInstance.first;
                if (schemaName.find("API", schemaName.size() - 3) != std::string::npos) {
                    schemaName = schemaName.substr(0, schemaName.size() - 3);
                }

                // Add the instance name for multi-apply schemas:
                if (!typeNameAndInstance.second.IsEmpty()) {
                    schemaName += typeNameAndInstance.second.GetString();
                }

                // Lowercase everything (because "nodegraph"):
                auto toLower = [](std::string& s) {
                    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
                        return std::tolower(c);
                    });
                };
                toLower(schemaName);

                // Split the attribute nice names into tokens and see if we find a sequence of
                // tokens that match the lowercased schema name.
                auto attributeTokens = PXR_NS::TfStringSplit(niceName, " ");

                // Rebuild nice name using the tokens:
                niceName = "";
                auto tokenIt = attributeTokens.begin();
                while (tokenIt != attributeTokens.end()) {
                    size_t substringSize = tokenIt->size();
                    auto   lastIt = tokenIt + 1;
                    while (substringSize < schemaName.size() && lastIt != attributeTokens.end()) {
                        substringSize += lastIt->size();
                        ++lastIt;
                    }

                    // We have enough information to build a substring.
                    if (substringSize == schemaName.size()) {
                        std::string substring;
                        for (auto it = tokenIt; it != lastIt; ++it) {
                            substring += *it;
                        }
                        toLower(substring);
                        if (substring == schemaName) {
                            // Exact match. We can skip these tokens:
                            tokenIt = lastIt;
                            break;
                        }
                    }

                    // No match here. Add the current token to the nice name:
                    if (niceName.empty()) {
                        niceName = *tokenIt;
                    } else {
                        niceName += " " + *tokenIt;
                    }

                    // Get ready for next iteration:
                    ++tokenIt;

                    // We can exit the loop if we have run out of tokens and can not build a
                    // substring of sufficient size.
                    if (lastIt == attributeTokens.end() && substringSize < schemaName.size()) {
                        break;
                    }
                }
                // Add whatever token remains to the nice name:
                for (; tokenIt != attributeTokens.end(); ++tokenIt) {
                    if (niceName.empty()) {
                        niceName = *tokenIt;
                    } else {
                        niceName += " " + *tokenIt;
                    }
                }
            }
#endif
            return Ufe::Value(niceName);
        }
        PXR_NS::VtValue v;
        if (_usdAttr.GetMetadata(tok, &v)) {
            if (v.IsHolding<bool>())
                return Ufe::Value(v.Get<bool>());
            else if (v.IsHolding<int>())
                return Ufe::Value(v.Get<int>());
            else if (v.IsHolding<float>())
                return Ufe::Value(v.Get<float>());
            else if (v.IsHolding<double>())
                return Ufe::Value(v.Get<double>());
            else if (v.IsHolding<std::string>())
                return Ufe::Value(v.Get<std::string>());
            else if (v.IsHolding<PXR_NS::TfToken>())
                return Ufe::Value(v.Get<PXR_NS::TfToken>().GetString());
            else {
                std::stringstream ss;
                ss << v;
                return Ufe::Value(ss.str());
            }
        }
    }
    return Ufe::Value();
}

bool UsdAttributeHolder::setMetadata(const std::string& key, const Ufe::Value& value)
{
    if (isValid())
        return setUsdAttrMetadata(_usdAttr, key, value);
    else {
        return false;
    }
}

bool UsdAttributeHolder::clearMetadata(const std::string& key)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    InSetAttribute inSetAttr;

    if (isValid()) {
        AttributeEditRouterContext ctx(_usdAttr.GetPrim(), _usdAttr.GetName());

        PXR_NS::TfToken tok(key);
        // Special cases for NodeGraphs:
        if (PXR_NS::UsdShadeNodeGraph(usdPrim())) {
            if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                PXR_NS::UsdShadeInput(_usdAttr).ClearSdrMetadataByKey(tok);
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                PXR_NS::UsdShadeOutput(_usdAttr).ClearSdrMetadataByKey(tok);
            }
            return !hasMetadata(key);
        }

        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            return _usdAttr.ClearMetadata(MayaUsdMetadata->Lock);
        }
        return _usdAttr.ClearMetadata(tok);
    } else {
        return true;
    }
}

bool UsdAttributeHolder::hasMetadata(const std::string& key) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    bool result = false;
    if (isValid()) {
        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            result = _usdAttr.HasMetadata(MayaUsdMetadata->Lock);
            if (result) {
                return true;
            }
        } else if (key == MayaUsdMetadata->UIName) {
            return true;
        }

        PXR_NS::TfToken tok(key);
        // Special cases for NodeGraphs:
        if (PXR_NS::UsdShadeNodeGraph(usdPrim())) {
            if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                return PXR_NS::UsdShadeInput(_usdAttr).HasSdrMetadataByKey(tok);
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                return PXR_NS::UsdShadeOutput(_usdAttr).HasSdrMetadataByKey(tok);
            }
        }
        result = _usdAttr.HasMetadata(tok);
        if (result) {
            return true;
        }
    }
    return false;
}
#endif

PXR_NS::SdfValueTypeName UsdAttributeHolder::usdAttributeType() const
{
    return _usdAttr.GetTypeName();
}

Ufe::AttributeEnumString::EnumValues UsdAttributeHolder::getEnumValues() const
{
    Ufe::AttributeEnumString::EnumValues retVal;
    if (_usdAttr.IsValid()) {
        auto attrDefn
            = _usdAttr.GetPrim().GetPrimDefinition().GetSchemaAttributeSpec(_usdAttr.GetName());
        if (attrDefn && attrDefn->HasAllowedTokens()) {
            for (auto const& token : attrDefn->GetAllowedTokens()) {
                retVal.push_back(token.GetString());
            }
        }
    }

    return retVal;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
