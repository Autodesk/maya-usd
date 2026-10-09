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
#include "initStringResources.h"

#include <mayaUsd/commands/mayaLayerEditorDCCFunctions.h>

#include <maya/MString.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>

#include <string>
#include <unordered_map>

#include <stringResources.h>

namespace {

// Keys whose Maya wording differs from the shared default.
const std::unordered_map<std::string, const char*> kMayaOverrides {
    // "Listener" is the 3ds Max window; Maya's equivalent is the Script Editor.
    { "kMenuPrintToScriptEditor", "Print to Script Editor" },
};

} // namespace

namespace MAYAUSD_NS_DEF {

MStatus initStringResources()
{
    MStatus status { MStatus::kSuccess };

    for (const auto& entry : UsdLayerEditor::StringResources::allResources()) {
        const std::string&                               key = entry.first;
        const UsdLayerEditor::StringResources::Resource& resource = entry.second;

        const auto        override_ = kMayaOverrides.find(key);
        const char* const text
            = (override_ != kMayaOverrides.end()) ? override_->second : resource.value.c_str();

        MStringResourceId id(UsdLayerEditor::kStringResourcePluginId, key.c_str(), text);
        if (!MStringResource::registerString(id))
            status = MStatus::kFailure;
    }

    return status;
}

} // namespace MAYAUSD_NS_DEF
