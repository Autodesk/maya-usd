//
// Copyright 2022 Autodesk
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

#include "displayName.h"

#include <mayaUsd/utils/json.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/arch/env.h>
#include <pxr/base/js/json.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/stringUtils.h>

#include <maya/MGlobal.h>

#include <fstream>

namespace {

using RemovedPrefixes = std::set<std::string>;
using AttributeMappings = std::map<std::string, std::string>;

RemovedPrefixes   removedPrefixes;
AttributeMappings attributeMappings;

const std::string versionKey = "version";
const std::string removedPrefixesKey = "removed_prefixes";
const std::string attributeMappingsKey = "attribute_mappings";
const std::string mappingFileName = "attribute_mappings.json";

using MayaUsd::convertJsonKeyToValue;
using MayaUsd::convertToArray;
using MayaUsd::convertToDouble;
using MayaUsd::convertToObject;
using MayaUsd::convertToString;
using MayaUsd::convertToValue;

// Extract the version entry from the given JSON.
double getAttributeMappingsVersion(const PXR_NS::JsObject& mappingJSON)
{
    return convertToDouble(convertJsonKeyToValue(mappingJSON, versionKey));
}

// Extract the valid removed prefix entries from the given JSON and add them to the given set.
void loadRemovedPrefixes(const PXR_NS::JsObject& mappingJSON, RemovedPrefixes& removed)
{
    const PXR_NS::JsArray array
        = convertToArray(convertJsonKeyToValue(mappingJSON, removedPrefixesKey));

    for (auto& value : array)
        removed.insert(PXR_NS::TfStringToLower(convertToString(value)));
}

// Extract the attribute mappings entries from the given JSON and add
// them into the given map, overwriting previous entries if necessary.
void loadAttributeMappings(const PXR_NS::JsObject& mappingJSON, AttributeMappings& mappings)
{
    const PXR_NS::JsObject obj
        = convertToObject(convertJsonKeyToValue(mappingJSON, attributeMappingsKey));

    for (auto& item : obj) {
        const std::string key = PXR_NS::TfStringToLower(item.first);
        const std::string value = convertToString(item.second);
        // Note: don't use std::map::insert() as that fails to overwrite existing entries.
        mappings[key] = value;
    }
}

void loadFolderAttributeNameMappings(const std::string& folder)
{
    if (folder.empty())
        return;

    // Verify if the file exists to avoid reporting errors about non-existant
    // attribute mappings.
    std::string filename = PXR_NS::UsdMayaUtilFileSystem::appendPaths(folder, mappingFileName);
    if (!PXR_NS::TfPathExists(filename))
        return;

    try {
        std::ifstream          mappingFile(filename);
        const PXR_NS::JsObject mappingJSON = convertToObject(PXR_NS::JsParseStream(mappingFile));

        if (getAttributeMappingsVersion(mappingJSON) < 1.0)
            return;

        loadRemovedPrefixes(mappingJSON, removedPrefixes);
        loadAttributeMappings(mappingJSON, attributeMappings);
    } catch (std::exception& ex) {
        const std::string msg = PXR_NS::TfStringPrintf(
            "Could not load the attribute mappings JSON file [%s].\n%s",
            filename.c_str(),
            ex.what());
        MGlobal::displayInfo(msg.c_str());
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {

// Load the attribute mappings.
void loadAttributeNameMappings(const std::string& pluginFilePath)
{
    loadFolderAttributeNameMappings(
        PXR_NS::UsdMayaUtilFileSystem::joinPaths({ pluginFilePath, "..", "..", "..", "lib" }));

    // Note: order is important as the following user-defined mappings take precedence
    //       and must be loaded last, possibly over-writing existing mappings.
    loadFolderAttributeNameMappings(PXR_NS::UsdMayaUtilFileSystem::getMayaPrefDir());
}

// Convert the attribute name into a nice name.
std::string getAttributeDisplayName(const std::string& attrName)
{
    std::string niceName = attrName;
    std::string lowerName = PXR_NS::TfStringToLower(attrName);
    for (const std::string& prefix : removedPrefixes) {
        if (PXR_NS::TfStringStartsWith(lowerName, prefix)) {
            niceName = niceName.substr(prefix.size());
            lowerName = lowerName.substr(prefix.size());
        }
    }

    if (attributeMappings.count(lowerName)) {
        niceName = attributeMappings[lowerName];
    }

    return niceName;
}

} // namespace MAYAUSD_NS_DEF
