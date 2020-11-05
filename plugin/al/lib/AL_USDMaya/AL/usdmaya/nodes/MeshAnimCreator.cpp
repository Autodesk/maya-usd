//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/nodes/MeshAnimCreator.h"

#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsd/nodes/stageData.h>

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFnMeshData.h>
#include <maya/MTime.h>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(MeshAnimCreator, MTypeId(0x696A), AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MObject MeshAnimCreator::m_primPath = MObject::kNullObj;
MObject MeshAnimCreator::m_inTime = MObject::kNullObj;
MObject MeshAnimCreator::m_inStageData = MObject::kNullObj;
MObject MeshAnimCreator::m_outMesh = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
MStatus MeshAnimCreator::initialise()
{
    try {
        setNodeType(kTypeName);
        addFrame("Mesh Animation Createor");

        // do not write these nodes to the file. They will be created automagically by the proxy
        // shape
        m_primPath = addStringAttr("primPath", "pp", kReadable | kWritable);
        m_inTime = addTimeAttr(
            "inTime", "it", MTime(), kReadable | kWritable | kStorable | kConnectable);
        m_inStageData = addDataAttr(
            "inStageData",
            "isd",
            MayaUsdStageData::mayaTypeId,
            kWritable | kStorable | kConnectable);
        m_outMesh = addMeshAttr("outMesh", "out", kReadable | kStorable | kConnectable);
        attributeAffects(m_primPath, m_outMesh);
        attributeAffects(m_inTime, m_outMesh);
        attributeAffects(m_inStageData, m_outMesh);
    } catch (const MStatus& status) {
        return status;
    }
    generateAETemplate();
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MeshAnimCreator::compute(const MPlug& plug, MDataBlock& data)
{
    TF_DEBUG(ALUSDMAYA_GEOMETRY_DEFORMER)
        .Msg("MeshAnimCreator::compute ==> %s\n", plug.name().asChar());

    MStatus status = MS::kInvalidParameter;
    if (plug != m_outMesh) {
        return status;
    }

    MTime       inTimeVal = inputTimeValue(data, m_inTime);
    UsdTimeCode usdTime(inTimeVal.value());

    MDataHandle    outputHandle = data.outputValue(m_outMesh, &status);
    UsdStageRefPtr stage = getStage();
    if (stage) {
        UsdPrim     prim = stage->GetPrimAtPath(m_cachePath);
        UsdGeomMesh mesh(prim);

        MFnMeshData fnData;
        MObject     obj = fnData.create();

        utils::MeshImportContext context(mesh, obj, MString(), usdTime);
        context.applyHoleFaces();
        context.applyVertexNormals();
        context.applyEdgeCreases();
        context.applyVertexCreases();
        context.applyUVs();
        context.applyColourSetData();
        outputHandle.set(obj);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MeshAnimCreator::connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
{
    TF_DEBUG(ALUSDMAYA_GEOMETRY_DEFORMER).Msg("MeshAnimCreator::connectionMade\n");
    if (!asSrc && plug == m_inStageData) {
        MFnDependencyNode otherNode(otherPlug.node());
        if (otherNode.typeId() == ProxyShape::kTypeId) {
            proxyShapeHandle = otherPlug.node();
        }
    }
    return MPxNode::connectionMade(plug, otherPlug, asSrc);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MeshAnimCreator::connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
{
    TF_DEBUG(ALUSDMAYA_GEOMETRY_DEFORMER).Msg("MeshAnimCreator::connectionBroken\n");
    if (!asSrc && plug == m_inStageData) {
        MFnDependencyNode otherNode(otherPlug.node());
        if (otherNode.typeId() == ProxyShape::kTypeId) {
            proxyShapeHandle = MObject();
        }
    }
    return MPxNode::connectionBroken(plug, otherPlug, asSrc);
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr MeshAnimCreator::getStage()
{
    TF_DEBUG(ALUSDMAYA_GEOMETRY_DEFORMER).Msg("MeshAnimCreator::getStage\n");
    if (proxyShapeHandle.isValid() && proxyShapeHandle.isAlive()) {
        MFnDependencyNode fn(proxyShapeHandle.object());
        ProxyShape*       node = (ProxyShape*)fn.userNode();
        if (node) {
            return node->usdStage();
        }
    }
    return UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
void MeshAnimCreator::postConstructor()
{
    auto obj = thisMObject();
    m_attributeChanged
        = MNodeMessage::addAttributeChangedCallback(obj, onAttributeChanged, (void*)this);
}

//----------------------------------------------------------------------------------------------------------------------
void MeshAnimCreator::onAttributeChanged(
    MNodeMessage::AttributeMessage msg,
    MPlug&                         plug,
    MPlug&,
    void* clientData)
{
    TF_DEBUG(ALUSDMAYA_GEOMETRY_DEFORMER).Msg("MeshAnimCreator::onAttributeChanged\n");
    MeshAnimCreator* deformer = (MeshAnimCreator*)clientData;
    if (msg & MNodeMessage::kAttributeSet) {
        if (plug == m_primPath) {
            // Get the prim
            // If no primPath string specified, then use the pseudo-root.
            MString primPathStr = plug.asString();
            if (primPathStr.length()) {
                deformer->m_cachePath = SdfPath(AL::maya::utils::convert(primPathStr));
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
