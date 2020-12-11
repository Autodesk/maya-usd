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
#ifndef PATHCHECKER_H
#define PATHCHECKER_H

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/sdf/layer.h>

#include <algorithm>
#include <string>

class QString;
namespace UsdLayerEditor {

class LayerTreeItem;

// check if it's safe to add a sub path on a layer by
// verifying that it would not be creating a recursion
// bad paths are always allowed, because they could be custom URIs or future paths
// used for Load Layers
bool checkIfPathIsSafeToAdd(
    const QString&     in_errorTitle,
    LayerTreeItem*     in_parentItem,
    const std::string& in_pathToAdd);

// check if it's safe to save an anon sublayer to the given path
// and then does it.
// stategy:
// save the layer, then use the same logic as loadLayers to
// see if it actually can add this path without creating a recursion
// if that fails, delete the file we created.
// for now, assumes an absolute input path
bool saveSubLayer(
    const QString&         in_errorTitle,
    LayerTreeItem*         in_parentItem,
    PXR_NS::SdfLayerRefPtr in_layer,
    const std::string&     in_absolutePath,
    const std::string&     in_formatTag);

// convert path string to use forward slashes
inline std::string toForwardSlashes(const std::string& in_path)
{
    // it works better on windows if all the paths have forward slashes
    auto path = in_path;
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

// contains the logic to get the right path to use for SdfLayer:::FindOrOpen
// from a sublayerpath.
// this path could be absolute, relative, or be an anon layer
inline std::string computePathToLoadSublayer(
    const std::string&  subLayerPath,
    const std::string&  anchor,
    PXR_NS::ArResolver& resolver)
{
    std::string actualPath = subLayerPath;
    if (resolver.IsRelativePath(subLayerPath)) {
        auto subLayer = PXR_NS::SdfLayer::Find(subLayerPath); // note: finds in the cache
        if (subLayer) {
            if (!resolver.IsRelativePath(subLayer->GetIdentifier())) {
                actualPath = toForwardSlashes(subLayer->GetRealPath());
            }
        } else {
            actualPath = resolver.AnchorRelativePath(anchor, subLayerPath);
        }
    }
    return actualPath;
}

} // namespace UsdLayerEditor

#endif // PATHCHECKER_H
