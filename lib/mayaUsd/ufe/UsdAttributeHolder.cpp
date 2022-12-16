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

#include <mayaUsd/utils/util.h>
#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/base/tokens.h>
#endif

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>

namespace {
#ifdef UFE_V3_FEATURES_AVAILABLE

static constexpr char kErrorMsgInvalidValueType[] = "Unexpected Ufe::Value type";

bool setUsdAttrMetadata(
    const PXR_NS::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           value)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    // Special cases for known Ufe metadata keys.

    // Note: we allow the locking attribute to be changed even if attribute is locked
    //       since that is how you unlock.
    if (key == Ufe::Attribute::kLocked) {
        return attr.SetMetadata(
            MayaUsdMetadata->Lock, value.get<bool>() ? MayaUsdTokens->On : MayaUsdTokens->Off);
    }

    // If attribute is locked don't allow setting Metadata.
    std::string errMsg;
    const bool  isSetAttrAllowed = MayaUsd::ufe::isAttributeEditAllowed(attr, &errMsg);
    if (!isSetAttrAllowed) {
        throw std::runtime_error(errMsg);
    }

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
        std::string errMsg;
        isAttributeEditAllowed(_usdAttr, &errMsg);
        return errMsg;
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
                if (metadata.empty() && key == "uiname") {
                    // Strip and prettify:
                    metadata = UsdMayaUtil::prettifyName(input.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                const PXR_NS::UsdShadeOutput output(_usdAttr);
                std::string                  metadata = output.GetSdrMetadataByKey(tok);
                if (metadata.empty() && key == "uiname") {
                    // Strip and prettify:
                    metadata = UsdMayaUtil::prettifyName(output.GetBaseName().GetString());
                }
                return Ufe::Value(metadata);
            }
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
    if (isValid()) {
        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            return _usdAttr.ClearMetadata(MayaUsdMetadata->Lock);
        }
        PXR_NS::TfToken tok(key);
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
        }
        PXR_NS::TfToken tok(key);
        // Special cases for NodeGraphs:
        if (PXR_NS::UsdShadeNodeGraph(usdPrim())) {
            if (key == "uiname") {
                return PXR_NS::UsdShadeInput::IsInput(_usdAttr)
                    || PXR_NS::UsdShadeOutput::IsOutput(_usdAttr);
            } else {
                if (PXR_NS::UsdShadeInput::IsInput(_usdAttr)) {
                    return PXR_NS::UsdShadeInput(_usdAttr).HasSdrMetadataByKey(tok);
                } else if (PXR_NS::UsdShadeOutput::IsOutput(_usdAttr)) {
                    return PXR_NS::UsdShadeOutput(_usdAttr).HasSdrMetadataByKey(tok);
                }
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
