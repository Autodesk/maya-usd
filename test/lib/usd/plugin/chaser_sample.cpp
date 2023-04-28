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

PXR_NAMESPACE_USING_DIRECTIVE

class VRayUsdMayaImportChaser : public UsdMayaImportChaser {
public:
    bool PostImport(
		Usd_PrimFlagsPredicate& returnPredicate,
		const UsdStagePtr& stage,
		const MDagPathArray& dagPaths,
		const SdfPathVector& sdfPaths,
		const UsdMayaJobImportArgs& jobArgs
	) override {

		int numPaths = (int)sdfPaths.size();
        int numDagPaths = dagPaths.length();
		//assert(numPaths == (int)dagPaths.length());
        assert(numPaths == numDagPaths);
		for (int i = 0; i < numPaths; ++i) {
			SdfPath sdfPath = sdfPaths[(size_t)i];
			MDagPath dagPath = dagPaths[(unsigned)i];
			UsdPrim prim = stage->GetPrimAtPath(sdfPath);
			MGlobal::displayInfo(MString("----------primitive ") + i );
			MGlobal::displayInfo(sdfPath.GetString().c_str());
			MGlobal::displayInfo("\n");
			MGlobal::displayInfo(dagPath.fullPathName());
			MGlobal::displayInfo("\n");
			MGlobal::displayInfo(prim.GetTypeName().GetText());
			MGlobal::displayInfo("\n");
		}
		MGlobal::displayInfo("Stage Traversal: \n");
        for (const UsdPrim prim : stage->TraverseAll()) {
            MGlobal::displayInfo(MString(prim.GetName().GetText()) + "\n");
        }
		return true;
	}
};

USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(vray, ctx) {
	return new VRayUsdMayaImportChaser;
}
