//
// Copyright 2016 Pixar
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
#ifndef MAYA_IMPORT_COMMAND_H
#define MAYA_IMPORT_COMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/readJob.h>

#include <maya/MPxCommand.h>

#include <memory>

namespace MAYAUSD_NS_DEF {

class MAYAUSD_CORE_PUBLIC MayaUSDImportCommand : public MPxCommand
{
public:
    //
    // Command flags are a mix of Arg Tokens defined in readJob.h
    // and some that are defined by this command itself.
    // All short forms of the Maya flag names are defined here.
    // All long forms of flags defined by the command are also here.
    // All long forms of flags defined by the Arg Tokens are queried
    // for and set when creating the MSyntax object.
    // Derived classes can use the short forms of the flags when
    // calling Maya functions like argData.isFlagSet()
    //
    // The list of short forms of flags defined as Arg Tokens:
    static constexpr auto kShadingModeFlag = "shd";
    static constexpr auto kPreferredMaterialFlag = "prm";
    static constexpr auto kImportInstancesFlag = "ii";
    static constexpr auto kImportUSDZTexturesFlag = "itx";
    static constexpr auto kImportUSDZTexturesFilePathFlag = "itf";
    static constexpr auto kMetadataFlag = "md";
    static constexpr auto kApiSchemaFlag = "api";
    static constexpr auto kJobContextFlag = "jc";
    static constexpr auto kExcludePrimvarFlag = "epv";
    static constexpr auto kUseAsAnimationCacheFlag = "uac";
    static constexpr auto kImportChaserFlag = "chr";
    static constexpr auto kImportChaserArgsFlag = "cha";

    // Short and Long forms of flags defined by this command itself:
    static constexpr auto kFileFlag = "f";
    static constexpr auto kFileFlagLong = "file";
    static constexpr auto kParentFlag = "p";
    static constexpr auto kParentFlagLong = "parent";
    static constexpr auto kReadAnimDataFlag = "ani";
    static constexpr auto kReadAnimDataFlagLong = "readAnimData";
    static constexpr auto kFrameRangeFlag = "fr";
    static constexpr auto kFrameRangeFlagLong = "frameRange";
    static constexpr auto kPrimPathFlag = "pp";
    static constexpr auto kPrimPathFlagLong = "primPath";
    static constexpr auto kVariantFlag = "var";
    static constexpr auto kVariantFlagLong = "variant";
    static constexpr auto kVerboseFlag = "v";
    static constexpr auto kVerboseFlagLong = "verbose";

    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
    bool    isUndoable() const override { return true; };

    static MSyntax createSyntax();
    static void*   creator();

protected:
    virtual std::unique_ptr<UsdMaya_ReadJob>
    initializeReadJob(const MayaUsd::ImportData&, const UsdMayaJobImportArgs&);

private:
    std::unique_ptr<UsdMaya_ReadJob> _readJob;
};

} // namespace MAYAUSD_NS_DEF

#endif
