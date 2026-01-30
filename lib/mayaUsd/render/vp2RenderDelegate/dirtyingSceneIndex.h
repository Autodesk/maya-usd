//
// Copyright 2025 Autodesk
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

#ifndef HD_VP2_DIRTYING_SCENE_INDEX
#define HD_VP2_DIRTYING_SCENE_INDEX

#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/imaging/hd/sceneIndex.h>
#include <pxr/imaging/hd/types.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(DirtyingSceneIndex);

class DirtyingSceneIndex : public PXR_NS::HdSingleInputFilteringSceneIndexBase
{
public:
    static DirtyingSceneIndexRefPtr New(const HdSceneIndexBaseRefPtr &inputSceneIndex) {
        return PXR_NS::TfCreateRefPtr(new DirtyingSceneIndex(inputSceneIndex));
    }

    void MarkRprimDirty(const SdfPath& primPath, const HdDirtyBits& dirtyBits);

    void MarkSprimDirty(const SdfPath& primPath, const HdDirtyBits& dirtyBits);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    DirtyingSceneIndex(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override {
            _SendPrimsAdded(entries);
        }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override {
            _SendPrimsRemoved(entries);
        }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override {
            _SendPrimsDirtied(entries);
        }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_DIRTYING_SCENE_INDEX