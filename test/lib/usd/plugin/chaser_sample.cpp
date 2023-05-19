#include <mayaUsd/fileio/chaser/importChaser.h>
#include <mayaUsd/fileio/chaser/importChaserRegistry.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>


///
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/primRange.h>

//
#include <mayaUsd/fileio/jobs/jobArgs.h>

PXR_NAMESPACE_USING_DIRECTIVE

class VRayUsdMayaImportChaser : public UsdMayaImportChaser
{
public:
    bool PostImport(
        Usd_PrimFlagsPredicate&     returnPredicate,
        const UsdStagePtr&          stage,
        const MDagPathArray&        dagPaths,
        const SdfPathVector&        sdfPaths,
        const UsdMayaJobImportArgs& jobArgs) override
    {

        int numPaths = (int)sdfPaths.size();
        //int numDagPaths = dagPaths.length();
        assert(numPaths == (int)dagPaths.length());
        //assert(numPaths == numDagPaths);
        for (int i = 0; i < numPaths; ++i) {
            SdfPath  sdfPath = sdfPaths[(size_t)i];
            MDagPath dagPath = dagPaths[(unsigned)i];
            UsdPrim  prim = stage->GetPrimAtPath(sdfPath);
            MGlobal::displayInfo(MString("----------primitive ") + i);
            MGlobal::displayInfo(sdfPath.GetString().c_str());
            MGlobal::displayInfo("\n");
            MGlobal::displayInfo(dagPath.fullPathName());
            MGlobal::displayInfo("\n");
            MGlobal::displayInfo(prim.GetTypeName().GetText());
            MGlobal::displayInfo("\n");
        }
        MGlobal::displayInfo("Stage Traversal: \n");
        MGlobal::displayInfo("Lets reach the leaf child: \n");

        SdfPathSet    sdfImportedPaths;
        MDagPathArray myDagArray;
        SdfPathVector mySdfPaths;

        // collect all SDF Paths
        for (const UsdPrim prim : stage->TraverseAll()) {

            if (!prim.GetChildren()) {
                // traverse to the leaf child
                sdfImportedPaths.insert(prim.GetPath());
            }
        }

        for (const auto& p : sdfToDagMap) {
            mySdfPaths.push_back(p.first);
            myDagArray.append(p.second);
        }

        // find the matching sdf and DAG path sets
        /*
        TF_FOR_ALL(pathsIter, sdfImportedPaths)
        {
            std::string key = pathsIter->GetString();
            MObject     obj;
            if (TfMapLookup(mNewNodeRegistry, key, &obj)) {
                if (obj.hasFn(MFn::kDagNode)) {
                    myDagArray.append(MDagPath::getAPathTo(obj));
                    mySdfPaths.push_back(pathsIter->GetPrimPath());
                }
            }
        }
        */
        int n = (int)mySdfPaths.size();
        assert(n == (int)myDagArray.length());

        for (int i = 0; i < n; ++i) {
            SdfPath  sdfPath = mySdfPaths[(size_t)i];
            MDagPath dagPath = myDagArray[(unsigned)i];
            UsdPrim  prim = stage->GetPrimAtPath(sdfPath);
            MGlobal::displayInfo(MString("----------primitive ") + i);
            MGlobal::displayInfo(sdfPath.GetString().c_str());
            MGlobal::displayInfo("\n");
            MGlobal::displayInfo(dagPath.fullPathName());
            MGlobal::displayInfo("\n");
            MGlobal::displayInfo(prim.GetTypeName().GetText());
            MGlobal::displayInfo("\n");
        }


        return true;
    }
};

USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(vray, ctx) { return new VRayUsdMayaImportChaser; }
