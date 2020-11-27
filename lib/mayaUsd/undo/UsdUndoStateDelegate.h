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

#ifndef MAYAUSD_UNDO_UNDOSTATE_DELEGATE_H
#define MAYAUSD_UNDO_UNDOSTATE_DELEGATE_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/layerStateDelegate.h>

// convenient way to bring in other headers 
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUndoStateDelegate);

//! \brief State delegate that records whether any changes have been made to a layer.
/*!
    The state delegate is invoked on every authoring operation on a layer.

    There exist exactly one invert function for every authoring operation. These invert functions are collected
    by UsdUndoManager::addInverse() call which then will be transfered to an UsdUndoableItem object when UsdUndoBlock expires.
*/
class MAYAUSD_CORE_PUBLIC UsdUndoStateDelegate : public SdfLayerStateDelegateBase
{
public:
    UsdUndoStateDelegate();
    ~UsdUndoStateDelegate() override;

    static UsdUndoStateDelegateRefPtr New();

private:
    void invertSetField(const SdfPath& path, const TfToken& fieldName, const VtValue& inverse);
    void invertCreateSpec(const SdfPath& path, bool inert);
    void invertDeleteSpec(const SdfPath& path, bool inert, SdfSpecType deletedSpecType, const SdfDataRefPtr& deletedData);
    void invertMoveSpec(const SdfPath& oldPath, const SdfPath& newPath);
    void invertPushTokenChild(const SdfPath& parentPath, const TfToken& fieldName, const TfToken& value);
    void invertPopTokenChild(const SdfPath& parentPath, const TfToken& fieldName, const TfToken& value);
    void invertPushPathChild(const SdfPath& parentPath, const TfToken& fieldName, const SdfPath& value);
    void invertPopPathChild(const SdfPath& parentPath, const TfToken& fieldName, const SdfPath& value);
    void invertSetFieldDictValueByKey(const SdfPath& path, const TfToken& fieldName, const TfToken& keyPath, const VtValue& inverse);
    void invertSetTimeSample(const SdfPath& path, double time, const VtValue& inverse);

protected:
    bool _IsDirty() override;
    void _MarkCurrentStateAsClean() override;
    void _MarkCurrentStateAsDirty() override;

    void _OnSetLayer(const SdfLayerHandle& layer) override;

    void _OnSetField(const SdfPath& path, const TfToken& fieldName, const VtValue& value) override;
    void _OnSetField(const SdfPath& path, const TfToken& fieldName, const SdfAbstractDataConstValue& value) override;

    void _OnSetFieldDictValueByKey(
        const SdfPath& path,
        const TfToken& fieldName,
        const TfToken& keyPath,
        const VtValue& value) override;
    void _OnSetFieldDictValueByKey(
        const SdfPath&                   path,
        const TfToken&                   fieldName,
        const TfToken&                   keyPath,
        const SdfAbstractDataConstValue& value) override;

    void _OnSetTimeSample(const SdfPath& path, double time, const VtValue& value) override;
    void _OnSetTimeSample(const SdfPath& path, double time, const SdfAbstractDataConstValue& value) override;

    void _OnCreateSpec(const SdfPath& path, SdfSpecType specType, bool inert) override;

    void _OnDeleteSpec(const SdfPath& path, bool inert) override;
    void _OnMoveSpec(const SdfPath& oldPath, const SdfPath& newPath) override;

    void _OnPushChild(const SdfPath& parentPath, const TfToken& fieldName, const TfToken& value) override;
    void _OnPushChild(const SdfPath& parentPath, const TfToken& fieldName, const SdfPath& value) override;
    void _OnPopChild(const SdfPath& parentPath, const TfToken& fieldName, const TfToken& oldValue) override;
    void _OnPopChild(const SdfPath& parentPath, const TfToken& fieldName, const SdfPath& oldValue) override;

private:
    void _OnSetFieldDictValueByKeyImpl(const SdfPath& path, const TfToken& fieldName, const TfToken& keyPath);
    void _OnSetTimeSampleImpl(const SdfPath& path, double time);

    template <class T>
    void _PopChild(const SdfPath& parentPath, const TfToken& fieldName, const T& oldValue);

private:
    SdfLayerHandle _layer;
    bool           _dirty;
    bool           _setMessageAlreadyShowed;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_UNDOSTATE_DELEGATE_H
