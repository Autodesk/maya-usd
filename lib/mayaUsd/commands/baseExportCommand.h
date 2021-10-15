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
#ifndef MAYA_EXPORT_COMMAND_H
#define MAYA_EXPORT_COMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/writeJob.h>

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

#include <memory>

namespace MAYAUSD_NS_DEF {

class MAYAUSD_CORE_PUBLIC MayaUSDExportCommand : public MPxCommand
{
public:
    //
    // Command flags are a mix of Arg Tokens defined in writeJob.h
    // and some that are defined by this command itself.
    // All short forms of the Maya flag names are defined here.
    // All long forms of flags defined by the command are also here.
    // All long forms of flags defined by the Arg Tokens are queried
    // for and set when creating the MSyntax object.
    // Derived classes can use the short forms of the flags when
    // calling Maya functions like argData.isFlagSet()
    //
    // The list of short forms of flags defined as Arg Tokens:
    static constexpr auto kDefaultMeshSchemeFlag = "dms";
    static constexpr auto kDefaultUSDFormatFlag = "duf";
    static constexpr auto kExportColorSetsFlag = "cls";
    static constexpr auto kExportUVsFlag = "uvs";
    static constexpr auto kEulerFilterFlag = "ef";
    static constexpr auto kExportVisibilityFlag = "vis";
    static constexpr auto kIgnoreWarningsFlag = "ign";
    static constexpr auto kExportInstancesFlag = "ein";
    static constexpr auto kMergeTransformAndShapeFlag = "mt";
    static constexpr auto kStripNamespacesFlag = "sn";
    static constexpr auto kExportRefsAsInstanceableFlag = "eri";
    static constexpr auto kExportDisplayColorFlag = "dsp";
    static constexpr auto kShadingModeFlag = "shd";
    static constexpr auto kConvertMaterialsToFlag = "cmt";
    static constexpr auto kMaterialsScopeNameFlag = "msn";
    static constexpr auto kExportMaterialCollectionsFlag = "mcs";
    static constexpr auto kMaterialCollectionsPathFlag = "mcp";
    static constexpr auto kExportCollectionBasedBindingsFlag = "cbb";
    static constexpr auto kNormalizeNurbsFlag = "nnu";
    static constexpr auto kExportReferenceObjectsFlag = "ero";
    static constexpr auto kExportRootsFlag = "ert";
    static constexpr auto kExportSkelsFlag = "skl";
    static constexpr auto kExportSkinFlag = "skn";
    static constexpr auto kExportBlendShapesFlag = "ebs";
    static constexpr auto kParentScopeFlag = "psc";
    static constexpr auto kRenderableOnlyFlag = "ro";
    static constexpr auto kDefaultCamerasFlag = "dc";
    static constexpr auto kRenderLayerModeFlag = "rlm";
    static constexpr auto kKindFlag = "k";
    static constexpr auto kCompatibilityFlag = "com";
    static constexpr auto kChaserFlag = "chr";
    static constexpr auto kChaserArgsFlag = "cha";
    static constexpr auto kMelPerFrameCallbackFlag = "mfc";
    static constexpr auto kMelPostCallbackFlag = "mpc";
    static constexpr auto kPythonPerFrameCallbackFlag = "pfc";
    static constexpr auto kPythonPostCallbackFlag = "ppc";
    static constexpr auto kVerboseFlag = "v";
    static constexpr auto kStaticSingleSample = "sss";
    static constexpr auto kGeomSidednessFlag = "gs";
    static constexpr auto kApiSchemaFlag = "api";
    static constexpr auto kJobContextFlag = "jc";

    // Short and Long forms of flags defined by this command itself:
    static constexpr auto kAppendFlag = "a";
    static constexpr auto kAppendFlagLong = "append";
    static constexpr auto kFilterTypesFlag = "ft";
    static constexpr auto kFilterTypesFlagLong = "filterTypes";
    static constexpr auto kFileFlag = "f";
    static constexpr auto kFileFlagLong = "file";
    static constexpr auto kSelectionFlag = "sl";
    static constexpr auto kSelectionFlagLong = "selection";
    static constexpr auto kFrameSampleFlag = "fs";
    static constexpr auto kFrameSampleFlagLong = "frameSample";
    static constexpr auto kFrameStrideFlag = "fst";
    static constexpr auto kFrameStrideFlagLong = "frameStride";
    static constexpr auto kFrameRangeFlag = "fr";
    static constexpr auto kFrameRangeFlagLong = "frameRange";

    MStatus doIt(const MArgList& args) override;
    bool    isUndoable() const override { return false; };

    static MSyntax createSyntax();
    static void*   creator();

protected:
    virtual std::unique_ptr<PXR_NS::UsdMaya_WriteJob>
    initializeWriteJob(const PXR_NS::UsdMayaJobExportArgs&);
};

} // namespace MAYAUSD_NS_DEF

#endif
