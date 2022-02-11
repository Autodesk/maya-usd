//
// Copyright 2021 Autodesk
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
#include "OpUndoItemList.h"

#include <mayaUsd/listeners/notice.h>

#include <pxr/base/tf/weakBase.h>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// Notifier to automatically clear OpUndoItemList when the scene is reset.
//------------------------------------------------------------------------------

namespace {

struct OnSceneResetListener : public PXR_NS::TfWeakBase
{
    OnSceneResetListener()
    {
        PXR_NS::TfWeakPtr<OnSceneResetListener> self(this);
        PXR_NS::TfNotice::Register(self, &OnSceneResetListener::OnSceneReset);
        PXR_NS::TfNotice::Register(self, &OnSceneResetListener::OnMayaAboutToExit);
    }

    void OnSceneReset(const PXR_NS::UsdMayaSceneResetNotice& notice)
    {
        OpUndoItemList::instance().clear();
    }

    void OnMayaAboutToExit(const PXR_NS::UsdMayaExitNotice& notice)
    {
        OpUndoItemList::instance().clear();
    }
};

} // namespace

//------------------------------------------------------------------------------
// OpUndoItemList
//------------------------------------------------------------------------------

OpUndoItemList::OpUndoItemList(OpUndoItemList&& other) { *this = std::move(other); }

OpUndoItemList& OpUndoItemList::operator=(OpUndoItemList&& other)
{
    _undoItems = std::move(other._undoItems);
    other._undoItems.clear();

    _isUndone = other._isUndone;
    other._isUndone = false;

    return *this;
}

OpUndoItemList::~OpUndoItemList() { clear(); }

bool OpUndoItemList::undo()
{
    if (_isUndone)
        return true;

    bool overallSuccess = true;
    // Note: iterate in reverse order since operations might depend on each other.
    const auto end = _undoItems.rend();
    for (auto iter = _undoItems.rbegin(); iter != end; ++iter)
        overallSuccess &= (*iter)->undo();

    _isUndone = true;

    return overallSuccess;
}

bool OpUndoItemList::redo()
{
    if (!_isUndone)
        return true;

    bool overallSuccess = true;
    for (auto& item : _undoItems)
        overallSuccess &= item->redo();

    _isUndone = false;

    return overallSuccess;
}

void OpUndoItemList::addItem(OpUndoItem::Ptr&& item)
{
    // Note: OpUndoItem::Ptr are unique_ptr, so we need to take ownership of them.
    _undoItems.emplace_back(std::move(item));
}

void OpUndoItemList::clear()
{
    // Note: we need to destroy the undo items in reverse order
    //       since some items might depend on previous ones.
    if (_isUndone) {
        const auto end = _undoItems.rend();
        for (auto iter = _undoItems.rbegin(); iter != end; ++iter)
            iter->reset();
    } else {
        const auto end = _undoItems.end();
        for (auto iter = _undoItems.begin(); iter != end; ++iter)
            iter->reset();
    }

    _undoItems.clear();
    _isUndone = false;
}

OpUndoItemList& OpUndoItemList::instance()
{
    static OpUndoItemList       itemList;
    static OnSceneResetListener onSceneResetListener;

    return itemList;
}

} // namespace MAYAUSD_NS_DEF