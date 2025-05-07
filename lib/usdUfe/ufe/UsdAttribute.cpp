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
#include "UsdAttribute.h"

#include <usdUfe/ufe/StagesSubject.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usdShade/utils.h>

#include <sstream>
#include <unordered_map>
#include <unordered_set>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <usdUfe/ufe/UsdShaderAttributeDef.h>
#endif
#ifdef UFE_V3_FEATURES_AVAILABLE
#include <usdUfe/base/tokens.h>
#endif

// Note: normally we would use this using directive, but here we cannot because
//       our class is called UsdAttribute which is exactly the same as the one
//       in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

static constexpr char kErrorMsgFailedConvertToString[]
    = "Could not convert the attribute '%s' to a string";
static constexpr char kErrorMsgInvalidType[]
    = "USD attribute does not match created attribute class type";

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

namespace {

template <typename T> bool setUsdAttr(UsdUfe::UsdAttribute& attr, const T& value)
{
    // USD Attribute Notification doubling problem:
    // As of 24-Nov-2019, calling Set() on a UsdAttribute causes two "info only"
    // change notifications to be sent (see StagesSubject::stageChanged).  With
    // the current USD implementation (USD 19.11), UsdAttribute::Set() ends up
    // in UsdStage::_SetValueImpl().  This function calls in sequence:
    // - UsdStage::_CreateAttributeSpecForEditing(), which has an SdfChangeBlock
    //   whose expiry causes a notification to be sent.
    // - SdfLayer::SetField(), which also has an SdfChangeBlock whose
    //   expiry causes a notification to be sent.
    // These two calls appear to be made on all calls to UsdAttribute::Set(),
    // not just on the first call.
    //
    // Trying to wrap the call to UsdAttribute::Set() inside an additional
    // SdfChangeBlock fails: no notifications are sent at all.  This is most
    // likely because of the warning given in the SdfChangeBlock documentation:
    //
    // https://graphics.pixar.com/usd/docs/api/class_sdf_change_block.html
    //
    // which stages that "it is not safe to use [...] [a] downstream API [such
    // as Usd] while a changeblock is open [...]".
    //
    // Therefore, we have implemented an attribute change block notification of
    // our own in the StagesSubject, which we invoke here, so that only a
    // single UFE attribute changed notification is generated.

    UsdUfe::InSetAttribute                    inSetAttr;
    UsdUfe::AttributeChangedNotificationGuard guard;
    const std::string                         errMsg = attr.isEditAllowedMsg();
    if (!errMsg.empty()) {
        throw std::runtime_error(errMsg);
    }

    PXR_NS::VtValue vt(value);
    return attr.UsdAttribute::set(vt, UsdTimeCode::Default());
}

PXR_NS::UsdTimeCode getCurrentTime(const Ufe::SceneItem::Ptr& item)
{
    // Attributes with time samples will fail when calling Get with default time code.
    // So we'll always use the current time when calling Get. If there are no time
    // samples, it will fall-back to the default time code.
    return UsdUfe::getTime(item->path());
}

std::string
getUsdAttributeValueAsString(const UsdUfe::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    if (!attr.isValid() ||
#ifdef UFE_V4_FEATURES_AVAILABLE
        !attr._hasValue())
#else
        !attr.hasValue())
#endif
        return attr.defaultValue();

    PXR_NS::VtValue v;
    if (attr.get(v, time)) {
        if (v.CanCast<std::string>()) {
            PXR_NS::VtValue v_str = v.Cast<std::string>();
            return v_str.Get<std::string>();
        }

        std::ostringstream os;
        os << v;
        return os.str();
    }

#ifdef UFE_V4_FEATURES_AVAILABLE
    TF_CODING_ERROR(kErrorMsgFailedConvertToString, attr._name().c_str());
#else
    TF_CODING_ERROR(kErrorMsgFailedConvertToString, attr.name().c_str());
#endif
    return std::string();
}

template <typename T, typename U>
U getUsdAttributeVectorAsUfe(const UsdUfe::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    VtValue vt;
    if (!attr.isValid() ||
#ifdef UFE_V4_FEATURES_AVAILABLE
        !attr._hasValue()) {
#else
        !attr.hasValue()) {
#endif
        if (!attr.defaultValue().empty()) {
            vt = UsdUfe::vtValueFromString(attr.usdAttributeType(), attr.defaultValue());
        } else {
            return U();
        }
    } else if (!attr.get(vt, time) || !vt.IsHolding<T>()) {
        return U();
    }
    T                gfVec = vt.UncheckedGet<T>();
    U                ret;
    constexpr size_t num = ret.vector.size();
    std::copy(gfVec.data(), gfVec.data() + num, ret.vector.data());
    return ret;
}

template <typename T, typename U>
void setUsdAttributeVectorFromUfe(
    UsdUfe::UsdAttribute&      attr,
    const U&                   value,
    const PXR_NS::UsdTimeCode& time)
{
    T vec;
    std::copy(value.vector.data(), value.vector.data() + value.vector.size(), vec.data());
    setUsdAttr<T>(attr, vec);
}

template <typename T, typename U>
U getUsdAttributeColorAsUfe(const UsdUfe::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    VtValue vt;
    if (!attr.isValid() ||
#ifdef UFE_V4_FEATURES_AVAILABLE
        !attr._hasValue()) {
#else
        !attr.hasValue()) {
#endif
        if (!attr.defaultValue().empty()) {
            vt = UsdUfe::vtValueFromString(attr.usdAttributeType(), attr.defaultValue());
        } else {
            return U();
        }
    } else if (!attr.get(vt, time) || !vt.IsHolding<T>()) {
        return U();
    }
    T gfVec = vt.UncheckedGet<T>();
    U ret;
    std::copy(gfVec.data(), gfVec.data() + ret.color.size(), ret.color.data());
    return ret;
}

template <typename T, typename U>
void setUsdAttributeColorFromUfe(
    UsdUfe::UsdAttribute&      attr,
    const U&                   value,
    const PXR_NS::UsdTimeCode& time)
{
    T vec;
    std::copy(value.color.data(), value.color.data() + value.color.size(), vec.data());
    setUsdAttr<T>(attr, vec);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
template <typename T, typename U>
U getUsdAttributeMatrixAsUfe(const UsdUfe::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    VtValue vt;
    if (!attr.isValid() || !attr._hasValue()) {
        if (!attr.defaultValue().empty()) {
            vt = UsdUfe::vtValueFromString(attr.usdAttributeType(), attr.defaultValue());
        } else {
            return U();
        }
    } else if (!attr.get(vt, time) || !vt.IsHolding<T>()) {
        return U();
    }

    T gfMat = vt.UncheckedGet<T>();
    U ret;
    std::copy(
        gfMat.data(), gfMat.data() + ret.matrix.size() * ret.matrix.size(), ret.matrix[0].data());
    return ret;
}

template <typename T, typename U>
void setUsdAttributeMatrixFromUfe(
    UsdUfe::UsdAttribute&      attr,
    const U&                   value,
    const PXR_NS::UsdTimeCode& time)
{
    T mat;
    std::copy(
        value.matrix[0].data(),
        value.matrix[0].data() + value.matrix.size() * value.matrix.size(),
        mat.data());
    setUsdAttr<T>(attr, mat);
}
#endif

template <typename T, typename A = UsdUfe::TypedUsdAttribute<T>>
class SetUndoableCommand : public UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    SetUndoableCommand(const typename A::Ptr& attr, const T& newValue)
        : _attr(attr)
        , _newValue(newValue)
    {
    }

    void undo() override
    {
        UsdUfe::InSetAttribute inSetAttr;
        UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>::undo();
    }

    void redo() override
    {
        UsdUfe::InSetAttribute inSetAttr;
        UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>::redo();
    }

protected:
    void executeImplementation() override { _attr->set(_newValue); }

private:
    const typename A::Ptr _attr;
    const T               _newValue;
};

#ifdef UFE_V3_FEATURES_AVAILABLE
class SetUndoableMetadataCommand : public UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    SetUndoableMetadataCommand(
        UsdUfe::UsdAttribute& attr,
        const std::string&    key,
        const Ufe::Value&     newValue)
        : _attr(attr)
        , _key(key)
        , _newValue(newValue)
    {
    }

    void undo() override
    {
        UsdUfe::InSetAttribute inSetAttr;
        UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>::undo();
    }

    void redo() override
    {
        UsdUfe::InSetAttribute inSetAttr;
        UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>::redo();
    }

protected:
    void executeImplementation() override
    {
#ifdef UFE_V4_FEATURES_AVAILABLE
        _attr._setMetadata(_key, _newValue);
#else
        _attr.setMetadata(_key, _newValue);
#endif
    }

private:
    UsdUfe::UsdAttribute& _attr;
    const std::string     _key;
    const Ufe::Value      _newValue;
};
#endif

} // end namespace

namespace USDUFE_NS_DEF {

//------------------------------------------------------------------------------
// UsdAttribute:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdAttribute);

UsdAttribute::UsdAttribute(UsdAttributeHolder::UPtr&& attrHolder)
    : _attrHolder(std::move(attrHolder))
{
}

std::string UsdAttribute::isEditAllowedMsg() const { return _attrHolder->isEditAllowedMsg(); }

std::string UsdAttribute::defaultValue() const { return _attrHolder->defaultValue(); }

std::string UsdAttribute::nativeType() const { return _attrHolder->nativeType(); }

bool UsdAttribute::get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const
{
    return _attrHolder->get(value, time);
}

bool UsdAttribute::set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time)
{
    return _attrHolder->set(value, time);
}

#ifdef UFE_DEFAULT_VALUE_SUPPORT
bool UsdAttribute::_isDefault() const { return _attrHolder->isDefault(); }

void UsdAttribute::_reset() { _attrHolder->reset(); }
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
bool UsdAttribute::_hasValue() const
#else
bool UsdAttribute::hasValue() const
#endif
{
    return _attrHolder->hasValue();
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdAttribute::_name() const
#else
std::string UsdAttribute::name() const
#endif
{
    return _attrHolder->name();
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdAttribute::_displayName() const
#else
std::string UsdAttribute::displayName() const
#endif
{
    return _attrHolder->displayName();
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdAttribute::_documentation() const
#else
std::string UsdAttribute::documentation() const
#endif
{
    return _attrHolder->documentation();
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdAttribute::_string(const Ufe::SceneItem::Ptr& item) const
#else
std::string UsdAttribute::string(const Ufe::SceneItem::Ptr& item) const
#endif
{
    return getUsdAttributeValueAsString(*this, getCurrentTime(item));
}

#ifdef UFE_V3_FEATURES_AVAILABLE
#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::Value UsdAttribute::_getMetadata(const std::string& key) const
#else
Ufe::Value UsdAttribute::getMetadata(const std::string& key) const
#endif
{
    return _attrHolder->getMetadata(key);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
bool UsdAttribute::_setMetadata(const std::string& key, const Ufe::Value& value)
#else
bool UsdAttribute::setMetadata(const std::string& key, const Ufe::Value& value)
#endif
{
    return _attrHolder->setMetadata(key, value);
}

Ufe::UndoableCommand::Ptr
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttribute::_setMetadataCmd(const std::string& key, const Ufe::Value& value)
#else
UsdAttribute::setMetadataCmd(const std::string& key, const Ufe::Value& value)
#endif
{
    return std::make_shared<SetUndoableMetadataCommand>(*this, key, value);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
bool UsdAttribute::_clearMetadata(const std::string& key)
#else
bool UsdAttribute::clearMetadata(const std::string& key)
#endif
{
    return _attrHolder->clearMetadata(key);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
bool UsdAttribute::_hasMetadata(const std::string& key) const
#else
bool UsdAttribute::hasMetadata(const std::string& key) const
#endif
{
    return _attrHolder->hasMetadata(key);
}
#endif

PXR_NS::SdfValueTypeName UsdAttribute::usdAttributeType() const
{
    return _attrHolder->usdAttributeType();
}

//------------------------------------------------------------------------------
// UsdAttributeGeneric:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeGeneric is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::AttributeGeneric, UsdAttributeGeneric);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeGeneric);

UsdAttributeGeneric::UsdAttributeGeneric(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeGeneric(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeGeneric::Ptr
UsdAttributeGeneric::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeGeneric>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeGeneric - Ufe::AttributeGeneric overrides
//------------------------------------------------------------------------------

std::string UsdAttributeGeneric::nativeType() const { return UsdAttribute::nativeType(); }

const std::string& UsdAttributeGeneric::nativeSdrTypeMetadata()
{
    static const auto kMetadataName = std::string { "nativeSdrType" };
    return kMetadataName;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
//------------------------------------------------------------------------------
// UsdAttributeFilename:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeFilename is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::AttributeFilename, UsdAttributeFilename);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeFilename);

UsdAttributeFilename::UsdAttributeFilename(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeFilename(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeFilename::Ptr
UsdAttributeFilename::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeFilename>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFilename - Ufe::AttributeFilename overrides
//------------------------------------------------------------------------------

std::string UsdAttributeFilename::get() const
{
    if (!hasValue())
        return std::string();

    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem()))) {
        if (vt.IsHolding<SdfAssetPath>()) {
            SdfAssetPath path = vt.UncheckedGet<SdfAssetPath>();
            return path.GetAssetPath();
        } else if (vt.IsHolding<std::string>()) {
            return vt.UncheckedGet<std::string>();
        }
    }

    return std::string();
}

void UsdAttributeFilename::set(const std::string& value)
{
    if (UsdAttribute::usdAttributeType() == SdfValueTypeNames->String) {
        setUsdAttr<std::string>(*this, value);
    } else {
        SdfAssetPath path(value);
        setUsdAttr<PXR_NS::SdfAssetPath>(*this, path);
    }
}

Ufe::UndoableCommand::Ptr UsdAttributeFilename::setCmd(const std::string& value)
{
    auto self = std::static_pointer_cast<UsdAttributeFilename>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeFilename>>(self, value);
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeEnumString:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeEnumString is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::AttributeEnumString, UsdAttributeEnumString);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeEnumString);

UsdAttributeEnumString::UsdAttributeEnumString(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeEnumString(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeEnumString::Ptr
UsdAttributeEnumString::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeEnumString>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeEnumString - Ufe::AttributeEnumString overrides
//------------------------------------------------------------------------------

std::string UsdAttributeEnumString::get() const
{
    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem()))) {
        if (vt.IsHolding<std::string>()) {
            return vt.UncheckedGet<std::string>();
        }
        if (vt.IsHolding<TfToken>()) {
            return vt.UncheckedGet<TfToken>().GetString();
        }
    }

    return std::string();
}

void UsdAttributeEnumString::set(const std::string& value)
{
    if (usdAttribute().IsValid() && usdAttribute().GetTypeName() == SdfValueTypeNames->Token) {
        setUsdAttr<PXR_NS::TfToken>(*this, TfToken(value));
    } else {
        setUsdAttr<std::string>(*this, value);
    }
}

Ufe::UndoableCommand::Ptr UsdAttributeEnumString::setCmd(const std::string& value)
{
    auto self = std::static_pointer_cast<UsdAttributeEnumString>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeEnumString>>(self, value);
}

Ufe::AttributeEnumString::EnumValues UsdAttributeEnumString::getEnumValues() const
{
    return UsdAttribute::getEnumValues();
}

//------------------------------------------------------------------------------
// UsdAttributeEnumToken:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeEnumToken is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::AttributeEnumString, UsdAttributeEnumToken);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeEnumToken);

UsdAttributeEnumToken::UsdAttributeEnumToken(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeEnumString(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeEnumToken::Ptr
UsdAttributeEnumToken::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeEnumToken>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeEnumToken - Ufe::AttributeEnumString overrides
//------------------------------------------------------------------------------

std::string UsdAttributeEnumToken::get() const
{
    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem())) && vt.IsHolding<TfToken>()) {
        return vt.UncheckedGet<TfToken>().GetString();
    }

    return std::string();
}

void UsdAttributeEnumToken::set(const std::string& value)
{
    PXR_NS::TfToken tok(value);
    setUsdAttr<PXR_NS::TfToken>(*this, tok);
}

Ufe::UndoableCommand::Ptr UsdAttributeEnumToken::setCmd(const std::string& value)
{
    auto self = std::static_pointer_cast<UsdAttributeEnumToken>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeEnumToken>>(self, value);
}

Ufe::AttributeEnumString::EnumValues UsdAttributeEnumToken::getEnumValues() const
{
    return UsdAttribute::getEnumValues();
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T>:
//------------------------------------------------------------------------------

template <typename T>
TypedUsdAttribute<T>::TypedUsdAttribute(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::TypedAttribute<T>(item)
    , UsdAttribute(std::move(attrHolder))
{
}

template <typename T> Ufe::UndoableCommand::Ptr TypedUsdAttribute<T>::setCmd(const T& value)
{
    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    // See
    // https://stackoverflow.com/questions/17853212/using-shared-from-this-in-templated-classes
    // for explanation of this->shared_from_this() in templated class.
    auto self = std::static_pointer_cast<TypedUsdAttribute<T>>(this->shared_from_this());
    return std::make_shared<SetUndoableCommand<T>>(self, value);
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T> - Ufe::TypedAttribute overrides
//------------------------------------------------------------------------------

template <> Ufe::Color3f TypedUsdAttribute<Ufe::Color3f>::get() const
{
    return getUsdAttributeColorAsUfe<GfVec3f, Ufe::Color3f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Color3f>::set(const Ufe::Color3f& value)
{
    setUsdAttributeColorFromUfe<GfVec3f, Ufe::Color3f>(*this, value, getCurrentTime(sceneItem()));
}

#ifdef UFE_V4_FEATURES_AVAILABLE
template <> Ufe::Color4f TypedUsdAttribute<Ufe::Color4f>::get() const
{
    return getUsdAttributeColorAsUfe<GfVec4f, Ufe::Color4f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Color4f>::set(const Ufe::Color4f& value)
{
    setUsdAttributeColorFromUfe<GfVec4f, Ufe::Color4f>(*this, value, getCurrentTime(sceneItem()));
}
#endif

template <> Ufe::Vector3i TypedUsdAttribute<Ufe::Vector3i>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3i, Ufe::Vector3i>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3i>::set(const Ufe::Vector3i& value)
{
    setUsdAttributeVectorFromUfe<GfVec3i, Ufe::Vector3i>(*this, value, getCurrentTime(sceneItem()));
}

#ifdef UFE_V4_FEATURES_AVAILABLE
template <> Ufe::Vector2f TypedUsdAttribute<Ufe::Vector2f>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec2f, Ufe::Vector2f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector2f>::set(const Ufe::Vector2f& value)
{
    setUsdAttributeVectorFromUfe<GfVec2f, Ufe::Vector2f>(*this, value, getCurrentTime(sceneItem()));
}
#endif

template <> Ufe::Vector3f TypedUsdAttribute<Ufe::Vector3f>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3f, Ufe::Vector3f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3f>::set(const Ufe::Vector3f& value)
{
    setUsdAttributeVectorFromUfe<GfVec3f, Ufe::Vector3f>(*this, value, getCurrentTime(sceneItem()));
}

#ifdef UFE_V4_FEATURES_AVAILABLE
template <> Ufe::Vector4f TypedUsdAttribute<Ufe::Vector4f>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec4f, Ufe::Vector4f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector4f>::set(const Ufe::Vector4f& value)
{
    setUsdAttributeVectorFromUfe<GfVec4f, Ufe::Vector4f>(*this, value, getCurrentTime(sceneItem()));
}
#endif

template <> Ufe::Vector3d TypedUsdAttribute<Ufe::Vector3d>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3d, Ufe::Vector3d>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3d>::set(const Ufe::Vector3d& value)
{
    setUsdAttributeVectorFromUfe<GfVec3d, Ufe::Vector3d>(*this, value, getCurrentTime(sceneItem()));
}

#ifdef UFE_V4_FEATURES_AVAILABLE
template <> Ufe::Matrix3d TypedUsdAttribute<Ufe::Matrix3d>::get() const
{
    return getUsdAttributeMatrixAsUfe<GfMatrix3d, Ufe::Matrix3d>(
        *this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Matrix3d>::set(const Ufe::Matrix3d& value)
{
    setUsdAttributeMatrixFromUfe<GfMatrix3d, Ufe::Matrix3d>(
        *this, value, getCurrentTime(sceneItem()));
}

template <> Ufe::Matrix4d TypedUsdAttribute<Ufe::Matrix4d>::get() const
{
    return getUsdAttributeMatrixAsUfe<GfMatrix4d, Ufe::Matrix4d>(
        *this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Matrix4d>::set(const Ufe::Matrix4d& value)
{
    setUsdAttributeMatrixFromUfe<GfMatrix4d, Ufe::Matrix4d>(
        *this, value, getCurrentTime(sceneItem()));
}
#endif

template <typename T> T TypedUsdAttribute<T>::get() const
{
    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(Ufe::Attribute::sceneItem())) && vt.IsHolding<T>()) {
        return vt.UncheckedGet<T>();
    }

    return T();
}

template <typename T> void TypedUsdAttribute<T>::set(const T& value)
{
    setUsdAttr<T>(*this, value);
}

//------------------------------------------------------------------------------
// UsdAttributeBool:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<bool>, UsdAttributeBool);

/*static*/
UsdAttributeBool::Ptr
UsdAttributeBool::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeBool>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeInt:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<int>, UsdAttributeInt);

/*static*/
UsdAttributeInt::Ptr
UsdAttributeInt::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeInt>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeUInt:
//------------------------------------------------------------------------------

#ifdef UFE_HAS_UNSIGNED_INT
USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<unsigned int>, UsdAttributeUInt);

/*static*/
UsdAttributeUInt::Ptr
UsdAttributeUInt::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeUInt>(item, std::move(attrHolder));
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeFloat:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<float>, UsdAttributeFloat);

/*static*/
UsdAttributeFloat::Ptr
UsdAttributeFloat::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeFloat>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeDouble:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<double>, UsdAttributeDouble);

/*static*/
UsdAttributeDouble::Ptr
UsdAttributeDouble::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeDouble>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeString:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeString is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::TypedAttribute<std::string>, UsdAttributeString);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeString);

UsdAttributeString::UsdAttributeString(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeString(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeString::Ptr
UsdAttributeString::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeString>(item, std::move(attrHolder));
    return attr;
}

std::string UsdAttributeString::get() const
{
    if (!hasValue())
        return std::string();

    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem())) && vt.IsHolding<std::string>()) {
        return vt.UncheckedGet<std::string>();
    }

    return std::string();
}

void UsdAttributeString::set(const std::string& value)
{
    // We need to figure out if the USDAttribute is holding a TfToken or string.
    setUsdAttr<std::string>(*this, value);
}

Ufe::UndoableCommand::Ptr UsdAttributeString::setCmd(const std::string& value)
{
    auto self = std::static_pointer_cast<UsdAttributeString>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeString>>(self, value);
}

//------------------------------------------------------------------------------
// UsdAttributeToken:
//------------------------------------------------------------------------------

// Ensure that UsdAttributeToken is properly setup.
USDUFE_VERIFY_CLASS_SETUP(Ufe::TypedAttribute<std::string>, UsdAttributeToken);
USDUFE_VERIFY_CLASS_BASE(UsdAttribute, UsdAttributeToken);

UsdAttributeToken::UsdAttributeToken(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
    : Ufe::AttributeString(item)
    , UsdAttribute(std::move(attrHolder))
{
}

/*static*/
UsdAttributeToken::Ptr
UsdAttributeToken::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeToken>(item, std::move(attrHolder));
    return attr;
}

std::string UsdAttributeToken::get() const
{
    if (!hasValue())
        return std::string();

    PXR_NS::VtValue vt;
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem())) && vt.IsHolding<TfToken>()) {
        return vt.UncheckedGet<TfToken>().GetString();
    }

    return std::string();
}

void UsdAttributeToken::set(const std::string& value)
{
    PXR_NS::TfToken tok(value);
    setUsdAttr<PXR_NS::TfToken>(*this, tok);
}

Ufe::UndoableCommand::Ptr UsdAttributeToken::setCmd(const std::string& value)
{
    auto self = std::static_pointer_cast<UsdAttributeToken>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
        displayMessage(MessageType::kError, errMsg);
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeToken>>(self, value);
}

//------------------------------------------------------------------------------
// UsdAttributeColorFloat3:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Color3f>, UsdAttributeColorFloat3);

/*static*/
UsdAttributeColorFloat3::Ptr UsdAttributeColorFloat3::create(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeColorFloat3>(item, std::move(attrHolder));
    return attr;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
//------------------------------------------------------------------------------
// UsdAttributeColorFloat4:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Color4f>, UsdAttributeColorFloat4);

/*static*/
UsdAttributeColorFloat4::Ptr UsdAttributeColorFloat4::create(
    const UsdSceneItem::Ptr&   item,
    UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeColorFloat4>(item, std::move(attrHolder));
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeInt3:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Vector3i>, UsdAttributeInt3);

/*static*/
UsdAttributeInt3::Ptr
UsdAttributeInt3::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeInt3>(item, std::move(attrHolder));
    return attr;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
//------------------------------------------------------------------------------
// UsdAttributeFloat2:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Vector2f>, UsdAttributeFloat2);

/*static*/
UsdAttributeFloat2::Ptr
UsdAttributeFloat2::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeFloat2>(item, std::move(attrHolder));
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeFloat3:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Vector3f>, UsdAttributeFloat3);

/*static*/
UsdAttributeFloat3::Ptr
UsdAttributeFloat3::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeFloat3>(item, std::move(attrHolder));
    return attr;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
//------------------------------------------------------------------------------
// UsdAttributeFloat4:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Vector4f>, UsdAttributeFloat4);

/*static*/
UsdAttributeFloat4::Ptr
UsdAttributeFloat4::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeFloat4>(item, std::move(attrHolder));
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeDouble3:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Vector3d>, UsdAttributeDouble3);

/*static*/
UsdAttributeDouble3::Ptr
UsdAttributeDouble3::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeDouble3>(item, std::move(attrHolder));
    return attr;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
//------------------------------------------------------------------------------
// UsdAttributeMatrix3d:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Matrix3d>, UsdAttributeMatrix3d);

/*static*/
UsdAttributeMatrix3d::Ptr
UsdAttributeMatrix3d::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeMatrix3d>(item, std::move(attrHolder));
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeMatrix4d:
//------------------------------------------------------------------------------

USDUFE_VERIFY_CLASS_SETUP(TypedUsdAttribute<Ufe::Matrix4d>, UsdAttributeMatrix4d);

/*static*/
UsdAttributeMatrix4d::Ptr
UsdAttributeMatrix4d::create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder)
{
    auto attr = std::make_shared<UsdAttributeMatrix4d>(item, std::move(attrHolder));
    return attr;
}

#endif

#if 0
// Note: if we were to implement generic attribute setting (via string) this
//       would be the way it could be done.
bool UsdAttribute::setValue(const std::string& value)
{
    // Put the input string into a VtValue so we can cast it to the proper type.
    PXR_NS::VtValue val(value.c_str());

    // Attempt to cast the value to what we want.  Get a default value for this
    // attribute's type name.
    PXR_NS::VtValue defVal = fAttrHandle->type().GetDefaultValue();

    // Attempt to cast the given string to the default value's type.
    // If casting fails, attempt to continue with the given value.
    PXR_NS::VtValue cast = PXR_NS::VtValue::CastToTypeOf(val, defVal);
    if (!cast.IsEmpty())
        cast.Swap(val);

    return setUsdAttr<PXR_NS::VtValue>(fAttrHandle, val);
}
#endif

} // namespace USDUFE_NS_DEF
