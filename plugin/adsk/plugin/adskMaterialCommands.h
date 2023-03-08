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
#ifndef ADSK_MAYA_MATERIAL_COMMANDS_H
#define ADSK_MAYA_MATERIAL_COMMANDS_H

#include "base/api.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/mayaUsd.h>

#include <maya/MPxCommand.h>
#include <ufe/path.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// GetMaterialXMaterialsCommand
//------------------------------------------------------------------------------

//! \brief Returns an array of strings containing materials associated with a given renderer.The
//! strings are in the format: "Renderer Name/Material Label|MaterialIdentifier" e.g. "Arnold/AI
//! Standard Surface|arnold:standard_surface" The main intention is for the returned strings to be
//! split in order to populate menu entries. \todo: The list of materials and renderers is currently
//! hard-coded. We need to make it dynamic so that third-party renderers can hook in to provide
//! their own materials.
class MAYAUSD_PLUGIN_PUBLIC ADSKMayaUSDGetMaterialsForRenderersCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const MString commandName;
    static void*         creator();
    static MSyntax       createSyntax();

    MStatus doIt(const MArgList& argList) override;
    bool    isUndoable() const override { return false; }

private:
    MStatus parseArgs(const MArgList& argList);

    void appendMaterialXMaterials() const;
    void appendArnoldMaterials() const;
    void appendUsdMaterials() const;
};

//! \brief Returns an array of materials in the same stage as the object passed in via argument.
//! The returned strings are simply paths to a material.
class MAYAUSD_PLUGIN_PUBLIC ADSKMayaUSDGetMaterialsInStageCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const MString commandName;
    static void*         creator();
    static MSyntax       createSyntax();

    MStatus doIt(const MArgList& argList) override;
    bool    isUndoable() const override { return false; }

private:
    MStatus parseArgs(const MArgList& argList);
};

} // namespace MAYAUSD_NS_DEF

#endif /* ADSK_MAYA_MATERIAL_COMMANDS_H */
