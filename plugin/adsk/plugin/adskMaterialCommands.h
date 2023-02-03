//
// Copyright 2023 Autodesk
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
#pragma once

#ifndef ADSK_MAYA_MATERIAL_COMMANDS_H
#define ADSK_MAYA_MATERIAL_COMMANDS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/mayaUsd.h>
#include <mayaUsd/undo/OpUndoItemList.h>

#include <maya/MPxCommand.h>
#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// GetMaterialXMaterialsCommand
//------------------------------------------------------------------------------

struct MxShaderMenuEntry
{
    MxShaderMenuEntry(const std::string& label, const std::string& identifier)
        : _label(label)
        , _identifier(identifier)
    {
    }
    const std::string  _label;
    const std::string& _identifier;
};
typedef std::vector<MxShaderMenuEntry> MxShaderMenuEntryVec;

//! \brief Fills the currently active menu with submenus listing available materials from different
//! renderers. 
//! \todo: The list of materials and renderers is currently hard-coded. We need to make
//! it dynamic so that third-party renderers can hook in to provide their own materials.
class MAYAUSD_CORE_PUBLIC ADSKMayaUSDGetMaterialsForRenderersCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const char commandName[];
    static void*      creator();
    static MSyntax    createSyntax();

    MStatus doIt(const MArgList& argList) override;
    bool isUndoable() const override
    {
        return false;
    }

private:
    MStatus parseArgs(const MArgList& argList);
    
    void appendMaterialXMaterials() const;
    void appendArnoldMaterials() const;
    void appendUsdMaterials() const;
};

//! \brief Fills the currently active menu with the available materials existing in the given
//! object's stage.
class MAYAUSD_CORE_PUBLIC ADSKMayaUSDGetMaterialsInStageCommand : public MPxCommand
{
    static constexpr auto kSceneItem = "si";
    static constexpr auto kSceneItemLong = "sceneItem";

public:
    // plugin registration requirements
    static const char commandName[];
    static void*      creator();
    static MSyntax    createSyntax();

    MStatus doIt(const MArgList& argList) override;
    bool isUndoable() const override
    {
        return false;
    }

private:
    MStatus parseArgs(const MArgList& argList);
};

} // namespace MAYAUSD_NS_DEF

#endif /* ADSK_MAYA_MATERIAL_COMMANDS_H */
