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

#include "mayaPrimCommon.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT

namespace {

MayaUsdCustomData sMayaUsdCustomData;

} // namespace

/* static */
InstanceIdMap& MayaUsdCustomData::Get(const MHWRender::MRenderItem& renderItem)
{
    return sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._instanceIdMap;
}

/* static */
void MayaUsdCustomData::Remove(const MHWRender::MRenderItem& renderItem)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    sMayaUsdCustomData._itemData.unsafe_erase(renderItem.InternalObjectId());
}

/* static */
bool MayaUsdCustomData::ItemDataDirty(const MHWRender::MRenderItem& renderItem)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    return sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._itemDataDirty;
}

/* static */
void MayaUsdCustomData::ItemDataDirty(const MHWRender::MRenderItem& renderItem, bool dirty)
{
    // not thread safe, so if they are destroyed in parallel this will crash.
    // consider concurrent_hash_map for locking version that can erase
    sMayaUsdCustomData._itemData[renderItem.InternalObjectId()]._itemDataDirty = dirty;
}

/* static */
InstancePrimPaths& MayaUsdCustomData::GetInstancePrimPaths(const SdfPath& prim)
{
    return sMayaUsdCustomData._primData[prim]._instancePrimPaths;
}

/* static */
void MayaUsdCustomData::RemoveInstancePrimPaths(const SdfPath& prim)
{
    sMayaUsdCustomData._primData.unsafe_erase(prim);
}

#endif

MayaUsdRPrim::MayaUsdRPrim(HdVP2RenderDelegate* delegate)
{
    
}

PXR_NAMESPACE_CLOSE_SCOPE
