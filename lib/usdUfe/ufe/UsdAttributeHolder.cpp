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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/Utils.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/stage.h>

#include <unordered_set>

namespace {
#ifdef UFE_V3_FEATURES_AVAILABLE

static constexpr char kErrorMsgInvalidValueType[] = "Unexpected Ufe::Value type";

bool setUsdNativeMetadata(
    const PXR_NS::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           value)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!UsdShadeInput::IsInput(attr) && !UsdShadeOutput::IsOutput(attr)) {
        // Only allow for shader ports at this time. Could potentially expand
        // to dynamic attributes if requested. Editing static attributes is
        // not recommended.
        return false;
    }
    if (key == UsdUfe::MetadataTokens->UIDoc) {
        attr.SetDocumentation(value.get<std::string>());
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIEnumLabels) {
        const auto   enumStrings = UsdUfe::splitString(value.get<std::string>(), ",");
        VtTokenArray allowedTokens;
        allowedTokens.reserve(enumStrings.size());
        for (const auto& tokenString : enumStrings) {
            allowedTokens.push_back(TfToken(TfStringTrim(tokenString, " ")));
        }
        attr.SetMetadata(SdfFieldKeys->AllowedTokens, allowedTokens);
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIFolder) {
        // Translate '|' to ':'.
        // This is to transform nested group separators that differ between platform.
        // MaterialX uses "/". See "NodeDef Parameter Interface" in the spec.
        // USD uses ":". See documentation for "SetDisplayGroup".
        // UFE uses "|". Undocumented, but used in LookdevX.
        // All three agree that the topmost group is on the left.
        std::string group = value.get<std::string>();
        std::replace(group.begin(), group.end(), '|', ':');
        attr.SetDisplayGroup(group);
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIName) {
        attr.SetDisplayName(value.get<std::string>());
        return true;
    }

    return false;
}

bool hasUsdNativeMetadata(const PXR_NS::UsdAttribute& attr, const std::string& key)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!UsdShadeInput::IsInput(attr) && !UsdShadeOutput::IsOutput(attr)) {
        // Only allow for shader ports at this time. Could potentially expand
        // to dynamic attributes if requested. Editing static attributes is
        // not recommended.
        return false;
    }
    if (key == UsdUfe::MetadataTokens->UIDoc) {
        return attr.HasAuthoredDocumentation();
    } else if (key == UsdUfe::MetadataTokens->UIEnumLabels) {
        return attr.HasMetadata(SdfFieldKeys->AllowedTokens);
    } else if (key == UsdUfe::MetadataTokens->UIFolder) {
        return attr.HasAuthoredDisplayGroup();
    } else if (key == UsdUfe::MetadataTokens->UIName) {
        return attr.HasAuthoredDisplayName();
    }
    return false;
}

Ufe::Value getUsdNativeMetadata(const PXR_NS::UsdAttribute& attr, const std::string& key)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!hasUsdNativeMetadata(attr, key)) {
        return {};
    }
    Ufe::Value retVal;
    if (key == UsdUfe::MetadataTokens->UIDoc) {
        retVal = Ufe::Value(attr.GetDocumentation());
    } else if (key == UsdUfe::MetadataTokens->UIEnumLabels) {
        VtTokenArray allowedTokens;
        if (attr.GetMetadata(SdfFieldKeys->AllowedTokens, &allowedTokens)) {
            std::string enumstrings;
            for (auto const& tok : allowedTokens) {
                if (!enumstrings.empty()) {
                    enumstrings += ", ";
                }
                enumstrings += tok.GetString();
            }
            retVal = Ufe::Value(enumstrings);
        }
    } else if (key == UsdUfe::MetadataTokens->UIFolder) {
        std::string uifolder = attr.GetDisplayGroup();
        // Translate ':' to '|'.
        // This is to transform nested group separators that differ between platform.
        // MaterialX uses "/". See "NodeDef Parameter Interface" in the spec.
        // USD uses ":". See documentation for "SetDisplayGroup".
        // UFE uses "|". Undocumented, but used in LookdevX.
        // All three agree that the topmost group is on the left.
        std::replace(uifolder.begin(), uifolder.end(), ':', '|');
        retVal = Ufe::Value(uifolder);
    } else if (key == UsdUfe::MetadataTokens->UIName) {
        retVal = Ufe::Value(attr.GetDisplayName());
    }
    return retVal;
}

// Returns true if handled, and set cleared on success
bool clearUsdNativeMetadata(const PXR_NS::UsdAttribute& attr, const std::string& key, bool& cleared)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    cleared = false;
    if (!UsdShadeInput::IsInput(attr) && !UsdShadeOutput::IsOutput(attr)) {
        // Only allow for shader ports at this time. Could potentially expand
        // to dynamic attributes if requested. Editing static attributes is
        // not recommended.
        return false;
    }
    if (key == UsdUfe::MetadataTokens->UIDoc) {
        if (attr.HasAuthoredDocumentation()) {
            attr.ClearDocumentation();
            cleared = !attr.HasAuthoredDocumentation();
        }
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIEnumLabels) {
        if (attr.HasMetadata(SdfFieldKeys->AllowedTokens)) {
            attr.ClearMetadata(SdfFieldKeys->AllowedTokens);
            cleared = !attr.HasMetadata(SdfFieldKeys->AllowedTokens);
        }
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIFolder) {
        if (attr.HasAuthoredDisplayGroup()) {
            attr.ClearDisplayGroup();
            cleared = !attr.HasAuthoredDisplayGroup();
        }
        return true;
    } else if (key == UsdUfe::MetadataTokens->UIName) {
        if (attr.HasAuthoredDisplayName()) {
            attr.ClearDisplayName();
            cleared = !attr.HasAuthoredDisplayName();
        }
        return true;
    }
    return false;
}

bool setUsdAttrMetadata(
    const PXR_NS::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           value)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    UsdUfe::InSetAttribute inSetAttr;

    // Special cases for known Ufe metadata keys.

    // Note: we allow the locking attribute to be changed even if attribute is locked
    //       since that is how you unlock.
    if (key == Ufe::Attribute::kLocked) {
        return attr.SetMetadata(
            UsdUfe::MetadataTokens->Lock,
            value.get<bool>() ? UsdUfe::GenericTokens->On : UsdUfe::GenericTokens->Off);
    }

    // If attribute is locked don't allow setting Metadata.
    UsdUfe::enforceAttributeEditAllowed(attr);

    UsdUfe::AttributeEditRouterContext ctx(attr.GetPrim(), attr.GetName());

    // These metadata have a USD equivalent. Use it to stay compatible with other DCCs:
    static const auto knownUSDMetadata
        = std::unordered_set<std::string> { UsdUfe::MetadataTokens->UIDoc,
                                            UsdUfe::MetadataTokens->UIEnumLabels,
                                            UsdUfe::MetadataTokens->UIName,
                                            UsdUfe::MetadataTokens->UIFolder };

    PXR_NS::TfToken tok(key);
    if (PXR_NS::UsdShadeNodeGraph(attr.GetPrim())) {
        if (PXR_NS::UsdShadeInput::IsInput(attr)) {
            PXR_NS::UsdShadeInput input(attr);
            if (knownUSDMetadata.count(key)) {
                // Cleanup known USD metadata previously set as SdrMetadata
                input.ClearSdrMetadataByKey(tok);
            } else {
                input.SetSdrMetadataByKey(tok, value.get<std::string>());
                return true;
            }
        } else if (PXR_NS::UsdShadeOutput::IsOutput(attr)) {
            PXR_NS::UsdShadeOutput output(attr);
            if (knownUSDMetadata.count(key)) {
                // Cleanup known USD metadata previously set as SdrMetadata
                output.ClearSdrMetadataByKey(tok);
            } else {
                output.SetSdrMetadataByKey(tok, value.get<std::string>());
                return true;
            }
        }
    }

    if (setUsdNativeMetadata(attr, key, value)) {
        return true;
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

namespace USDUFE_NS_DEF {

//------------------------------------------------------------------------------
// UsdAttributeHolder:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeHolder is properly setup.
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdAttributeHolder);

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
            UsdUfe::isAttributeEditAllowed(_usdAttr, &errMsg);
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
#ifdef UFE_V3_FEATURES_AVAILABLE
    if (usdAttributeType() == SdfValueTypeNames->Token && PXR_NS::UsdShadeNodeGraph(usdPrim())) {
        // We might have saved the Sdr native type as metadata:
        if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)
            || PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
            const auto metaValue = getMetadata(UsdAttributeGeneric::nativeSdrTypeMetadata());
            if (!metaValue.empty() && metaValue.isType<std::string>()) {
                return metaValue.get<std::string>();
            }
        }
    }
#endif
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

    UsdUfe::InSetAttribute inSetAttr;
    return _usdAttr.Set(value, time);
}

bool UsdAttributeHolder::isDefault()
{
    // Checks both authored default value and authored time samples
    return !_usdAttr.HasAuthoredValue();
}

void UsdAttributeHolder::reset()
{
    // Will clear all values, including time samples.
    _usdAttr.Clear();
    _usdAttr.GetPrim().RemoveProperty(_usdAttr.GetName());
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

std::string UsdAttributeHolder::displayName() const
{
    if (isValid()) {
        std::string dn = _usdAttr.GetDisplayName();
        if (dn.empty()) {
            static const std::string prefixesToRemove = "xformOp";
            dn = _usdAttr.GetName().GetString();
            if (PXR_NS::TfStringStartsWith(dn, prefixesToRemove)) {
                dn = dn.substr(prefixesToRemove.size());
            }
            dn = prettifyName(dn);
        }
        return dn;
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
            bool            ret = _usdAttr.GetMetadata(MetadataTokens->Lock, &lock);
            if (ret)
                return Ufe::Value((lock == GenericTokens->On) ? true : false);
            return Ufe::Value();
        }

        // Handle metadata known to USD under a different key/API.
        Ufe::Value usdNativeValue = getUsdNativeMetadata(_usdAttr, key);
        if (!usdNativeValue.empty()) {
            return usdNativeValue;
        }

        PXR_NS::TfToken tok(key);
        if (PXR_NS::UsdShadeNodeGraph(usdPrim())) {
            if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                const PXR_NS::UsdShadeInput input(_usdAttr);
                std::string                 metadata = input.GetSdrMetadataByKey(tok);
                if (metadata.empty() && key == MetadataTokens->UIName) {
                    // Strip and prettify:
                    metadata = prettifyName(input.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                const PXR_NS::UsdShadeOutput output(_usdAttr);
                std::string                  metadata = output.GetSdrMetadataByKey(tok);
                if (metadata.empty() && key == MetadataTokens->UIName) {
                    // Strip and prettify:
                    metadata = prettifyName(output.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            }
        }
        if (key == MetadataTokens->UIName) {
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
            niceName = prettifyName(niceName);

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
        } else if (key == SdfFieldKeys->ColorSpace) {
            const auto csValue = _usdAttr.GetColorSpace();
            return !csValue.IsEmpty() ? Ufe::Value(csValue.GetString()) : Ufe::Value();
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
    if (isValid()) {
        if (key == SdfFieldKeys->ColorSpace) {
            if (!value.empty() && value.isType<std::string>()) {
                const AttributeEditRouterContext ctx(_usdAttr.GetPrim(), _usdAttr.GetName());
                _usdAttr.SetColorSpace(TfToken(value.get<std::string>()));
                return true;
            }
            return false;
        }
        return setUsdAttrMetadata(_usdAttr, key, value);
    } else {
        return false;
    }
}

bool UsdAttributeHolder::clearMetadata(const std::string& key)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    UsdUfe::InSetAttribute inSetAttr;

    if (isValid()) {
        AttributeEditRouterContext ctx(_usdAttr.GetPrim(), _usdAttr.GetName());

        bool cleared = false;
        if (clearUsdNativeMetadata(_usdAttr, key, cleared)) {
            return cleared;
        }

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
            return _usdAttr.ClearMetadata(MetadataTokens->Lock);
        }

        if (key == SdfFieldKeys->ColorSpace) {
            return _usdAttr.ClearColorSpace();
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
            result = _usdAttr.HasMetadata(MetadataTokens->Lock);
            if (result) {
                return true;
            }
        } else if (key == MetadataTokens->UIName) {
            return true;
        }

        if (hasUsdNativeMetadata(_usdAttr, key)) {
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
        for (auto const& option : getEnums()) {
            retVal.push_back(option.first);
        }
    }

    return retVal;
}

UsdAttributeHolder::EnumOptions UsdAttributeHolder::getEnums() const
{
    UsdAttributeHolder::EnumOptions retVal;
    if (_usdAttr.IsValid()) {
        PXR_NS::VtTokenArray allowedTokens;
        if (_usdAttr.GetPrim().GetPrimDefinition().GetPropertyMetadata(
                _usdAttr.GetName(), PXR_NS::SdfFieldKeys->AllowedTokens, &allowedTokens)) {
            for (auto const& token : allowedTokens) {
                retVal.emplace_back(token.GetString(), "");
            }
        }
        // We might have a propagated enum copied into the created NodeGraph port, resulting from
        // connecting a shader enum property.
        PXR_NS::UsdShadeNodeGraph ngPrim(_usdAttr.GetPrim());
        if (ngPrim && PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
            auto getEnumLabels = [](auto const& i) {
                std::vector<std::string> retVal;

                if (i.HasSdrMetadataByKey(UsdUfe::MetadataTokens->UIEnumLabels)) {
                    const auto enumLabels
                        = i.GetSdrMetadataByKey(UsdUfe::MetadataTokens->UIEnumLabels);
                    retVal = splitString(enumLabels, ", ");
                } else {
                    // Enum tokens can also be found at the Sdf level:
                    VtTokenArray allowedTokens;
                    if (i.GetAttr().GetMetadata(SdfFieldKeys->AllowedTokens, &allowedTokens)) {
                        for (auto const& t : allowedTokens) {
                            retVal.push_back(t);
                        }
                    }
                }
                return retVal;
            };
            const auto shaderInput = PXR_NS::UsdShadeInput { _usdAttr };
            const auto enumValues
                = shaderInput.GetSdrMetadataByKey(UsdUfe::MetadataTokens->UIEnumValues);
            const std::vector<std::string> allLabels = getEnumLabels(shaderInput);
            std::vector<std::string>       allValues = splitString(enumValues, ", ");

            if (!allValues.empty() && allValues.size() != allLabels.size()) {
                // An array of vector2 values will produce twice the expected number of
                // elements. We can fix that by regrouping them.
                if (allValues.size() > allLabels.size()
                    && allValues.size() % allLabels.size() == 0) {

                    size_t                   stride = allValues.size() / allLabels.size();
                    std::vector<std::string> rebuiltValues;
                    std::string              currentValue;
                    for (size_t i = 0; i < allValues.size(); ++i) {
                        if (i % stride != 0) {
                            currentValue += ",";
                        }
                        currentValue += allValues[i];
                        if ((i + 1) % stride == 0) {
                            rebuiltValues.push_back(currentValue);
                            currentValue = "";
                        }
                    }
                    allValues.swap(rebuiltValues);
                } else {
                    // Can not reconcile the size difference:
                    allValues.clear();
                }
            }

            const bool hasValues = allLabels.size() == allValues.size();
            for (size_t i = 0; i < allLabels.size(); ++i) {
                if (hasValues) {
                    retVal.emplace_back(allLabels[i], allValues[i]);
                } else {
                    retVal.emplace_back(allLabels[i], "");
                }
            }
        }
    }

    return retVal;
}

} // namespace USDUFE_NS_DEF
