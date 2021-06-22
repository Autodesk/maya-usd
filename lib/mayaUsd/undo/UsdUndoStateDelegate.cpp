//
// Copyright 2020 Autodesk
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
#include "UsdUndoStateDelegate.h"

#include "UsdUndoBlock.h"
#include "UsdUndoManager.h"

#include <mayaUsd/base/debugCodes.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void copySpecAtPath(const SdfAbstractData& src, SdfAbstractData* dst, const SdfPath& path)
{
    // create a new spec at a path with the given specType
    dst->CreateSpec(path, src.GetSpecType(path));

    // get the list of tokens
    const std::vector<TfToken>& tokens = src.List(path);

    // set the value of dst at the given path and a fieldName
    for (const auto& token : tokens) {
        dst->Set(path, token, src.Get(path, token));
    }
}

// This class is used to copy specs from source SdfAbstractData contrainer
// to the destination SdfAbstractData container.
class SpecCopier : public SdfAbstractDataSpecVisitor
{
public:
    SpecCopier(SdfAbstractData* dst)
        : _dst(dst)
    {
    }

    bool VisitSpec(const SdfAbstractData& src, const SdfPath& path) override
    {
        copySpecAtPath(src, _dst, path);
        return true;
    }

    void Done(const SdfAbstractData&) override
    {
        // Do nothing
    }

    SdfAbstractData* const _dst;
};

} // namespace

namespace MAYAUSD_NS_DEF {

UsdUndoStateDelegate::UsdUndoStateDelegate()
    : _dirty(false)
    , _setMessageAlreadyShowed(false)
{
    // TfDebug::Enable(USDMAYA_UNDOSTATEDELEGATE);
}

UsdUndoStateDelegate::~UsdUndoStateDelegate() { }

UsdUndoStateDelegateRefPtr UsdUndoStateDelegate::New()
{
    return TfCreateRefPtr(new UsdUndoStateDelegate());
}

bool UsdUndoStateDelegate::_IsDirty() { return _dirty; }

void UsdUndoStateDelegate::_MarkCurrentStateAsClean() { _dirty = false; }

void UsdUndoStateDelegate::_MarkCurrentStateAsDirty() { _dirty = true; }

void UsdUndoStateDelegate::_OnSetLayer(const SdfLayerHandle& layer)
{
    if (layer) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Setting Layer '%s' \n", layer->GetDisplayName().c_str());
        _layer = layer;
    } else {
        _layer = nullptr;
    }
}

void UsdUndoStateDelegate::invertSetField(
    const SdfPath& path,
    const TfToken& fieldName,
    const VtValue& inverse)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting set Field '%s' for Spec '%s'\n", fieldName.GetText(), path.GetText());

    SetField(path, fieldName, inverse);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertCreateSpec(const SdfPath& path, bool inert)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE).Msg("Inverting creating spec at '%s'\n", path.GetText());

    DeleteSpec(path, inert);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertDeleteSpec(
    const SdfPath&       path,
    bool                 inert,
    SdfSpecType          deletedSpecType,
    const SdfDataRefPtr& deletedData)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE).Msg("Inverting deleting spec at '%s'\n", path.GetText());

    CreateSpec(path, deletedSpecType, inert);

    auto layerDataPtr = get_pointer(_GetLayerData());
    TF_AXIOM(layerDataPtr);

    // copy back every spec(s) with the given visitor
    SpecCopier specCopier(layerDataPtr);
    deletedData->VisitSpecs(&specCopier);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertMoveSpec(const SdfPath& oldPath, const SdfPath& newPath)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting move of '%s' to '%s'\n", oldPath.GetText(), newPath.GetText());

    MoveSpec(newPath, oldPath);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertPushTokenChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& value)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting push field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());

    _PopChild(parentPath, fieldName, value);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertPushPathChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& value)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting push field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());

    _PopChild(parentPath, fieldName, value);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertPopTokenChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& value)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting pop field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());

    PushChild(parentPath, fieldName, value);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertPopPathChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& value)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting pop field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());

    PushChild(parentPath, fieldName, value);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertSetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const VtValue& inverse)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg(
            "Inverting Field '%s' By Key '%s' for Spec '%s'\n",
            fieldName.GetText(),
            keyPath.GetText(),
            path.GetText());

    SetFieldDictValueByKey(path, fieldName, keyPath, inverse);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::invertSetTimeSample(
    const SdfPath& path,
    double         time,
    const VtValue& inverse)
{
    _setMessageAlreadyShowed = true;

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Inverting TimeSample '%f' for Spec '%s'\n", time, path.GetText());

    SetTimeSample(path, time, inverse);

    _setMessageAlreadyShowed = false;
}

void UsdUndoStateDelegate::_OnSetField(
    const SdfPath& path,
    const TfToken& fieldName,
    const VtValue& value)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Setting Field '%s' for Spec '%s'\n", fieldName.GetText(), path.GetText());
    }

    if (!_layer) {
        return;
    }

    const VtValue inverseValue = _layer->GetField(path, fieldName);

    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertSetField, this, path, fieldName, inverseValue));
}

void UsdUndoStateDelegate::_OnSetField(
    const SdfPath&                   path,
    const TfToken&                   fieldName,
    const SdfAbstractDataConstValue& value)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Setting Field '%s' for Spec '%s'\n", fieldName.GetText(), path.GetText());
    }

    if (!_layer) {
        return;
    }

    const VtValue inverseValue = _layer->GetField(path, fieldName);

    // add invert
    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertSetField, this, path, fieldName, inverseValue));
}

void UsdUndoStateDelegate::_OnSetFieldDictValueByKey(
    const SdfPath& path,
    const TfToken& fieldName,
    const TfToken& keyPath,
    const VtValue& value)
{
    _OnSetFieldDictValueByKeyImpl(path, fieldName, keyPath);
}

void UsdUndoStateDelegate::_OnSetFieldDictValueByKey(
    const SdfPath&                   path,
    const TfToken&                   fieldName,
    const TfToken&                   keyPath,
    const SdfAbstractDataConstValue& value)
{
    _OnSetFieldDictValueByKeyImpl(path, fieldName, keyPath);
}

void UsdUndoStateDelegate::_OnSetTimeSample(const SdfPath& path, double time, const VtValue& value)
{
    _OnSetTimeSampleImpl(path, time);
}

void UsdUndoStateDelegate::_OnSetTimeSample(
    const SdfPath&                   path,
    double                           time,
    const SdfAbstractDataConstValue& value)
{
    _OnSetTimeSampleImpl(path, time);
}

void UsdUndoStateDelegate::_OnCreateSpec(const SdfPath& path, SdfSpecType specType, bool inert)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE).Msg("Creating spec at '%s'\n", path.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertCreateSpec, this, path, inert));
}

void UsdUndoStateDelegate::_OnDeleteSpec(const SdfPath& path, bool inert)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE).Msg("Deleting spec at '%s'\n", path.GetText());
    }

    if (!_layer) {
        return;
    }

    SdfDataRefPtr deletedData = TfCreateRefPtr(new SdfData());

    // traverse the hierarchy and call copySpecAtPath on each spec
    auto layerDataPtr = std::cref(*get_pointer(_GetLayerData()));
    auto deleteDataPtr = get_pointer(deletedData);

    _GetLayer()->Traverse(
        path, [&](const SdfPath& path) { copySpecAtPath(layerDataPtr, deleteDataPtr, path); });

    const SdfSpecType deletedSpecType = _GetLayer()->GetSpecType(path);

    UsdUndoManager::instance().addInverse(std::bind(
        &UsdUndoStateDelegate::invertDeleteSpec, this, path, inert, deletedSpecType, deletedData));
}

void UsdUndoStateDelegate::_OnMoveSpec(const SdfPath& oldPath, const SdfPath& newPath)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Moving spec from '%s' to '%s'\n", oldPath.GetText(), newPath.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertMoveSpec, this, oldPath, newPath));
}

void UsdUndoStateDelegate::_OnPushChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& value)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Pushing field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertPushTokenChild, this, parentPath, fieldName, value));
}

void UsdUndoStateDelegate::_OnPushChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& value)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Pushing field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(
        std::bind(&UsdUndoStateDelegate::invertPushPathChild, this, parentPath, fieldName, value));
}

void UsdUndoStateDelegate::_OnPopChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const TfToken& oldValue)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Poping field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(std::bind(
        &UsdUndoStateDelegate::invertPopTokenChild, this, parentPath, fieldName, oldValue));
}

void UsdUndoStateDelegate::_OnPopChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const SdfPath& oldValue)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg("Poping field '%s' of '%s'\n", fieldName.GetText(), parentPath.GetText());
    }

    if (!_layer) {
        return;
    }

    UsdUndoManager::instance().addInverse(std::bind(
        &UsdUndoStateDelegate::invertPopPathChild, this, parentPath, fieldName, oldValue));
}

void UsdUndoStateDelegate::_OnSetFieldDictValueByKeyImpl(
    const SdfPath& path,
    const TfToken& fieldName,
    const TfToken& keyPath)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    if (!_setMessageAlreadyShowed) {
        TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
            .Msg(
                "Setting field '%s' by key '%s' for spec '%s'\n",
                fieldName.GetText(),
                keyPath.GetText(),
                path.GetText());
    }

    if (!_layer) {
        return;
    }

    const VtValue inverseValue = _layer->GetFieldDictValueByKey(path, fieldName, keyPath);

    UsdUndoManager::instance().addInverse(std::bind(
        &UsdUndoStateDelegate::invertSetFieldDictValueByKey,
        this,
        path,
        fieldName,
        keyPath,
        inverseValue));
}

void UsdUndoStateDelegate::_OnSetTimeSampleImpl(const SdfPath& path, double time)
{
    _MarkCurrentStateAsDirty();

    // early return if we are not inside an UsdUndoBlock
    if (UsdUndoBlock::depth() == 0) {
        return;
    }

    TF_DEBUG(USDMAYA_UNDOSTATEDELEGATE)
        .Msg("Setting time sample '%f' for spec '%s'\n", time, path.GetText());

    if (!_GetLayer()->HasField(path, SdfFieldKeys->TimeSamples)) {
        UsdUndoManager::instance().addInverse(std::bind(
            &UsdUndoStateDelegate::invertSetField,
            this,
            path,
            SdfFieldKeys->TimeSamples,
            VtValue()));

    } else {
        VtValue oldValue;

        _GetLayer()->QueryTimeSample(path, time, &oldValue);

        UsdUndoManager::instance().addInverse(
            std::bind(&UsdUndoStateDelegate::invertSetTimeSample, this, path, time, oldValue));
    }
}

// We hit a wall when running testGroupCmd with the new Undo/Redo service.
// Grouping involves two command operation (AddPrim, Parent) and during the parent::undo(), the
// parented token (newGroup1) wasn't properly removed which caused the test to fail.
//
// In the implementation SdfLayerStateDelegateBase::PopChild, the value ( a.k.a oldValue ) is
// completely ignored from the vector associate with this field and instead the last value is
// pop_back which is incorrect.
//
// https://github.com/PixarAnimationStudios/USD/blob/d8a405a1344480f859f025c4f97085143efacb53/pxr/usd/sdf/layer.cpp#L3767
//
// _PopChild is the customized version of SdfLayer::_PrimPopChild where the oldValue is properly
// removed from the container.
template <class T>
void UsdUndoStateDelegate::_PopChild(
    const SdfPath& parentPath,
    const TfToken& fieldName,
    const T&       oldValue)
{
    // capture pop child
    _OnPopChild(parentPath, fieldName, oldValue);

    // get layer data
    SdfAbstractDataPtr data = _GetLayerData();

    // See efficiency notes in _PrimPushChild().
    VtValue box = data->Get(parentPath, fieldName);
    data->Erase(parentPath, fieldName);
    if (!box.IsHolding<std::vector<T>>()) {
        TF_CODING_ERROR(
            "SdfLayer::_PrimPopChild failed: field %s is "
            "non-vector",
            fieldName.GetText());
        return;
    }
    std::vector<T> vec;
    box.Swap(vec);
    if (vec.empty()) {
        TF_CODING_ERROR("SdfLayer::_PrimPopChild failed: %s is empty", fieldName.GetText());
        return;
    }

    // find the oldValue and remove it.
    vec.erase(std::remove(vec.begin(), vec.end(), oldValue), vec.end());

    box.Swap(vec);
    data->Set(parentPath, fieldName, box);
}

} // namespace MAYAUSD_NS_DEF
