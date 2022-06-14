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

#include "Utils.h"
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
#include <pxr/usd/usdShade/utils.h>

#include <sstream>
#include <unordered_map>
#include <unordered_set>

// Note: normally we would use this using directive, but here we cannot because
//       our class is called UsdAttribute which is exactly the same as the one
//       in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

static constexpr char kErrorMsgFailedConvertToString[]
    = "Could not convert the attribute '%s' to a string";
static constexpr char kErrorMsgInvalidType[]
    = "USD attribute does not match created attribute class type";
#ifdef UFE_V3_FEATURES_AVAILABLE
static constexpr char kErrorMsgInvalidValueType[] = "Unexpected Ufe::Value type";
#endif

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

namespace {

template <typename T> bool setUsdAttr(MayaUsd::ufe::UsdAttribute& attr, const T& value)
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

    MayaUsd::ufe::AttributeChangedNotificationGuard guard;
    const std::string                               errMsg = attr.isEditAllowedMsg();
    if (!errMsg.empty()) {
        throw std::runtime_error(errMsg);
    }

    PXR_NS::VtValue vt(value);
    return attr.UsdAttribute::set(vt, UsdTimeCode::Default());
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

std::string getUsdAttributeValueAsString(
    const MayaUsd::ufe::UsdAttribute& attr,
    const PXR_NS::UsdTimeCode&        time)
{
    if (!attr.isValid() || !attr.hasValue())
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

    TF_CODING_ERROR(kErrorMsgFailedConvertToString, attr.name().c_str());
    return std::string();
}

template <typename T, typename U>
U getUsdAttributeVectorAsUfe(
    const MayaUsd::ufe::UsdAttribute& attr,
    const PXR_NS::UsdTimeCode&        time)
{
    VtValue vt;
    if (!attr.isValid() || !attr.hasValue()) {
        vt = MayaUsd::ufe::vtValueFromString(attr.typeName(), attr.defaultValue());
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
    MayaUsd::ufe::UsdAttribute& attr,
    const U&                    value,
    const PXR_NS::UsdTimeCode&  time)
{
    T vec;
    std::copy(value.vector.data(), value.vector.data() + value.vector.size(), vec.data());
    setUsdAttr<T>(attr, vec);
}

template <typename T, typename U>
U getUsdAttributeColorAsUfe(const MayaUsd::ufe::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time)
{
    VtValue vt;
    if (!attr.isValid() || !attr.hasValue()) {
        vt = MayaUsd::ufe::vtValueFromString(attr.typeName(), attr.defaultValue());
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
    MayaUsd::ufe::UsdAttribute& attr,
    const U&                    value,
    const PXR_NS::UsdTimeCode&  time)
{
    T vec;
    std::copy(value.color.data(), value.color.data() + value.color.size(), vec.data());
    setUsdAttr<T>(attr, vec);
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
template <typename T, typename U>
U getUsdAttributeMatrixAsUfe(
    const MayaUsd::ufe::UsdAttribute& attr,
    const PXR_NS::UsdTimeCode&        time)
{
    VtValue vt;
    if (!attr.isValid() || !attr.hasValue()) {
        vt = MayaUsd::ufe::vtValueFromString(attr.typeName(), attr.defaultValue());
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
    MayaUsd::ufe::UsdAttribute& attr,
    const U&                    value,
    const PXR_NS::UsdTimeCode&  time)
{
    T mat;
    std::copy(
        value.matrix[0].data(),
        value.matrix[0].data() + value.matrix.size() * value.matrix.size(),
        mat.data());
    setUsdAttr<T>(attr, mat);
}
#endif

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
        MayaUsd::ufe::UsdAttribute& attr,
        const std::string&          key,
        const Ufe::Value&           newValue)
        : _attr(attr)
        , _key(key)
        , _newValue(newValue)
    {
    }

    void executeUndoBlock() override { _attr.setMetadata(_key, _newValue); }

private:
    MayaUsd::ufe::UsdAttribute& _attr;
    const std::string           _key;
    const Ufe::Value            _newValue;
};
#endif

} // end namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// UsdAttribute:
//------------------------------------------------------------------------------

#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttribute::UsdAttribute(const PXR_NS::UsdPrim& prim, const Ufe::AttributeDef::ConstPtr& attrDef)
    : fPrim(prim)
    , fAttrDef(attrDef)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_VERIFY(fPrim.IsValid(), "Invalid prim '%s' passed to UsdAttribute object", fPrim.GetName().GetText());
}
#endif

UsdAttribute::UsdAttribute(const PXR_NS::UsdAttribute& usdAttr)
    : fPrim(usdAttr.GetPrim())
    , fUsdAttr(usdAttr)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_VERIFY(fPrim.IsValid(), "Invalid prim '%s' passed to UsdAttribute object", fPrim.GetName().GetText());
}

std::string UsdAttribute::isEditAllowedMsg() const
{
    if (isValid()) {
        std::string errMsg;
        isAttributeEditAllowed(fUsdAttr, &errMsg);
        return errMsg;
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else if (fAttrDef && fPrim) {
        return std::string();
    }
#endif
    else {
        return "Editing is not allowed.";
    }
}

std::string UsdAttribute::typeName() const
{
    if (isValid()) {
        return usdTypeToUfe(fUsdAttr.GetTypeName());
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else {
        return fAttrDef->type();
    }
#else
    else {
        return std::string();
    }
#endif
}

std::string UsdAttribute::defaultValue() const
{
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (fAttrDef) {
        return fAttrDef->defaultValue();
    }
#endif
    return std::string();
}

bool UsdAttribute::get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const
{
    if (isAuthored()
#ifdef UFE_V4_FEATURES_AVAILABLE
        || !fAttrDef
#endif
    ) {
        return fUsdAttr.Get(&value, time);
    } else {
#ifdef UFE_V4_FEATURES_AVAILABLE
        // No prim check is required as we can get the value from the attribute definition
        if (fAttrDef) {
            const std::string& defaultValue = fAttrDef->defaultValue();
            const std::string& typeName = UsdAttribute::typeName();
            value = vtValueFromString(typeName, defaultValue);
            return !value.IsEmpty();
        }
#endif
        return false;
    }
}

bool UsdAttribute::set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time)
{
    if (!isValid()) {
        PXR_NS::VtValue currentValue;
        get(currentValue, time);
        if (currentValue == value) {
            return true;
        } else {
#ifdef UFE_V4_FEATURES_AVAILABLE
            if (fAttrDef && fPrim) {
                UsdShadeShader shader(fPrim);
                if (fAttrDef->ioType() == Ufe::AttributeDef::OUTPUT_ATTR) {
                    fUsdAttr = shader
                                   .CreateOutput(PXR_NS::TfToken(
                                       fAttrDef->name(), ufeTypeToUsd(fAttrDef->type())))
                                   .GetAttr();
                } else {
                    fUsdAttr = shader
                                   .CreateInput(PXR_NS::TfToken(
                                       fAttrDef->name(), ufeTypeToUsd(fAttrDef->type())))
                                   .GetAttr();
                }
            } else {
#endif
                return false;
#ifdef UFE_V4_FEATURES_AVAILABLE
            }
#endif
        }
    }

    return fUsdAttr.Set(value, time);
}

bool UsdAttribute::hasValue() const
{
    return isValid() ? fUsdAttr.HasValue() :
#ifdef UFE_V4_FEATURES_AVAILABLE
                     !fAttrDef->defaultValue().empty();
#else
                     false;
#endif
}

std::string UsdAttribute::name() const
{
    if (isValid()) {
        return fUsdAttr.GetName().GetString();
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else if (fAttrDef) {
        return PXR_NS::UsdShadeUtils::GetFullName(
            PXR_NS::TfToken(fAttrDef->name()),
            fAttrDef->ioType() == Ufe::AttributeDef::OUTPUT_ATTR
                ? PXR_NS::UsdShadeAttributeType::Output
                : PXR_NS::UsdShadeAttributeType::Input);
    }
#endif
    else {
        return std::string();
    }
}

std::string UsdAttribute::documentation() const
{
    if (isValid()) {
        return fUsdAttr.GetDocumentation();
    } else {
        return std::string();
    }
}

std::string UsdAttribute::string(const Ufe::SceneItem::Ptr& item) const
{
    return getUsdAttributeValueAsString(*this, getCurrentTime(item));
}

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::Value UsdAttribute::getMetadata(const std::string& key) const
{
    if (isValid()) {
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
            else {
                std::stringstream ss;
                ss << v;
                return Ufe::Value(ss.str());
            }
        }
        return Ufe::Value();
#ifdef UFE_V4_FEATURES_AVAILABLE
    } else if (fAttrDef && fAttrDef->hasMetadata(key)) {
        return fAttrDef->getMetadata(key);
#endif
    } else {
        return Ufe::Value();
    }
}

bool UsdAttribute::setMetadata(const std::string& key, const Ufe::Value& value)
{
    if (isValid())
        return setUsdAttrMetadata(fUsdAttr, key, value);
    else {
#ifdef UFE_V4_FEATURES_AVAILABLE
        if (fAttrDef && fPrim) {
            UsdShadeShader shader(fPrim);
            if (fAttrDef->ioType() == Ufe::AttributeDef::OUTPUT_ATTR) {
                fUsdAttr = shader
                               .CreateOutput(PXR_NS::TfToken(
                                   fAttrDef->name(), ufeTypeToUsd(fAttrDef->type())))
                               .GetAttr();
            } else {
                fUsdAttr = shader
                               .CreateInput(PXR_NS::TfToken(
                                   fAttrDef->name(), ufeTypeToUsd(fAttrDef->type())))
                               .GetAttr();
            }
            const PXR_NS::TfToken attrName(PXR_NS::UsdShadeUtils::GetFullName(
                PXR_NS::TfToken(fAttrDef->name()),
                fAttrDef->ioType() == Ufe::AttributeDef::OUTPUT_ATTR
                    ? PXR_NS::UsdShadeAttributeType::Output
                    : PXR_NS::UsdShadeAttributeType::Input));
            return setUsdAttrMetadata(fUsdAttr, key, value);
        } else {
#endif
            return false;
#ifdef UFE_V4_FEATURES_AVAILABLE
        }
#endif
    }
}

Ufe::UndoableCommand::Ptr
UsdAttribute::setMetadataCmd(const std::string& key, const Ufe::Value& value)
{
    return std::make_shared<SetUndoableMetadataCommand>(*this, key, value);
}

bool UsdAttribute::clearMetadata(const std::string& key)
{
    if (isValid()) {
        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            return fUsdAttr.ClearMetadata(MayaUsdMetadata->Lock);
        }
        PXR_NS::TfToken tok(key);
        return fUsdAttr.ClearMetadata(tok);
    } else {
        return true;
    }
}

bool UsdAttribute::hasMetadata(const std::string& key) const
{
    bool result = false;
    if (isValid()) {
        // Special cases for known Ufe metadata keys.
        if (key == Ufe::Attribute::kLocked) {
            result = fUsdAttr.HasMetadata(MayaUsdMetadata->Lock);
            if (result) {
                return true;
            }
        }
        PXR_NS::TfToken tok(key);
        result = fUsdAttr.HasMetadata(tok);
        if (result) {
            return true;
        }
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (fAttrDef && fAttrDef->hasMetadata(key)) {
        return true;
    }
#endif
    return false;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeGeneric:
//------------------------------------------------------------------------------

#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeGeneric::UsdAttributeGeneric(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
    : Ufe::AttributeGeneric(item)
    , UsdAttribute(prim, attrDef)
{
}
#endif

UsdAttributeGeneric::UsdAttributeGeneric(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::AttributeGeneric(item)
    , UsdAttribute(usdAttr)
{
}

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeGeneric::Ptr UsdAttributeGeneric::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeGeneric>(item, prim, attrDef);
    return attr;
}
#endif

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
    if (isValid()) {
        return fUsdAttr.GetTypeName().GetType().GetTypeName();
    } else {
#ifdef UFE_V4_FEATURES_AVAILABLE
        return ufeTypeToUsd(fAttrDef->type()).GetType().GetTypeName();
#else
        return ufeTypeToUsd(SdfValueTypeName().GetAsToken().GetString()).GetType().GetTypeName();
#endif
    }
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
//------------------------------------------------------------------------------
// UsdAttributeFilename:
//------------------------------------------------------------------------------

#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFilename::UsdAttributeFilename(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
    : Ufe::AttributeFilename(item)
    , UsdAttribute(prim, attrDef)
{
}
#endif

UsdAttributeFilename::UsdAttributeFilename(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::AttributeFilename(item)
    , UsdAttribute(usdAttr)
{
}

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFilename::Ptr UsdAttributeFilename::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeFilename>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeFilename::Ptr
UsdAttributeFilename::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFilename>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFilename - Ufe::AttributeFilename overrides
//------------------------------------------------------------------------------

std::string UsdAttributeFilename::get() const
{
    PXR_NS::VtValue vt;
    if (fUsdAttr.Get(&vt, getCurrentTime(sceneItem())) && vt.IsHolding<SdfAssetPath>()) {
        SdfAssetPath path = vt.UncheckedGet<SdfAssetPath>();
        return path.GetAssetPath();
    }

    return std::string();
}

void UsdAttributeFilename::set(const std::string& value)
{
    SdfAssetPath path(value);
    setUsdAttr<PXR_NS::SdfAssetPath>(*this, path);
}

Ufe::UndoableCommand::Ptr UsdAttributeFilename::setCmd(const std::string& value)
{
    auto self = std::dynamic_pointer_cast<UsdAttributeFilename>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    std::string errMsg;
    if (!MayaUsd::ufe::isAttributeEditAllowed(fUsdAttr, &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return nullptr;
    }

    return std::make_shared<SetUndoableCommand<std::string, UsdAttributeFilename>>(self, value);
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeEnumString:
//------------------------------------------------------------------------------

#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeEnumString::UsdAttributeEnumString(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
    : Ufe::AttributeEnumString(item)
    , UsdAttribute(prim, attrDef)
{
}
#endif

UsdAttributeEnumString::UsdAttributeEnumString(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::AttributeEnumString(item)
    , UsdAttribute(usdAttr)
{
}

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeEnumString::Ptr UsdAttributeEnumString::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeEnumString>(item, prim, attrDef);
    return attr;
}
#endif

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
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem())) && vt.IsHolding<TfToken>()) {
        TfToken tok = vt.UncheckedGet<TfToken>();
        return tok.GetString();
    }

    return std::string();
}

void UsdAttributeEnumString::set(const std::string& value)
{
    PXR_NS::TfToken tok(value);
    setUsdAttr<PXR_NS::TfToken>(*this, tok);
}

Ufe::UndoableCommand::Ptr UsdAttributeEnumString::setCmd(const std::string& value)
{
    auto self = std::dynamic_pointer_cast<UsdAttributeEnumString>(shared_from_this());
    if (!TF_VERIFY(self, kErrorMsgInvalidType))
        return nullptr;

    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
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

#ifdef UFE_V4_FEATURES_AVAILABLE
template <typename T>
TypedUsdAttribute<T>::TypedUsdAttribute(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
    : Ufe::TypedAttribute<T>(item)
    , UsdAttribute(prim, attrDef)
{
}
#endif

template <typename T>
TypedUsdAttribute<T>::TypedUsdAttribute(
    const UsdSceneItem::Ptr&    item,
    const PXR_NS::UsdAttribute& usdAttr)
    : Ufe::TypedAttribute<T>(item)
    , UsdAttribute(usdAttr)
{
}

template <typename T> Ufe::UndoableCommand::Ptr TypedUsdAttribute<T>::setCmd(const T& value)
{
    const std::string errMsg = isEditAllowedMsg();
    if (!errMsg.empty()) {
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
    if (UsdAttribute::get(vt, getCurrentTime(sceneItem()))) {
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
    const std::string typeName = UsdAttribute::typeName();
    if (typeName == Ufe::Attribute::kString) {
        setUsdAttr<std::string>(*this, value);
        return;
    } else if (typeName == Ufe::Attribute::kEnumString) {
        PXR_NS::TfToken tok(value);
        setUsdAttr<PXR_NS::TfToken>(*this, tok);
        return;
    }

    // If we get here it means the USDAttribute type wasn't TfToken or string.
    TF_CODING_ERROR(kErrorMsgInvalidType);
}

template <> Ufe::Color3f TypedUsdAttribute<Ufe::Color3f>::get() const
{
    return getUsdAttributeColorAsUfe<GfVec3f, Ufe::Color3f>(*this, getCurrentTime(sceneItem()));
}

template <> void TypedUsdAttribute<Ufe::Color3f>::set(const Ufe::Color3f& value)
{
    setUsdAttributeColorFromUfe<GfVec3f, Ufe::Color3f>(*this, value, getCurrentTime(sceneItem()));
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
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

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
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

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
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

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
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

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeBool::Ptr UsdAttributeBool::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeBool>(item, prim, attrDef);
    return attr;
}
#endif

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
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeInt::Ptr UsdAttributeInt::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeInt>(item, prim, attrDef);
    return attr;
}
#endif

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
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFloat::Ptr UsdAttributeFloat::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeFloat>(item, prim, attrDef);
    return attr;
}
#endif

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
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeDouble::Ptr UsdAttributeDouble::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeDouble>(item, prim, attrDef);
    return attr;
}
#endif

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
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeString::Ptr UsdAttributeString::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeString>(item, prim, attrDef);
    return attr;
}
#endif

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
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeColorFloat3::Ptr UsdAttributeColorFloat3::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeColorFloat3>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeColorFloat3::Ptr
UsdAttributeColorFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeColorFloat3>(item, usdAttr);
    return attr;
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
//------------------------------------------------------------------------------
// UsdAttributeColorFloat4:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeColorFloat4::Ptr UsdAttributeColorFloat4::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeColorFloat4>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeColorFloat4::Ptr
UsdAttributeColorFloat4::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeColorFloat4>(item, usdAttr);
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeInt3:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeInt3::Ptr UsdAttributeInt3::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeInt3>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeInt3::Ptr
UsdAttributeInt3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeInt3>(item, usdAttr);
    return attr;
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
//------------------------------------------------------------------------------
// UsdAttributeFloat2:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFloat2::Ptr UsdAttributeFloat2::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeFloat2>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeFloat2::Ptr
UsdAttributeFloat2::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFloat2>(item, usdAttr);
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeFloat3:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFloat3::Ptr UsdAttributeFloat3::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeFloat3>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeFloat3::Ptr
UsdAttributeFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFloat3>(item, usdAttr);
    return attr;
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
//------------------------------------------------------------------------------
// UsdAttributeFloat4:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeFloat4::Ptr UsdAttributeFloat4::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeFloat4>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeFloat4::Ptr
UsdAttributeFloat4::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeFloat4>(item, usdAttr);
    return attr;
}
#endif

//------------------------------------------------------------------------------
// UsdAttributeDouble3:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeDouble3::Ptr UsdAttributeDouble3::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeDouble3>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeDouble3::Ptr
UsdAttributeDouble3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeDouble3>(item, usdAttr);
    return attr;
}

#if (UFE_PREVIEW_VERSION_NUM >= 4015)
//------------------------------------------------------------------------------
// UsdAttributeMatrix3d:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeMatrix3d::Ptr UsdAttributeMatrix3d::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeMatrix3d>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeMatrix3d::Ptr
UsdAttributeMatrix3d::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeMatrix3d>(item, usdAttr);
    return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeMatrix4d:
//------------------------------------------------------------------------------

/*static*/
#ifdef UFE_V4_FEATURES_AVAILABLE
UsdAttributeMatrix4d::Ptr UsdAttributeMatrix4d::create(
    const UsdSceneItem::Ptr&           item,
    const PXR_NS::UsdPrim&             prim,
    const Ufe::AttributeDef::ConstPtr& attrDef)
{
    auto attr = std::make_shared<UsdAttributeMatrix4d>(item, prim, attrDef);
    return attr;
}
#endif

/*static*/
UsdAttributeMatrix4d::Ptr
UsdAttributeMatrix4d::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
    auto attr = std::make_shared<UsdAttributeMatrix4d>(item, usdAttr);
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
    PXR_NS::VtValue defVal = fAttrHandle->typeName().GetDefaultValue();

    // Attempt to cast the given string to the default value's type.
    // If casting fails, attempt to continue with the given value.
    PXR_NS::VtValue cast = PXR_NS::VtValue::CastToTypeOf(val, defVal);
    if (!cast.IsEmpty())
        cast.Swap(val);

    return setUsdAttr<PXR_NS::VtValue>(fAttrHandle, val);
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
