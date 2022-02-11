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

#include "private/Utils.h"

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/base/tokens.h>
#endif
#include <mayaUsd/ufe/StagesSubject.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>

#include <sstream>
#include <unordered_map>
#include <unordered_set>

// Note: normally we would use this using directive, but here we cannot because
//       our class is called UsdAttribute which is exactly the same as the one
//       in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

static constexpr char kErrorMsgFailedConvertToString[]
    = "Could not convert the attribute to a string";
static constexpr char kErrorMsgInvalidType[]
    = "USD attribute does not match created attribute class type";
#ifdef UFE_V3_FEATURES_AVAILABLE
static constexpr char kErrorMsgInvalidValueType[] = "Unexpected Ufe::Value type";
#endif

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

namespace {

template <typename T> bool setUsdAttr(const PXR_NS::UsdAttribute& attr, const T& value)
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
    if (!attr.IsValid())
        return false;

    MayaUsd::ufe::AttributeChangedNotificationGuard guard;
    std::string                                     errMsg;
    bool isSetAttrAllowed = MayaUsd::ufe::isAttributeEditAllowed(attr, &errMsg);
    if (!isSetAttrAllowed) {
        throw std::runtime_error(errMsg);
    }

    return attr.Set<T>(value);
}

#ifdef UFE_V3_FEATURES_AVAILABLE
bool setUsdAttrMetadata(
    const PXR_NS::UsdAttribute& attr,
    const std::string&          key,
    const Ufe::Value&           value)
{
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

PXR_NS::UsdTimeCode getCurrentTime(const Ufe::SceneItem::Ptr& item)
{
    // Attributes with time samples will fail when calling Get with default time code.
    // So we'll always use the current time when calling Get. If there are no time
    // samples, it will fall-back to the default time code.
    return MayaUsd::ufe::getTime(item->path());
}

std::string
getUsdAttributeValueAsString(const PXR_NS::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    if (!attr.IsValid() || !attr.HasValue())
        return std::string();

    PXR_NS::VtValue v;
    if (attr.Get(&v, time)) {
        if (v.CanCast<std::string>()) {
            PXR_NS::VtValue v_str = v.Cast<std::string>();
            return v_str.Get<std::string>();
        }

        std::ostringstream os;
        os << v;
        return os.str();
    }

    TF_CODING_ERROR(kErrorMsgFailedConvertToString);
    return std::string();
}

template <typename T, typename U>
U getUsdAttributeVectorAsUfe(const PXR_NS::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    if (!attr.IsValid() || !attr.HasValue())
        return U();

    PXR_NS::VtValue vt;
    if (attr.Get(&vt, time) && vt.IsHolding<T>()) {
        T gfVec = vt.UncheckedGet<T>();
        U ret(gfVec[0], gfVec[1], gfVec[2]);
        return ret;
    }

    return U();
}

template <typename T, typename U>
void setUsdAttributeVectorFromUfe(
    PXR_NS::UsdAttribute&      attr,
    const U&                   value,
    const PXR_NS::UsdTimeCode& time)
{
    T vec;
    vec.Set(value.x(), value.y(), value.z());
    setUsdAttr<T>(attr, vec);
}

class UsdUndoableCommand : public Ufe::UndoableCommand
{
public:
    void execute() override
    {
        MayaUsd::UsdUndoBlock undoBlock(&_undoableItem);
        executeUndoBlock();
    }

    void undo() override { _undoableItem.undo(); }
    void redo() override { _undoableItem.redo(); }

protected:
    // Actual implementation of the execution of the command,
    // executed "within" a UsdUndoBlock to capture undo data,
    // to be implemented by the sub-class.
    virtual void executeUndoBlock() = 0;

private:
    MayaUsd::UsdUndoableItem _undoableItem;
};

template <typename T, typename A = MayaUsd::ufe::TypedUsdAttribute<T>>
class SetUndoableCommand : public UsdUndoableCommand
{
public:
    SetUndoableCommand(const typename A::Ptr& attr, const T& newValue)
        : _attr(attr)
        , _newValue(newValue)
    {
    }

    void executeUndoBlock() override { _attr->set(_newValue); }

private:
    const typename A::Ptr _attr;
    const T               _newValue;
};

#ifdef UFE_V3_FEATURES_AVAILABLE
class SetUndoableMetadataCommand : public UsdUndoableCommand
{
public:
    SetUndoableMetadataCommand(
        const PXR_NS::UsdAttribute& usdAttr,
        const std::string&          key,
        const Ufe::Value&           newValue)
        : _usdAttr(usdAttr)
        , _key(key)
        , _newValue(newValue)
    {
    }

    void executeUndoBlock() override { setUsdAttrMetadata(_usdAttr, _key, _newValue); }

private:
    const PXR_NS::UsdAttribute _usdAttr;
    const std::string          _key;
    const Ufe::Value           _newValue;
};
#endif

} // end namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// UsdAttribute:
//------------------------------------------------------------------------------

UsdAttribute::UsdAttribute(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
    : fUsdAttr(usdAttr)
{
    fPrim = item->prim();
}

UsdAttribute::~UsdAttribute() { }

bool UsdAttribute::hasValue() const { return fUsdAttr.HasValue(); }

std::string UsdAttribute::name() const
{
    // Should be the same as the name we were created with.
    return fUsdAttr.GetName().GetString();
}

std::string UsdAttribute::documentation() const { return fUsdAttr.GetDocumentation(); }

std::string UsdAttribute::string(const Ufe::SceneItem::Ptr& item) const
{
    return getUsdAttributeValueAsString(fUsdAttr, getCurrentTime(item));
}

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::Value UsdAttribute::getMetadata(const std::string& key) const
{
    // Special cases for known Ufe metadata keys.
    if (key == Ufe::Attribute::kLocked) {
        PXR_NS::TfToken lock;
        bool            ret = fUsdAttr.GetMetadata(MayaUsdMetadata->Lock, &lock);
        if (ret)
            return Ufe::Value((lock == MayaUsdTokens->On) ? true : false);
        return Ufe::Value();
    }

    PXR_NS::TfToken tok(key);
    PXR_NS::VtValue v;
    if (fUsdAttr.GetMetadata(tok, &v)) {
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
    }
    return Ufe::Value();
}

bool UsdAttribute::setMetadata(const std::string& key, const Ufe::Value& value)
{
    return setUsdAttrMetadata(fUsdAttr, key, value);
}

Ufe::UndoableCommand::Ptr
UsdAttribute::setMetadataCmd(const std::string& key, const Ufe::Value& value)
{
    return std::make_shared<SetUndoableMetadataCommand>(fUsdAttr, key, value);
}

bool UsdAttribute::clearMetadata(const std::string& key)
{
    // Special cases for known Ufe metadata keys.
    if (key == Ufe::Attribute::kLocked) {
        return fUsdAttr.ClearMetadata(MayaUsdMetadata->Lock);
    }

    PXR_NS::TfToken tok(key);
    return fUsdAttr.ClearMetadata(tok);
}

bool UsdAttribute::hasMetadata(const std::string& key) const
{
    // Special cases for known Ufe metadata keys.
    if (key == Ufe::Attribute::kLocked) {
        return fUsdAttr.HasMetadata(MayaUsdMetadata->Lock);
    }

    PXR_NS::TfToken tok(key);
    return fUsdAttr.HasMetadata(tok);
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeGeneric:
//------------------------------------------------------------------------------

UsdAttributeGeneric::UsdAttributeGeneric(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::AttributeGeneric(item)
    , UsdAttribute(item, usdAttr)
{
}

/*static*/
UsdAttributeGeneric::Ptr
UsdAttributeGeneric::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeGeneric>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeGeneric - Ufe::AttributeGeneric overrides
//------------------------------------------------------------------------------

std::string UsdAttributeGeneric::nativeType() const
{
    return fUsdAttr.GetTypeName().GetType().GetTypeName();
}

//------------------------------------------------------------------------------
// UsdAttributeEnumString:
//------------------------------------------------------------------------------

UsdAttributeEnumString::UsdAttributeEnumString(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::AttributeEnumString(item)
    , UsdAttribute(item, usdAttr)
{
}

/*static*/
UsdAttributeEnumString::Ptr
UsdAttributeEnumString::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeEnumString>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeEnumString - Ufe::AttributeEnumString overrides
//------------------------------------------------------------------------------

std::string UsdAttributeEnumString::get() const
{
    PXR_NS::VtValue vt;
    if (fUsdAttr.Get(&vt, getCurrentTime(sceneItem())) && vt.IsHolding<TfToken>()) {
        TfToken tok = vt.UncheckedGet<TfToken>();
        return tok.GetString();
    }

    return std::string();
}

void UsdAttributeEnumString::set(const std::string& value)
{
    PXR_NS::TfToken tok(value);
    setUsdAttr<PXR_NS::TfToken>(fUsdAttr, tok);
}

Ufe::UndoableCommand::Ptr UsdAttributeEnumString::setCmd(const std::string& value)
{
    auto self = std::dynamic_pointer_cast<UsdAttributeEnumString>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    std::string errMsg;
    if (!MayaUsd::ufe::isAttributeEditAllowed(fUsdAttr, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeEnumString>>(self, value);
}

Ufe::AttributeEnumString::EnumValues UsdAttributeEnumString::getEnumValues() const
{
    PXR_NS::TfToken tk(name());
    auto            attrDefn = fPrim.GetPrimDefinition().GetSchemaAttributeSpec(tk);
    if (attrDefn && attrDefn->HasAllowedTokens()) {
        auto                         tokenArray = attrDefn->GetAllowedTokens();
        std::vector<PXR_NS::TfToken> tokenVec(tokenArray.begin(), tokenArray.end());
        EnumValues                   tokens = PXR_NS::TfToStringVector(tokenVec);
        return tokens;
    }

    return EnumValues();
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T>:
//------------------------------------------------------------------------------

template <typename T>
TypedUsdAttribute<T>::TypedUsdAttribute(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::TypedAttribute<T>(item)
    , UsdAttribute(item, usdAttr)
{
}

template <typename T> Ufe::UndoableCommand::Ptr TypedUsdAttribute<T>::setCmd(const T& value)
{
    std::string errMsg;
    if (!MayaUsd::ufe::isAttributeEditAllowed(fUsdAttr, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return nullptr;
    }

    // See
    // https://stackoverflow.com/questions/17853212/using-shared-from-this-in-templated-classes
    // for explanation of this->shared_from_this() in templated class.
    auto self = std::dynamic_pointer_cast<TypedUsdAttribute<T>>(this->shared_from_this());
    return std::make_shared<SetUndoableCommand<T>>(self, value);
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T> - Ufe::TypedAttribute overrides
//------------------------------------------------------------------------------

template <> std::string TypedUsdAttribute<std::string>::get() const
{
    if (!hasValue())
        return std::string();

    PXR_NS::VtValue vt;
    if (fUsdAttr.Get(&vt, getCurrentTime(sceneItem()))) {
        // The USDAttribute can be holding either TfToken or string.
        if (vt.IsHolding<TfToken>()) {
            TfToken tok = vt.UncheckedGet<TfToken>();
            return tok.GetString();
        } else if (vt.IsHolding<std::string>()) {
            return vt.UncheckedGet<std::string>();
        }
    }

    return std::string();
}

template <> void TypedUsdAttribute<std::string>::set(const std::string& value)
{
    // We need to figure out if the USDAttribute is holding a TfToken or string.
    const PXR_NS::SdfValueTypeName typeName = fUsdAttr.GetTypeName();
    if (typeName.GetHash() == SdfValueTypeNames->String.GetHash()) {
        setUsdAttr<std::string>(fUsdAttr, value);
        return;
    } else if (typeName.GetHash() == SdfValueTypeNames->Token.GetHash()) {
        PXR_NS::TfToken tok(value);
        setUsdAttr<PXR_NS::TfToken>(fUsdAttr, tok);
        return;
    }

    // If we get here it means the USDAttribute type wasn't TfToken or string.
    TF_CODING_ERROR(kErrorMsgInvalidType);
}

template <> Ufe::Color3f TypedUsdAttribute<Ufe::Color3f>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3f, Ufe::Color3f>(fUsdAttr, getCurrentTime(sceneItem()));
}

// Note: cannot use setUsdAttributeVectorFromUfe since it relies on x/y/z
template <> void TypedUsdAttribute<Ufe::Color3f>::set(const Ufe::Color3f& value)
{
    GfVec3f vec;
    vec.Set(value.r(), value.g(), value.b());
    setUsdAttr<GfVec3f>(fUsdAttr, vec);
}

template <> Ufe::Vector3i TypedUsdAttribute<Ufe::Vector3i>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3i, Ufe::Vector3i>(
        fUsdAttr, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3i>::set(const Ufe::Vector3i& value)
{
    setUsdAttributeVectorFromUfe<GfVec3i, Ufe::Vector3i>(
        fUsdAttr, value, getCurrentTime(sceneItem()));
}

template <> Ufe::Vector3f TypedUsdAttribute<Ufe::Vector3f>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3f, Ufe::Vector3f>(
        fUsdAttr, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3f>::set(const Ufe::Vector3f& value)
{
    setUsdAttributeVectorFromUfe<GfVec3f, Ufe::Vector3f>(
        fUsdAttr, value, getCurrentTime(sceneItem()));
}

template <> Ufe::Vector3d TypedUsdAttribute<Ufe::Vector3d>::get() const
{
    return getUsdAttributeVectorAsUfe<GfVec3d, Ufe::Vector3d>(
        fUsdAttr, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Vector3d>::set(const Ufe::Vector3d& value)
{
    setUsdAttributeVectorFromUfe<GfVec3d, Ufe::Vector3d>(
        fUsdAttr, value, getCurrentTime(sceneItem()));
}

template <typename T> T TypedUsdAttribute<T>::get() const
{
    if (!hasValue())
        return T();

    PXR_NS::VtValue vt;
    if (fUsdAttr.Get(&vt, getCurrentTime(Ufe::Attribute::sceneItem())) && vt.IsHolding<T>()) {
        return vt.UncheckedGet<T>();
    }

    return T();
}

template <typename T> void TypedUsdAttribute<T>::set(const T& value)
{
    setUsdAttr<T>(fUsdAttr, value);
}

//------------------------------------------------------------------------------
// UsdAttributeBool:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeBool::Ptr
UsdAttributeBool::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeBool>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeInt:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeInt::Ptr
UsdAttributeInt::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeInt>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFloat:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeFloat::Ptr
UsdAttributeFloat::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFloat>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeDouble:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeDouble::Ptr
UsdAttributeDouble::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeDouble>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeString:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeString::Ptr
UsdAttributeString::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeString>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeColorFloat3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeColorFloat3::Ptr
UsdAttributeColorFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeColorFloat3>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeInt3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeInt3::Ptr
UsdAttributeInt3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeInt3>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFloat3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeFloat3::Ptr
UsdAttributeFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFloat3>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeDouble3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeDouble3::Ptr
UsdAttributeDouble3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeDouble3>(item, usdAttr);
    return attr;
}

#if 0
// Note: if we were to implement generic attribute setting (via string) this
//       would be the way it could be done.
bool UsdAttribute::setValue(const std::string& value)
{
    // Put the input string into a VtValue so we can cast it to the proper type.
    PXR_NS::VtValue val(value.c_str());

    // Attempt to cast the value to what we want.  Get a default value for this
    // attribute's type name.
    PXR_NS::VtValue defVal = fUsdAttr.GetTypeName().GetDefaultValue();

    // Attempt to cast the given string to the default value's type.
    // If casting fails, attempt to continue with the given value.
    PXR_NS::VtValue cast = PXR_NS::VtValue::CastToTypeOf(val, defVal);
    if (!cast.IsEmpty())
        cast.Swap(val);

    return setUsdAttr<PXR_NS::VtValue>(fUsdAttr, val);
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
