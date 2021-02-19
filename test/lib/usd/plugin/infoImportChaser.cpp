//
// Copyright 2021 Apple
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
/**
 * @brief  This is a sample that demonstrates how to write an import chaser.
 *         It is essentially a no-op that prints out details about the paths
 *         imported, and the Maya DG nodes that were created, along with details
 *         about the import job itself.
 *
 *         Sample import command:
 *         cmds.mayaUSDImport(
 *             file='/tmp/test.usda',
 *             chaser=['info'])
 */
#include <mayaUsd/fileio/chaser/importChaserRegistry.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/primRange.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

class InfoImportChaser : public UsdMayaImportChaser
{
public:
    InfoImportChaser() { }

    virtual ~InfoImportChaser() override { }

    virtual bool PostImport(
        Usd_PrimFlagsPredicate&     returnPredicate,
        const UsdStagePtr&          stage,
        const MDagPathArray&        dagPaths,
        const SdfPathVector&        sdfPaths,
        const UsdMayaJobImportArgs& jobArgs) override
    {
        MString sdfPathsStr = "SdfPaths imported: ";
        for (size_t i = 0; i < sdfPaths.size(); ++i) {
            sdfPathsStr += MString(sdfPaths[i].GetString().c_str()) + "\n";
        }

        MString stageTraverseStr = "Stage traversal: ";
        for (const UsdPrim prim : stage->TraverseAll()) {
            stageTraverseStr += MString(prim.GetName().GetText()) + "\n";
        }

        VtDictionary customLayerData = stage->GetRootLayer()->GetCustomLayerData();
        MString      customLayerDataStr = "Custom layer data: ";
        for (auto it = customLayerData.begin(); it != customLayerData.end(); ++it) {
            if (it->second.IsHolding<std::string>()) {
                std::string secondStr = it->second.Get<std::string>();
                customLayerDataStr
                    += MString(it->first.c_str()) + MString(secondStr.c_str()) + "\n";
            } else {
                customLayerDataStr += MString(it->first.c_str())
                    + MString(it->second.GetTypeName().c_str()) + "\n";
            }
        }

        MGlobal::displayInfo(
            "Info from import:\n" + sdfPathsStr + stageTraverseStr + customLayerDataStr);

        // NOTE: (yliangsiew) Just for the sake of having something that we can actually run
        // unit tests against, we'll add a custom attribute to the root DAG paths imported
        // so that we can verify that the import chaser is working, since we can't easily
        // parse Maya Script Editor output.
        MStatus stat;
        for (unsigned int i = 0; i < dagPaths.length(); ++i) {
            MDagPath      curDagPath = dagPaths[i];
            MFnStringData fnStrData;
            MObject       defaultStr = fnStrData.create(customLayerDataStr, &stat);
            CHECK_MSTATUS_AND_RETURN(stat, false);
            MFnTypedAttribute fnTypedAttr;
            MObject           strAttr = fnTypedAttr.create(
                "customData", "customData", MFnData::kString, defaultStr, &stat);
            CHECK_MSTATUS_AND_RETURN(stat, false);
            MFnDagNode fnDagNode(curDagPath, &stat);
            CHECK_MSTATUS_AND_RETURN(stat, false);
            stat = fnDagNode.addAttribute(strAttr);
            CHECK_MSTATUS_AND_RETURN(stat, false);

            this->nodesEditedRecord.append(fnDagNode.object());
            this->editsRecord.append(strAttr);
        }
        return true;
    }

    virtual bool Redo() override
    {
        undoRecord.undoIt(); // NOTE: (yliangsiew) Undo the undo to re-do.
        return true;
    }

    virtual bool Undo() override
    {
        for (unsigned int i = 0; i < editsRecord.length(); ++i) {
            MObject nodeEdited = nodesEditedRecord[i];
            // TODO: (yliangsiew) This seems like a bit of a code smell...why would this crash
            // otherwise? But for now, this guards an undo-redo chain crash where the MObject is no
            // longer valid between invocations. Need to look at this further.
            if (!MObjectHandle(nodeEdited).isValid()) {
                continue;
            }
            MObject attrToDelete = editsRecord[i];
            if (!MObjectHandle(attrToDelete).isValid()) {
                continue;
            }
            undoRecord.removeAttribute(nodesEditedRecord[i], editsRecord[i]);
        }
        undoRecord.doIt();
        return true;
    }

    MDGModifier  undoRecord;
    MObjectArray editsRecord;
    MObjectArray nodesEditedRecord;
};

USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(info, ctx)
{
    std::map<std::string, std::string> myArgs;
    TfMapLookup(ctx.GetImportJobArgs().allChaserArgs, "info", &myArgs);

    // TODO: (yliangsiew) Figure out why the chaser isn't accessible, or the alembic example chaser
    // as well in a normal Maya session. Needs to have pxrUsd loaded first _then_ mayaUsdPlugin, in
    // that order. Why other way round doesn't work?
    return new InfoImportChaser();
}

PXR_NAMESPACE_CLOSE_SCOPE
