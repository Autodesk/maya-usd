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

#include "dirtyingSceneIndex.h"

#include <pxr/imaging/hd/dirtyBitsTranslator.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

DirtyingSceneIndex::DirtyingSceneIndex(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : PXR_NS::HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{

}

void DirtyingSceneIndex::MarkRprimDirty(const SdfPath& primPath, const HdDirtyBits& dirtyBits) {
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    HdDataSourceLocatorSet locators;
    HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet(prim.primType, dirtyBits, &locators);

    if (!locators.IsEmpty()) {
        _SendPrimsDirtied({{primPath, locators}});
    }

}
void DirtyingSceneIndex::MarkSprimDirty(const SdfPath& primPath, const HdDirtyBits& dirtyBits) {
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    HdDataSourceLocatorSet locators;
    HdDirtyBitsTranslator::SprimDirtyBitsToLocatorSet(prim.primType, dirtyBits, &locators);

    if (!locators.IsEmpty()) {
        _SendPrimsDirtied({{primPath, locators}});
    }
}

PXR_NAMESPACE_CLOSE_SCOPE