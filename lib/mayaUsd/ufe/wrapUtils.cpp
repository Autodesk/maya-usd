//
// Copyright 2019 Autodesk
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
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr_python.h>

#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateStageWithNewLayerCommand.h>

#include <ufe/undoableCommandMgr.h>
#endif

#include <string>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

PXR_NS::UsdPrim getPrimFromRawItem(uint64_t rawItem)
{
    Ufe::SceneItem*       item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    UsdUfe::UsdSceneItem* usdItem = dynamic_cast<UsdUfe::UsdSceneItem*>(item);
    if (nullptr != usdItem) {
        return usdItem->prim();
    }
    return PXR_NS::UsdPrim();
}

std::string getNodeNameFromRawItem(uint64_t rawItem)
{
    std::string     name;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item)
        name = item->nodeName();
    return name;
}

std::string getNodeTypeFromRawItem(uint64_t rawItem)
{
    std::string     type;
    Ufe::SceneItem* item = reinterpret_cast<Ufe::SceneItem*>(rawItem);
    if (nullptr != item) {
        // Prepend the name of the runtime manager of this item to the type.
        type = Ufe::RunTimeMgr::instance().getName(item->runTimeId()) + item->nodeType();
    }
    return type;
}

std::vector<PXR_NS::UsdStageRefPtr> _getAllStages()
{
    auto                                allStages = MayaUsd::ufe::getAllStages();
    std::vector<PXR_NS::UsdStageRefPtr> output;
    for (auto stage : allStages) {
        PXR_NS::UsdStageRefPtr stageRefPtr { stage };
        output.push_back(stageRefPtr);
    }
    return output;
}

PXR_NS::TfTokenVector _getProxyShapePurposes(const std::string& ufePathString)
{
    auto path = Ufe::PathString::path(ufePathString);
    return MayaUsd::ufe::getProxyShapePurposes(path);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string createStageWithNewLayer(const std::string& parentPathString)
{
    // Input path parent string is allowed to be empty in which case we'll
    // parent the new stage to the Maya world node.
    Ufe::SceneItem::Ptr parent;
    if (!parentPathString.empty()) {
        auto parentPath = Ufe::PathString::path(parentPathString);
        parent = Ufe::Hierarchy::createItem(parentPath);
    }
    auto command = MayaUsd::ufe::UsdUndoCreateStageWithNewLayerCommand::create(parent);
    if (!command) {
        return "";
    }

    Ufe::UndoableCommandMgr::instance().executeCmd(command);
    if (!command->sceneItem()) {
        return "";
    }

    return Ufe::PathString::string(command->sceneItem()->path());
}
#endif

void wrapUtils()
{
    def("getPrimFromRawItem", getPrimFromRawItem);
    def("getNodeNameFromRawItem", getNodeNameFromRawItem);
    def("getNodeTypeFromRawItem", getNodeTypeFromRawItem);

#ifdef UFE_V4_FEATURES_AVAILABLE
    def("createStageWithNewLayer", createStageWithNewLayer);
#endif

    // Because mayaUsd and UFE have incompatible Python bindings that do not
    // know about each other (provided by Boost/Pxr Python and pybind11,
    // respectively), we cannot pass in or return UFE objects such as Ufe::Path
    // here, and are forced to use strings.  Use the tentative string
    // representation of Ufe::Path as comma-separated segments.  We know that
    // the USD path separator is '/'.  PPT, 8-Dec-2019.
    def("getAllStages", _getAllStages, return_value_policy<PXR_NS::TfPySequenceToList>());
    def("getProxyShapePurposes", _getProxyShapePurposes);
}
