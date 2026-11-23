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
#include "exportStageEdits.h"

#include <mayaUsd/base/tokens.h>

#include <usdUfe/utils/stageEdits.h>

#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

// Opinions that Maya-USD authors in the session layer for its own needs. They only
// make sense in the Maya scene, so they are not exported:
//   - on prims edited as Maya: the pull information, and the deactivation of the prim
//     (so that only the Maya data shows), see fileio/pullInformation.cpp,
//   - on MayaReference prims: the name of the Maya reference node, see
//     fileio/translators/translatorMayaReference.cpp,
//   - the text color of the prims in the Outliner, see displayCustomControl.py.
const char* const kPullPrimMetadataKey = "Maya:Pull:DagPath";
const TfToken     kMayaReferenceNodeName("MayaReferenceNodeName");
const char* const kOutlinerColorKeys[] = { "Autodesk:Use Text Color", "Autodesk:Text Color" };

// Removes the value at the key path, and the dictionaries it leaves empty.
void removeValueAtPath(VtDictionary& dict, const std::string& keyPath)
{
    dict.EraseValueAtPath(keyPath);

    std::vector<std::string> keys = TfStringTokenize(keyPath, ":");
    while (keys.size() > 1) {
        keys.pop_back();
        const VtValue* parent = dict.GetValueAtPath(keys);
        if (!parent || !parent->IsHolding<VtDictionary>()
            || !parent->UncheckedGet<VtDictionary>().empty())
            break;
        dict.EraseValueAtPath(keys);
    }
}

// Removes the Maya-USD bookkeeping (see above) from a copy of the session layer.
void removeMayaUsdBookkeeping(const SdfLayerHandle& layer)
{
    SdfChangeBlock changeBlock;

    std::vector<SdfPath> primPaths;
    layer->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath& path) {
        if (layer->GetSpecType(path) == SdfSpecTypePrim)
            primPaths.push_back(path);
    });

    for (const SdfPath& path : primPaths) {
        const SdfPrimSpecHandle prim = layer->GetPrimAtPath(path);
        if (SdfPropertySpecHandle property
            = layer->GetPropertyAtPath(path.AppendProperty(kMayaReferenceNodeName)))
            prim->RemoveProperty(property);

        VtValue customData;
        if (!layer->HasField(path, SdfFieldKeys->CustomData, &customData)
            || !customData.IsHolding<VtDictionary>())
            continue;

        VtDictionary dict = customData.UncheckedGet<VtDictionary>();
        if (dict.GetValueAtPath(kPullPrimMetadataKey))
            layer->EraseField(path, SdfFieldKeys->Active);

        removeValueAtPath(dict, kPullPrimMetadataKey);
        for (const char* key : kOutlinerColorKeys)
            removeValueAtPath(dict, key);

        if (dict.empty())
            layer->EraseField(path, SdfFieldKeys->CustomData);
        else
            layer->SetField(path, SdfFieldKeys->CustomData, VtValue(dict));
    }
}

} // namespace

std::string getStageEditsFolder(const std::string& exportFilePath)
{
    const std::string baseName = TfStringGetBeforeSuffix(TfGetBaseName(exportFilePath));
    return TfStringCatPaths(TfGetPathName(exportFilePath), baseName + "_edits");
}

ExportedStageEdits exportStageEdits(
    const UsdStageRefPtr& stage,
    const std::string&    destFolder,
    const std::string&    fileFormat,
    bool                  relativeOriginalPaths)
{
    ExportedStageEdits result;
    if (!stage)
        return result;

    const std::string path = TfStringCatPaths(
        destFolder, std::string("edits.") + (fileFormat == "usda" ? "usda" : "usdc"));

    UsdUfe::StageEditsOptions options;
    options.layerPath = path;
    options.relativeOriginalPaths = relativeOriginalPaths;
    options.sessionLayerFilter = removeMayaUsdBookkeeping;

    UsdUfe::StageEdits edits = UsdUfe::extractStageEdits(stage, options);
    result.warnings = std::move(edits.warnings);
    if (!edits.layer
        || (edits.layer->GetRootPrims().empty() && edits.layer->GetNumSubLayerPaths() == 0))
        return result;

    // Don't export the bookkeeping of the unshared stages of Maya-USD.
    VtDictionary customData = edits.layer->GetCustomLayerData();
    if (customData.erase(MayaUsdMetadata->ReferencedLayers.GetString()) > 0) {
        if (customData.empty())
            edits.layer->ClearCustomLayerData();
        else
            edits.layer->SetCustomLayerData(customData);
    }

    // Never overwrite a layer that is open, e.g. the edits of a previous export opened
    // in a stage: the stage would be changed under its feet.
    if (SdfLayer::Find(path)) {
        result.warnings.push_back(TfStringPrintf(
            "Could not write the file '%s': it is open in Maya. Close the stages using it, or "
            "export to another file.",
            path.c_str()));
        return result;
    }

    if (!TfIsDir(destFolder) && !TfMakeDirs(destFolder, -1, true)) {
        result.warnings.push_back(
            TfStringPrintf("Could not create the folder '%s'.", destFolder.c_str()));
        return result;
    }

    if (!edits.layer->Export(path)) {
        result.warnings.push_back(TfStringPrintf("Could not write the file '%s'.", path.c_str()));
        return result;
    }

    result.referencedLayer = path;
    return result;
}

} // namespace MAYAUSD_NS_DEF
