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

#ifndef LAYEREDITORWINDOWCOMMAND_H
#define LAYEREDITORWINDOWCOMMAND_H

#include <mayaUsd/base/api.h>

#include <maya/MArgParser.h>
#include <maya/MPxCommand.h>

namespace MAYAUSD_NS_DEF {

class AbstractLayerEditorWindow;

class MAYAUSD_CORE_PUBLIC LayerEditorWindowCommand : public MPxCommand
{
public:
    LayerEditorWindowCommand();
    // plugin registration requirements
    static const char commandName[];
    static void*      creator();
    static MSyntax    createSyntax();

    // cleanup function to be called to delete the windows if the plugin is unloaded
    static void cleanupOnPluginUnload();

    // MPxCommand callbacks
    MStatus doIt(const MArgList& argList) override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    bool    isUndoable() const override;

protected:
    MStatus handleQueries(const MArgParser& argParser, AbstractLayerEditorWindow* layerEditor);
    MStatus handleEdits(const MArgParser& argParser, AbstractLayerEditorWindow* layerEditor);
};

} // namespace MAYAUSD_NS_DEF

#endif // LAYEREDITORWINDOWCOMMAND_H
