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
#include "utilFileSystem.h"

#include <usdLayerEditor/lib/layerEditorDCCFunctions.h>

#include <usdUfe/utils/fileSystem.h>

#include <pxr/usd/ar/resolver.h>

namespace UsdLayerEditor {
namespace FileSystem {

PXR_NAMESPACE_USING_DIRECTIVE

std::string getPathRelativeToDCCSceneFile(const std::string& fileName)
{
    auto relativePathAndSuccess
        = UsdUfe::FileSystem::makePathRelativeTo(fileName, UsdLayerEditor::getDCCSceneDir());

    if (!relativePathAndSuccess.second) {
        TF_WARN(
            "File name (%s) cannot be resolved as relative to the Maya scene file, using the "
            "absolute path.",
            fileName.c_str());
    }

    return relativePathAndSuccess.first;
}

bool prepareLayerSaveUILayer(const PXR_NS::SdfLayerHandle& layer, bool useSceneFileForRoot)
{
    std::string layerFileDir;
    if (layer) {
        layerFileDir = UsdUfe::FileSystem::getLayerFileDir(layer);
    } else if (useSceneFileForRoot) {
        layerFileDir = UsdLayerEditor::getDCCSceneDir();
    }

    return UsdLayerEditor::prepareLayerSaveUILayer(layerFileDir);
}

} // namespace FileSystem
} // namespace UsdLayerEditor
