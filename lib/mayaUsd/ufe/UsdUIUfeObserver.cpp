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
#include "UsdUIUfeObserver.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <ufe/attributes.h>
#include <ufe/pathString.h>

// Needed because of TF_VERIFY
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
Ufe::Observer::Ptr UsdUIUfeObserver::ufeObserver;

//------------------------------------------------------------------------------
// UsdUIUfeObserver
//------------------------------------------------------------------------------

UsdUIUfeObserver::UsdUIUfeObserver()
    : Ufe::Observer()
{
}

/*static*/
void UsdUIUfeObserver::create()
{
    TF_VERIFY(!ufeObserver);
    if (!ufeObserver) {
        ufeObserver = std::make_shared<UsdUIUfeObserver>();
        Ufe::Attributes::addObserver(ufeObserver);
    }
}

/*static*/
void UsdUIUfeObserver::destroy()
{
    TF_VERIFY(ufeObserver);
    if (ufeObserver) {
        Ufe::Attributes::removeObserver(ufeObserver);
        ufeObserver.reset();
    }
}

//------------------------------------------------------------------------------
// Ufe::Observer overrides
//------------------------------------------------------------------------------

void UsdUIUfeObserver::operator()(const Ufe::Notification& notification)
{
    // Note: exceptions must not escape from this function, as the caller has no try
    //       block and would crash Maya.
    try {
        Ufe::Path pathToRefresh;
        if (auto ac = dynamic_cast<const Ufe::AttributeValueChanged*>(&notification)) {
            if (ac->name() == UsdGeomTokens->xformOpOrder) {
                pathToRefresh = ac->path();
            }
        }
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4024)
        else if (auto aa = dynamic_cast<const Ufe::AttributeAdded*>(&notification)) {
            pathToRefresh = aa->path();
        } else if (auto ar = dynamic_cast<const Ufe::AttributeRemoved*>(&notification)) {
            pathToRefresh = ar->path();
        }
#endif
#endif

        if (!pathToRefresh.empty()) {
            static const MString mainObjListCmd(
                "if (`channelBox -exists mainChannelBox`) channelBox -q -mainObjectList "
                "mainChannelBox;");
            MStringArray paths;
            if (MGlobal::executeCommand(mainObjListCmd, paths) && (paths.length() > 0)) {
                // Skip any non-absolute Maya paths (we know non-Maya ufe paths will always
                // start with |
                MString firstPath = paths[0];
                if (firstPath.substringW(0, 0) != "|")
                    return;

                auto ufePath = Ufe::PathString::path(firstPath.asChar());
                if (ufePath.startsWith(pathToRefresh)) {
                    static const MString updateCBCmd("channelBox -e -update mainChannelBox;");
                    MGlobal::executeCommand(updateCBCmd);
                }
            }
        }
    } catch (const std::exception& ex) {
        // Note: do not let the exception out, it would crash Maya.
        TF_WARN(
            "Exception during UFE notification about attribute changes in mayaUsd: %s", ex.what());
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
