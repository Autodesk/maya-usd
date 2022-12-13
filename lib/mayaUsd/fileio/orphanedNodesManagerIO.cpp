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

#include "orphanedNodesManager.h"

#include <mayaUsd/utils/json.h>

#include <pxr/base/js/json.h>
#include <pxr/base/tf/diagnostic.h>

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <ufe/pathString.h>
#include <ufe/trie.imp.h>

namespace MAYAUSD_NS_DEF {

namespace {

////////////////////////////////////////////////////////////////////////////
//
// Conversion of the OrphanedNodesManager PullVariantInfo to JSON has the
// following structure:
//
//    {
//       "/UFE-path-component-1" : {
//          "/UFE-path-component-2" : {
//             "pull info": {
//                "editedAsMayaRoot": "DAG-path-of-root-of-generated-Maya-data"
//                "variantSetDescriptors": [
//                   {
//                       "path": "UFE-path-of-one-ancestor",
//                       "variantSelections": [
//                           [ "variant-set-1-name", "variant-set-1-selection" ],
//                           [ "variant-set-2-name", "variant-set-2-selection" ],
//                       ],
//                   },
//                ],
//             },
//          },
//       },
//    }
//
// Each UFE path component is prefixed by a slash ('/') to differentiate them
// from pull info data, which has a JOSN key without that slash prefix.

static const std::string ufeComponentPrefix = "/";
static const std::string pullInfoJsonKey = "pull info";
static const std::string editedAsMayaRootJsonKey = "editedAsMayaRoot";
static const std::string variantSetDescriptorsJsonKey = "variantSetDescriptors";
static const std::string pathJsonKey = "path";
static const std::string variantSelKey = "variantSelections";

static const char* invalidJson = "Invalid JSON";

////////////////////////////////////////////////////////////////////////////
//
// Types used in the pulled variant info, so shorten the code and make it more readable.

using VariantSelection = OrphanedNodesManager::VariantSelection;
using VariantSetDesc = OrphanedNodesManager::VariantSetDescriptor;
using VariantSetDescList = std::list<VariantSetDesc>;
using PullVariantInfo = OrphanedNodesManager::PullVariantInfo;
using PullInfoTrie = Ufe::Trie<PullVariantInfo>;
using PullInfoTrieNode = Ufe::TrieNode<PullVariantInfo>;
using Memento = OrphanedNodesManager::Memento;

////////////////////////////////////////////////////////////////////////////
//
// Conversion functions to and from JSON for orphaned nodes types.

using MAYAUSD_NS_DEF::convertToArray;
using MAYAUSD_NS_DEF::convertToObject;
using MAYAUSD_NS_DEF::convertToValue;

PXR_NS::JsArray  convertToArray(const VariantSelection& variantSel);
PXR_NS::JsObject convertToObject(const VariantSetDesc& variantDesc);
PXR_NS::JsArray  convertToArray(const std::list<VariantSetDesc>& allVariantDesc);
PXR_NS::JsObject convertToObject(const PullVariantInfo& pullInfo);
PXR_NS::JsObject convertToObject(const PullInfoTrieNode::Ptr& pullInfoNode);
PXR_NS::JsObject convertToObject(const PullInfoTrie& allPulledInfo);

VariantSelection   convertToVariantSelection(const PXR_NS::JsArray& variantSelJson);
VariantSetDesc     convertToVariantSetDescriptor(const PXR_NS::JsObject& variantDescJson);
VariantSetDescList convertToVariantSetDescList(const PXR_NS::JsArray& allVariantDescJson);
PullVariantInfo    convertToPullVariantInfo(const PXR_NS::JsObject& pullInfoJson);
void         convertToPullInfoTrieNodePtr(const PXR_NS::JsObject&, PullInfoTrieNode::Ptr intoRoot);
PullInfoTrie convertToPullInfoTrie(const PXR_NS::JsObject& allPulledInfoJson);

PXR_NS::JsArray convertToArray(const VariantSelection& variantSel)
{
    PXR_NS::JsArray variantSelJson;

    variantSelJson.push_back(convertToValue(variantSel.variantSetName));
    variantSelJson.push_back(convertToValue(variantSel.variantSelection));

    return variantSelJson;
}

VariantSelection convertToVariantSelection(const PXR_NS::JsArray& variantSelJson)
{
    VariantSelection variantSel;

    if (variantSelJson.size() < 2)
        throw std::runtime_error(invalidJson);

    variantSel.variantSetName = convertToString(variantSelJson[0]);
    variantSel.variantSelection = convertToString(variantSelJson[1]);

    return variantSel;
}

PXR_NS::JsObject convertToObject(const VariantSetDesc& variantDesc)
{
    PXR_NS::JsObject variantDescJson;

    variantDescJson[pathJsonKey] = convertToValue(variantDesc.path);

    PXR_NS::JsArray selections;

    for (const auto& variantSel : variantDesc.variantSelections) {
        selections.emplace_back(convertToArray(variantSel));
    }

    variantDescJson[variantSelKey] = selections;

    return variantDescJson;
}

VariantSetDesc convertToVariantSetDescriptor(const PXR_NS::JsObject& variantDescJson)
{
    VariantSetDesc variantDesc;

    variantDesc.path = convertToUfePath(convertJsonKeyToValue(variantDescJson, pathJsonKey));

    PXR_NS::JsArray variantSelectionsJson
        = convertToArray(convertJsonKeyToValue(variantDescJson, variantSelKey));

    for (const PXR_NS::JsValue& value : variantSelectionsJson)
        variantDesc.variantSelections.emplace_back(
            convertToVariantSelection(convertToArray(value)));

    return variantDesc;
}

PXR_NS::JsArray convertToArray(const std::list<VariantSetDesc>& allVariantDesc)
{
    PXR_NS::JsArray allVariantDescJson;

    for (const auto& variantDesc : allVariantDesc) {
        allVariantDescJson.emplace_back(convertToObject(variantDesc));
    }

    return allVariantDescJson;
}

VariantSetDescList convertToVariantSetDescList(const PXR_NS::JsArray& allVariantDescJson)
{
    VariantSetDescList allVariantDesc;

    for (const PXR_NS::JsValue& value : allVariantDescJson)
        allVariantDesc.emplace_back(convertToVariantSetDescriptor(convertToObject(value)));

    return allVariantDesc;
}

PXR_NS::JsObject convertToObject(const PullVariantInfo& pullInfo)
{
    PXR_NS::JsObject pullInfoJson;

    pullInfoJson[editedAsMayaRootJsonKey] = convertToValue(pullInfo.editedAsMayaRoot);
    pullInfoJson[variantSetDescriptorsJsonKey] = convertToArray(pullInfo.variantSetDescriptors);

    return pullInfoJson;
}

PullVariantInfo convertToPullVariantInfo(const PXR_NS::JsObject& pullInfoJson)
{
    PullVariantInfo pullInfo;

    pullInfo.editedAsMayaRoot
        = convertToDagPath(convertJsonKeyToValue(pullInfoJson, editedAsMayaRootJsonKey));
    pullInfo.variantSetDescriptors = convertToVariantSetDescList(
        convertToArray(convertJsonKeyToValue(pullInfoJson, variantSetDescriptorsJsonKey)));

    return pullInfo;
}

PXR_NS::JsObject convertToObject(const PullInfoTrieNode::Ptr& pullInfoNodePtr)
{
    if (!pullInfoNodePtr)
        return {};

    const PullInfoTrieNode& pullInfoNode = *pullInfoNodePtr;

    PXR_NS::JsObject pullInfoNodeJson;

    if (pullInfoNode.hasData()) {
        pullInfoNodeJson[pullInfoJsonKey] = convertToObject(pullInfoNode.data());
    }

    for (const auto& child : pullInfoNode.childrenComponents()) {
        PXR_NS::JsObject childJson = convertToObject(pullInfoNode[child]);
        if (childJson.empty())
            continue;
        pullInfoNodeJson[ufeComponentPrefix + child.string()] = childJson;
    }

    return pullInfoNodeJson;
}

void convertToPullInfoTrieNodePtr(
    const PXR_NS::JsObject& pullInfoNodeJson,
    PullInfoTrieNode::Ptr   intoRoot)
{
    for (const auto& keyValue : pullInfoNodeJson) {
        const std::string&     key = keyValue.first;
        const PXR_NS::JsValue& value = keyValue.second;
        if (key.size() <= 0) {
            continue;
        } else if (key == pullInfoJsonKey) {
            intoRoot->setData(convertToPullVariantInfo(convertToObject(value)));

        } else if (key[0] == '/') {
            PullInfoTrieNode::Ptr child = std::make_shared<PullInfoTrieNode>(key.substr(1));
            intoRoot->add(child);
            convertToPullInfoTrieNodePtr(convertToObject(value), child);
        }
    }
}

PXR_NS::JsObject convertToObject(const PullInfoTrie& allPullInfo)
{
    return convertToObject(allPullInfo.root());
}

PullInfoTrie convertToPullInfoTrie(const PXR_NS::JsObject& allPullInfoJson)
{
    PullInfoTrie allPullInfo;

    convertToPullInfoTrieNodePtr(allPullInfoJson, allPullInfo.root());

    return allPullInfo;
}

} // namespace

////////////////////////////////////////////////////////////////////////////
//
// Conversion of OrphanedNodesManager::Memento to and from JSON.

std::string Memento::convertToJson(const Memento& memento)
{
    try {
        return PXR_NS::JsWriteToString(convertToObject(memento._pulledPrims));
    } catch (const std::exception& e) {
        // Note: the TF_RUNTIME_ERROR macro needs to be used within the PXR_NS.
        using namespace PXR_NS;
        TF_RUNTIME_ERROR(
            "Unable to convert the orphaned nodes manager state to JSON: %s", e.what());
    }

    return {};
}

Memento Memento::convertFromJson(const std::string& json)
{
    Memento memento;

    try {
        memento._pulledPrims = convertToPullInfoTrie(convertToObject(PXR_NS::JsParseString(json)));
    } catch (const std::exception& e) {
        // Note: the TF_RUNTIME_ERROR macro needs to be used within the PXR_NS.
        using namespace PXR_NS;
        TF_RUNTIME_ERROR(
            "Unable to convert the JSON text to the orphaned nodes manager state: %s", e.what());
    }

    return memento;
}

} // namespace MAYAUSD_NS_DEF
