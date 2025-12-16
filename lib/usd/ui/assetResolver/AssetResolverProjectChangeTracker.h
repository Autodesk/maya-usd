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

#ifndef MAYAUSDUI_USD_ASSETRESOLVERPROJECTCHANGETRACKER_H
#define MAYAUSDUI_USD_ASSETRESOLVERPROJECTCHANGETRACKER_H

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <maya/MStatus.h>

namespace MAYAUSD_NS_DEF {

/** \class AssetResolverProjectChangeTracker
 \brief A utility class to track Maya project changes and update the
        Autodesk USD Asset Resolver settings accordingly.
 */
class MAYAUSD_UI_PUBLIC AssetResolverProjectChangeTracker
{
public:
    // Callback function for Maya project change events
    static void onProjectChanged(void* clientData);
    // Start tracking Maya project changes
    static MStatus startTracking();
    // Stop tracking Maya project changes
    static MStatus stopTracking();
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSDUI_USD_ASSETRESOLVERPROJECTCHANGETRACKER_H
