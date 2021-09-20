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
    if (auto ac = dynamic_cast<const Ufe::AttributeValueChanged*>(&notification)) {
        if (ac->name() == UsdGeomTokens->xformOpOrder) {
            MString cmd("if (`channelBox -exists mainChannelBox`) channelBox -q -mainObjectList "
                        "mainChannelBox;");
            MStringArray paths;
            if (MGlobal::executeCommand(cmd, paths) && (paths.length() > 0)) {
                auto ufePath = Ufe::PathString::path(paths[0].asChar());
                if (ufePath.startsWith(ac->path())) {
                    cmd = "channelBox -e -update mainChannelBox;";
                    MGlobal::executeCommand(cmd);
                }
            }
        }
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
