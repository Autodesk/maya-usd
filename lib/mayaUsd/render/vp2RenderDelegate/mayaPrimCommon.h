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

// Each instanced render item needs to map from a Maya instance id
// back to a usd instance id.
using InstanceIdMap = std::vector<unsigned int>;

// global singleton rather than MUserData, because consolidated world will
// not consolidate render items with different MUserData objects.
class MayaUsdCustomData
{
public:
    tbb::concurrent_unordered_map<int, InstanceIdMap> _itemData;

    static InstanceIdMap& Get(const MHWRender::MRenderItem& item);
};

struct MayaPrimCommon
{
    enum DirtyBits : HdDirtyBits
    {
        DirtySelection = HdChangeTracker::CustomBitsBegin,
        DirtySelectionHighlight = (DirtySelection << 1),
        DirtySelectionMode = (DirtySelectionHighlight << 1),
        DirtyBitLast = DirtySelectionMode
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_MAYA_PRIM_COMMON
