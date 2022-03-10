//
// Copyright 2022 Autodesk
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
#include "geomNode.h"

#include <mayaUsd/fileio/jobs/meshDataReadJob.h>
#include <mayaUsd/fileio/jobs/readJob.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MObject.h>

namespace MAYAUSD_NS_DEF {
// ========================================================

const MTypeId MAYAUSD_GEOMNODE_ID(0x58000099);

const MTypeId MayaUsdGeomNode::typeId(MayaUsd::MAYAUSD_GEOMNODE_ID);
const MString MayaUsdGeomNode::typeName("mayaUsdGeomNode");

// Attributes
MObject MayaUsdGeomNode::filePathAttr;
MObject MayaUsdGeomNode::rootPrimAttr;

// Output attributes
MObject MayaUsdGeomNode::outGeomAttr;
MObject MayaUsdGeomNode::outGeomMatrixAttr;

namespace {
// Utility class to simplify outputing attribute arrays.
class OutputArrayHandler
{
public:
    OutputArrayHandler(MDataBlock& dataBlock, const MObject& attribute, int size, MStatus& retValue)
        : mArrayDataHandle(dataBlock.outputValue(attribute, &retValue))
        , mBuilder(&dataBlock, attribute, size)
    {
    }

    template <typename T> void add(const T& item)
    {
        MDataHandle element = mBuilder.addLast();
        element.set(item);
    }

    void finish() { mArrayDataHandle.set(mBuilder); }

private:
    MArrayDataHandle  mArrayDataHandle;
    MArrayDataBuilder mBuilder;
};
} // namespace

struct MayaUsdGeomNode::CacheData
{
    std::string                                loadedFile;
    std::string                                rootPrim;
    std::vector<UsdMaya_MeshDataReadJob::Data> primitives;
};

/* static */
void* MayaUsdGeomNode::creator() { return new MayaUsdGeomNode(); }

/* static */
MStatus MayaUsdGeomNode::initialize()
{
    MStatus retValue = MS::kSuccess;

    MFnTypedAttribute    typedAttrFn;
    MFnCompoundAttribute compAttrFn;

    // Inputs
    filePathAttr
        = typedAttrFn.create("filePath", "fp", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setWritable(true);
    typedAttrFn.setReadable(false);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(filePathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    rootPrimAttr
        = typedAttrFn.create("rootPrim", "rp", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setWritable(true);
    typedAttrFn.setReadable(false);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(rootPrimAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Outputs
    outGeomAttr
        = typedAttrFn.create("geometry", "geo", MFnData::kMesh, MObject::kNullObj, &retValue);
    typedAttrFn.setWritable(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setArray(true);
    typedAttrFn.setUsesArrayDataBuilder(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(outGeomAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outGeomMatrixAttr
        = typedAttrFn.create("matrix", "tra", MFnData::kMatrix, MObject::kNullObj, &retValue);
    typedAttrFn.setWritable(false);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setArray(true);
    typedAttrFn.setUsesArrayDataBuilder(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(outGeomMatrixAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(filePathAttr, outGeomAttr);
    retValue = attributeAffects(rootPrimAttr, outGeomAttr);
    retValue = attributeAffects(filePathAttr, outGeomMatrixAttr);
    retValue = attributeAffects(rootPrimAttr, outGeomMatrixAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

MayaUsdGeomNode::MayaUsdGeomNode()
    : MPxNode()
    , _cache(std::make_unique<CacheData>())
{
}

/* virtual */
MayaUsdGeomNode::~MayaUsdGeomNode() { }

/* virtual */
MStatus MayaUsdGeomNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    if (plug == outGeomAttr || plug == outGeomMatrixAttr) {
        MDataHandle filePathHandle = dataBlock.inputValue(filePathAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        MDataHandle rootPrimHandle = dataBlock.inputValue(rootPrimAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        const std::string fileName = TfStringTrim(filePathHandle.asString().asChar());
        const std::string rootPrim = TfStringTrim(rootPrimHandle.asString().asChar());

        if (!fileName.empty()) {
            if (_cache->loadedFile.empty() || _cache->loadedFile != fileName
                || _cache->rootPrim != rootPrim) {
                MayaUsd::ImportData readData;
                readData.setFilename(fileName);

                if (!rootPrim.empty()) {
                    // Set the root prim to import from
                    readData.setRootPrimPath(rootPrim);
                }

                VtDictionary userArgs;
                GfInterval   timeInterval;

                UsdMayaJobImportArgs jobArgs = UsdMayaJobImportArgs::CreateFromDictionary(
                    userArgs,
                    /* importWithProxyShapes = */ false,
                    timeInterval);

                UsdMaya_MeshDataReadJob reader(readData, jobArgs);

                std::vector<MDagPath> addedDagPaths;
                reader.Read(&addedDagPaths);

                _cache->primitives.swap(reader.mMeshData);
                _cache->loadedFile = fileName;
                _cache->rootPrim = rootPrim;
            }

            OutputArrayHandler geomOut(dataBlock, outGeomAttr, _cache->primitives.size(), retValue);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            OutputArrayHandler matrixOut(
                dataBlock, outGeomMatrixAttr, _cache->primitives.size(), retValue);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            for (auto& prim : _cache->primitives) {
                geomOut.add(prim.geometry);
                matrixOut.add(prim.matrix);
            }

            geomOut.finish();
            matrixOut.finish();
        }
    }

    return retValue;
}

} // namespace MAYAUSD_NS_DEF
