//
// Copyright 2020 Autodesk
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
#include "pathChecker.h"

#include "layerTreeItem.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layerUtils.h>

#include <maya/MQtUtil.h>

#include <QtCore/QFileInfo>
#include <QtCore/QString>

#include <stdio.h>

namespace {
using namespace UsdLayerEditor;

typedef PXR_NS::SdfLayerRefPtr UsdLayer;
typedef std::vector<UsdLayer>  UsdLayerVector;

// helper for checkIfPathIsSafeToAdd
UsdLayerVector getAllParentHandles(LayerTreeItem* parentItem)
{
    UsdLayerVector result;
    while (parentItem != nullptr) {
        result.push_back(parentItem->layer());
        parentItem = parentItem->parentLayerItem();
    }
    return result;
}

// helper for checkIfPathIsSafeToAdd
// logic: we've loaded the layer, now check if it's also somewhere else in the hierachy
// the path parameter are just for UI display. it's testLayer that's important
bool checkPathRecursive(
    const QString&     in_errorTitle,
    UsdLayer           parentLayer,
    UsdLayerVector     parentHandles,
    UsdLayer           testLayer,
    const std::string& pathToCheck,
    const std::string& topPathToAdd)
{
    if (std::find(parentHandles.begin(), parentHandles.end(), testLayer) != parentHandles.end()) {
        QString message;

        if (pathToCheck != topPathToAdd) {
            MString tmp;
            tmp.format(
                StringResources::getAsMString(
                    StringResources::kErrorCannotAddPathInHierarchyThrough),
                pathToCheck.c_str(),
                topPathToAdd.c_str());
            message = MQtUtil::toQString(tmp);
        } else {
            MString tmp;
            tmp.format(
                StringResources::getAsMString(StringResources::kErrorCannotAddPathInHierarchy),
                pathToCheck.c_str());
            message = MQtUtil::toQString(tmp);
        }
        warningDialog(in_errorTitle, message);
        return false;
    }

    // now check all children of the testLayer recursivly down for conflicts with any of the parents
    parentHandles.push_back(testLayer);

    auto proxy = testLayer->GetSubLayerPaths();
    for (const auto path : proxy) {
        auto actualpath = PXR_NS::SdfComputeAssetPathRelativeToLayer(testLayer, path);

        auto childLayer = PXR_NS::SdfLayer::FindOrOpen(actualpath);
        if (childLayer != nullptr) {
            if (!checkPathRecursive(
                    in_errorTitle,
                    testLayer,
                    parentHandles,
                    childLayer,
                    actualpath,
                    topPathToAdd)) {
                return false;
            }
        }
    }
    parentHandles.pop_back();

    return true;
}

} // namespace

namespace UsdLayerEditor {

bool checkIfPathIsSafeToAdd(
    const QString&     in_errorTitle,
    LayerTreeItem*     in_parentItem,
    const std::string& in_pathToAdd)
{
    // We can't allow the user to add a sublayer path that is the same as the item or one of its
    // parent. At this point I think it's safe to go the route of actually loading the layer and
    // checking if the handles were already loaded

    auto parentLayer = in_parentItem->layer();

    // first check if the path is already in the stack
    auto proxy = parentLayer->GetSubLayerPaths();
    if (proxy.Find(in_pathToAdd) == size_t(-1)) {

        auto pathToAdd = PXR_NS::SdfComputeAssetPathRelativeToLayer(parentLayer, in_pathToAdd);

        // now we're going to check if the layer is already in the stack, through
        // another path
        bool foundLayerInStack = false;
        auto subLayer = PXR_NS::SdfLayer::FindOrOpen(pathToAdd);
        if (subLayer == nullptr) {
            return true; // always safe to add a bad path, unless it's already in the stack
        } else {
            // check the layer stack again, this time comparing handles
            for (const auto path : proxy) {
                std::string actualpath
                    = PXR_NS::SdfComputeAssetPathRelativeToLayer(parentLayer, path);

                auto childLayer = PXR_NS::SdfLayer::FindOrOpen(actualpath);
                if (childLayer == subLayer) {
                    foundLayerInStack = true;
                    break;
                }
            }
        }
        if (!foundLayerInStack) {
            UsdLayerVector parentHandles = getAllParentHandles(in_parentItem);
            return checkPathRecursive(
                in_errorTitle, parentLayer, parentHandles, subLayer, pathToAdd, pathToAdd);
        }
    }

    MString msg;
    msg.format(
        StringResources::getAsMString(StringResources::kErrorCannotAddPathTwice),
        in_pathToAdd.c_str());
    warningDialog(in_errorTitle, MQtUtil::toQString(msg));
    return false;
}

bool saveSubLayer(
    const QString&         in_errorTitle,
    LayerTreeItem*         in_parentItem,
    PXR_NS::SdfLayerRefPtr in_layer,
    const std::string&     in_absolutePath,
    const std::string&     in_formatTag)
{
    std::string backupFileName;
    QFileInfo   fileInfo(QString::fromStdString(in_absolutePath));
    bool        fileError = false;
    bool        success = false;
    if (fileInfo.exists()) {
        // backup existing file
        backupFileName = in_absolutePath + ".backup";
        remove(backupFileName.c_str());
        if (rename(in_absolutePath.c_str(), backupFileName.c_str()) != 0) {
            fileError = true;
        }
    }

    if (!fileError) {
        if (!MayaUsd::utils::saveLayerWithFormat(in_layer, in_absolutePath, in_formatTag)) {
            fileError = true;
        } else {
            // parent item is null if we're saving the root later
            if (in_parentItem != nullptr) {
                success = checkIfPathIsSafeToAdd(in_errorTitle, in_parentItem, in_absolutePath);
            } else {
                success = true;
            }
        }
    }

    // place back the file on failure
    if (!success && !backupFileName.empty()) {
        remove(in_absolutePath.c_str());
        rename(backupFileName.c_str(), in_absolutePath.c_str());
    }

    if (fileError) {
        MString msg;
        msg.format(
            StringResources::getAsMString(StringResources::kErrorFailedToSaveFile),
            in_absolutePath.c_str());

        warningDialog(in_errorTitle, MQtUtil::toQString(msg));
        return false;
    }

    return success;
}

} // namespace UsdLayerEditor
