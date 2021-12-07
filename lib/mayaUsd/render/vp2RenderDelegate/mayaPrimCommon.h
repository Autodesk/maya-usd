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
#ifndef HD_VP2_MAYA_PRIM_COMMON
#define HD_VP2_MAYA_PRIM_COMMON

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/types.h"

#include <maya/MHWGeometry.h>

#include <tbb/concurrent_unordered_map.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
// Each instanced render item needs to map from a Maya instance id
// back to a usd instance id.
using InstanceIdMap = std::vector<unsigned int>;

using InstancePrimPaths = std::vector<SdfPath>;

// global singleton rather than MUserData, because consolidated world will
// not consolidate render items with different MUserData objects.
class MayaUsdCustomData
{
public:
    struct MayaUsdRenderItemData
    {
        InstanceIdMap _instanceIdMap;
        bool          _itemDataDirty { false };
    };

    struct MayaUsdPrimData
    {
        InstancePrimPaths _instancePrimPaths;
    };

    tbb::concurrent_unordered_map<int, MayaUsdRenderItemData>              _itemData;
    tbb::concurrent_unordered_map<SdfPath, MayaUsdPrimData, SdfPath::Hash> _primData;

    static InstanceIdMap& Get(const MHWRender::MRenderItem& item);
    static void           Remove(const MHWRender::MRenderItem& item);
    static bool           ItemDataDirty(const MHWRender::MRenderItem& item);
    static void           ItemDataDirty(const MHWRender::MRenderItem& item, bool dirty);

    static InstancePrimPaths& GetInstancePrimPaths(const SdfPath& prim);
    static void               RemoveInstancePrimPaths(const SdfPath& prim);
};
#endif

struct MayaPrimCommon
{
    enum DirtyBits : HdDirtyBits
    {
        DirtySelectionHighlight = HdChangeTracker::CustomBitsBegin,
        DirtySelectionMode = (DirtySelectionHighlight << 1),
        DirtyBitLast = DirtySelectionMode
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_MAYA_PRIM_COMMON
