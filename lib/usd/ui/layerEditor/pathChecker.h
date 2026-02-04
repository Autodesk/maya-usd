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
class QWidget;
namespace UsdLayerEditor {

class LayerTreeItem;

// check if it's safe to add a sub path on a layer by
// verifying that it would not be creating a recursion
// bad paths are always allowed, because they could be custom URIs or future paths
// used for Load Layers
// in corner cases the parent layer is null, we assume its safe to add
bool checkIfPathIsSafeToAdd(
    QWidget*           parent,
    const QString&     in_errorTitle,
    LayerTreeItem*     in_parentItem,
    const std::string& in_pathToAdd);

} // namespace UsdLayerEditor

#endif // PATHCHECKER_H
