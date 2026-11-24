//
// Copyright 2026 Autodesk
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
#include "stringResources.h"

#include <maya/MStatus.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>

namespace MAYAUSD_NS_DEF {
namespace StringResources {

// Define each resource once. The extern declarations in the header give these external linkage.
#define DEFINE_MAYAUSD_STR(key, value) const Resource key { "MayaUsd", #key, value };
MAYAUSD_STRING_LIST(DEFINE_MAYAUSD_STR)
#undef DEFINE_MAYAUSD_STR

namespace {

// Maya's string-resource plugin id, for registrying strings for translation with Maya.
// Note: the pluginId param MUST match the plugin name so that the Maya string extraction
//       command can find them and extract them for translation.
constexpr const char* kStringResourcePluginId = "mayaUsdPlugin";

MStringResourceId toMayaId(const Resource& resource)
{
    return MStringResourceId(
        kStringResourcePluginId, resource._key.c_str(), resource._value.c_str());
}
} // namespace

MString getAsMString(const Resource& stringResourceID)
{
    MStatus       status;
    const MString text = MStringResource::getString(toMayaId(stringResourceID), status);
    // Fall back to the source text if Maya has no entry for this key.
    return status ? text : MString(stringResourceID._value.c_str());
}

std::string getAsString(const Resource& stringResourceID)
{
    return getAsMString(stringResourceID).asChar();
}

bool registerString(const Resource& stringResourceID)
{
    return bool(MStringResource::registerString(toMayaId(stringResourceID)));
}

bool registerResources(const ResourceMap& resources)
{
    // Keep going after a failure to register as many strings as possible, and report the last.
    MStatus status;
    StringResourceBase::registerAll(resources, [&status](const Resource& resource) {
        const MStatus s = MStringResource::registerString(toMayaId(resource));
        if (MFAIL(s)) {
            status = s;
        }
        return !MFAIL(s);
    });
    CHECK_MSTATUS_AND_RETURN(status, false);
    return true;
}

bool registerStrings()
{
    return registerResources(STRING_RESOURCE_BASE_MAKE_MAP(MAYAUSD_STRING_LIST));
}

} // namespace StringResources
} // namespace MAYAUSD_NS_DEF
