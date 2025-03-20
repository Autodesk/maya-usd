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
#ifndef LAYER_EDITOR_COMMANDS_H
#define LAYER_EDITOR_COMMANDS_H

#include <mayaUsdUI/ui/api.h>

#include <maya/MPxCommand.h>

#include <vector>

namespace UsdLayerEditor {

/**
 * @brief Class that defines the "mayaUsdGetSelectedLayers" MEL command.
 *
 * This command allows the user to query the selected layers (i.e. selected rows)
 * in the layer editor widget.
 **/
class MAYAUSD_UI_PUBLIC GetSelectedWidgetLayersMpxCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const MString commandName;
    static void*         creator();
    static MSyntax       createSyntax();

    MStatus doIt(const MArgList& args) override;
    bool    isUndoable() const override { return false; }
};

/**
 * @brief Class that defines the "mayaUsdSetSelectedLayers -l 'layer_id_1;layer_id_2" MEL command.
 *
 * This command allows the user to set the selected layers (i.e. selected rows)
 * in the layer editor widget.
 **/
class MAYAUSD_UI_PUBLIC SetSelectedWidgetLayersMpxCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const MString commandName;
    static void*         creator();
    static MSyntax       createSyntax();

    MStatus doIt(const MArgList& args) override;
    bool    isUndoable() const override { return false; }

    MStatus parse(const MString& layersString);

private:
    std::vector<std::string> _layers;
    static constexpr auto    kLayersFlag = "l";
    static constexpr auto    kLayersFlagLong = "layers";
};

} // namespace UsdLayerEditor

#endif //LAYER_EDITOR_COMMANDS_H