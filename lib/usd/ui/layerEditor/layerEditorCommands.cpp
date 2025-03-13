//
// Copyright 2025 Autodesk
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

#include "layerEditorWidgetManager.h"
#include "layerEditorCommands.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnosticLite.h>

#include <maya/MSyntax.h>
#include <maya/MArgParser.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {
const MString GetSelectedWidgetLayersMpxCommand::commandName("mayaUsdGetSelectedLayers");

void* GetSelectedWidgetLayersMpxCommand::creator()
{
    return static_cast<MPxCommand*>(new GetSelectedWidgetLayersMpxCommand());
}

MSyntax GetSelectedWidgetLayersMpxCommand::createSyntax()
{
    MSyntax syntax;
    return syntax;
}

MStatus GetSelectedWidgetLayersMpxCommand::doIt(const MArgList& args)
{
    MStatus status(MS::kSuccess);

    std::vector<std::string> layers
        = LayerEditorWidgetManager::getInstance()->getSelectedLayers();
    MStringArray results;
    for (int i = 0; i < layers.size(); i++) {
        results.append(layers[i].c_str());
    }
    setResult(results);
    return status;
};

const MString SetSelectedWidgetLayersMpxCommand::commandName("mayaUsdSetSelectedLayers");

void* SetSelectedWidgetLayersMpxCommand::creator()
{
    return static_cast<MPxCommand*>(new SetSelectedWidgetLayersMpxCommand());
}

MSyntax SetSelectedWidgetLayersMpxCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag(kLayersFlag, kLayersFlagLong, MSyntax::kString);
    return syntax;
}

MStatus SetSelectedWidgetLayersMpxCommand::parse(const MString& layersString)
{
    MStatus status = MS::kSuccess;

    if (layersString.length() > 0) {
        int i, length;
        MStringArray layersList;
        layersString.split(';', layersList); // break out all the layers

        length = layersList.length();
        for (i = 0; i < length; ++i) {
            layers.emplace_back(layersList[i].asChar());
        }
    }
    return status;
}

MStatus SetSelectedWidgetLayersMpxCommand::doIt(const MArgList& args)
{
    MStatus status(MS::kSuccess);
    MArgParser argData(syntax(), args, &status);

    if (argData.isFlagSet("l", &status)) {
        MString layersString;
        argData.getFlagArgument(kLayersFlag, 0, layersString);
        parse(layersString);
    } else {
        TF_RUNTIME_ERROR("-layers not specified.");
        return MS::kFailure;
    }

    LayerEditorWidgetManager::getInstance()->selectLayers(layers);

    return status;
};
} // namespace UsdLayerEditor