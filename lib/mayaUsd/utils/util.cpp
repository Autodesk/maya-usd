//
// Copyright 2016 Pixar
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
#include "util.h"

#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MBoundingBox.h>
#include <maya/MColor.h>
#include <maya/MDGModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnExpression.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTime.h>

#include <boost/functional/hash.hpp>

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if MAYA_API_VERSION >= 20200000
#include <maya/MFnStandardSurfaceShader.h>
#endif

#include <mayaUsd/utils/colorSpace.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/js/json.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
bool shouldAddToSet(const MDagPath& toAdd, const UsdMayaUtil::MDagPathSet& dagPaths)
// Utility function to check if an object should be added to the set of objects to
// export.  An object should not be added if it's invalid, or if any of it's parent
// objects are already in the set.
{
    if (!toAdd.isValid())
        return false;

    MStatus  status = MS::kSuccess;
    bool     pathIsValid = true;
    MDagPath dp = toAdd;

    UsdMayaUtil::MDagPathSet::const_iterator end = dagPaths.end();

    // Travel up the hierarchy looking for a parent object that is already in the set.
    // That is our only reason to return false.  Not finding any ancestors in the set
    // will eventually hit the world root, which will be an invalid path and in that case
    // we just exit the loop and return true.
    while (pathIsValid && status == MS::kSuccess) {
        UsdMayaUtil::MDagPathSet::const_iterator dpIter = dagPaths.find(dp);
        if (dpIter != end)
            return false;

        status = dp.pop();
        pathIsValid = dp.isValid();
    }

    return true;
}
} // namespace

double UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(const MDistance::Unit mdistanceUnit)
{
    switch (mdistanceUnit) {
    case MDistance::kInches: return UsdGeomLinearUnits::inches;
    case MDistance::kFeet: return UsdGeomLinearUnits::feet;
    case MDistance::kYards: return UsdGeomLinearUnits::yards;
    case MDistance::kMiles: return UsdGeomLinearUnits::miles;
    case MDistance::kMillimeters: return UsdGeomLinearUnits::millimeters;
    case MDistance::kCentimeters: return UsdGeomLinearUnits::centimeters;
    case MDistance::kKilometers: return UsdGeomLinearUnits::kilometers;
    case MDistance::kMeters: return UsdGeomLinearUnits::meters;
    default:
        TF_CODING_ERROR("Invalid MDistance unit %d. Assuming centimeters", mdistanceUnit);
        return UsdGeomLinearUnits::centimeters;
    }
}

MDistance::Unit UsdMayaUtil::ConvertUsdGeomLinearUnitToMDistanceUnit(const double linearUnit)
{
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::millimeters)) {
        return MDistance::kMillimeters;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::centimeters)) {
        return MDistance::kCentimeters;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::meters)) {
        return MDistance::kMeters;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::kilometers)) {
        return MDistance::kKilometers;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::inches)) {
        return MDistance::kInches;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::feet)) {
        return MDistance::kFeet;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::yards)) {
        return MDistance::kYards;
    }
    if (UsdGeomLinearUnitsAre(linearUnit, UsdGeomLinearUnits::miles)) {
        return MDistance::kMiles;
    }

    TF_CODING_ERROR("Invalid UsdGeomLinearUnit %f. Assuming centimeters", linearUnit);
    return MDistance::kCentimeters;
}

std::string UsdMayaUtil::GetMayaNodeName(const MObject& mayaNode)
{
    MString nodeName;
    MStatus status;

    // All DAG nodes are also DG nodes, so try it as a DG node first.
    const MFnDependencyNode depNodeFn(mayaNode, &status);
    if (status == MS::kSuccess) {
        const MString depName = depNodeFn.absoluteName(&status);
        if (status == MS::kSuccess) {
            nodeName = depName;
        }
    }

    // Overwrite the DG name if we find that it's a DAG node.
    const MFnDagNode dagNodeFn(mayaNode, &status);
    if (status == MS::kSuccess) {
        const MString dagName = dagNodeFn.fullPathName(&status);
        if (status == MS::kSuccess) {
            nodeName = dagName;
        }
    }

    return nodeName.asChar();
}

MString UsdMayaUtil::GetUniqueNameOfDAGNode(const MObject& node)
{
    if (!TF_VERIFY(!node.isNull() && node.hasFn(MFn::kDagNode))) {
        return MString();
    }
    MStatus    stat;
    MFnDagNode fnNode(node, &stat);
    CHECK_MSTATUS_AND_RETURN(stat, MString());
    MString nodeName = fnNode.partialPathName(&stat);
    return nodeName;
}

MStatus UsdMayaUtil::GetMObjectByName(const std::string& nodeName, MObject& mObj)
{
    MSelectionList selectionList;
    MStatus        status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDependNode(0, mObj);

    return status;
}

MStatus UsdMayaUtil::GetDagPathByName(const std::string& nodeName, MDagPath& dagPath)
{
    MSelectionList selectionList;
    MStatus        status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDagPath(0, dagPath);

    return status;
}

MStatus UsdMayaUtil::GetPlugByName(const std::string& attrPath, MPlug& plug)
{
    std::vector<std::string> comps = TfStringSplit(attrPath, ".");
    if (comps.size() != 2) {
        TF_RUNTIME_ERROR("'%s' is not a valid Maya attribute path", attrPath.c_str());
        return MStatus::kFailure;
    }

    MObject object;
    MStatus status = GetMObjectByName(comps[0], object);
    if (!status) {
        return status;
    }

    MFnDependencyNode depNode(object, &status);
    if (!status) {
        return status;
    }

    MPlug tmpPlug = depNode.findPlug(comps[1].c_str(), true, &status);
    if (!status) {
        return status;
    }

    plug = tmpPlug;
    return status;
}

MPlug UsdMayaUtil::FindChildPlugWithName(const MPlug& parent, const MString& name)
{
    MPlug sentinel;
    if (parent.isNull() || !parent.isCompound()) {
        return sentinel;
    }
    MStatus      stat;
    unsigned int numChildren = parent.numChildren(&stat);
    CHECK_MSTATUS_AND_RETURN(stat, sentinel);
    if (numChildren == 0) {
        return sentinel;
    }

    MFnAttribute fnAttr;

    // TODO: (yliangsiew) for a certain threshold of child plugs, might want to
    //       binary search instead.
    for (unsigned int i = 0; i < numChildren; ++i) {
        MPlug plgChild = parent.child(i, &stat);
        CHECK_MSTATUS_AND_RETURN(stat, sentinel);
        MObject attrChild = plgChild.attribute(&stat);
        CHECK_MSTATUS_AND_RETURN(stat, sentinel);
        stat = fnAttr.setObject(attrChild);
        CHECK_MSTATUS_AND_RETURN(stat, sentinel);
        const MString attrName = fnAttr.name();
        if (attrName == name) {
            return plgChild;
        }
    }

    return sentinel;
}

MPlug UsdMayaUtil::GetMayaTimePlug()
{
    MPlug   timePlug;
    MStatus status;

    // As an extra sanity check, we only return a discovered plug if its
    // value matches the current time.
    MTime curTime = MAnimControl::currentTime();

    MItDependencyNodes iter(MFn::kTime, &status);
    CHECK_MSTATUS_AND_RETURN(status, timePlug);

    while (!timePlug && !iter.isDone()) {
        MObject node = iter.thisNode();
        iter.next();

        MFnDependencyNode depNodeFn(node, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MPlug outTimePlug = depNodeFn.findPlug("outTime", true, &status);
        if (status != MS::kSuccess || !outTimePlug) {
            continue;
        }

        if (outTimePlug.asMTime() != curTime) {
            continue;
        }

        timePlug = outTimePlug;
    }

    return timePlug;
}

MPlug UsdMayaUtil::GetMayaShaderListPlug()
{
    MPlug   shadersPlug;
    MStatus status;

    MItDependencyNodes iter(MFn::kShaderList, &status);
    CHECK_MSTATUS_AND_RETURN(status, shadersPlug);

    while (!shadersPlug && !iter.isDone()) {
        MObject node = iter.thisNode();
        iter.next();

        MFnDependencyNode depNodeFn(node, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MPlug outShadersPlug = depNodeFn.findPlug("shaders", true, &status);
        if (status != MS::kSuccess || !outShadersPlug) {
            continue;
        }

        shadersPlug = outShadersPlug;
    }

    return shadersPlug;
}

MObject UsdMayaUtil::GetDefaultLightSetObject()
{
    MObject node;
    MStatus status;

    MItDependencyNodes setIter(MFn::kSet, &status);
    CHECK_MSTATUS_AND_RETURN(status, node);

    while (!setIter.isDone()) {
        node = setIter.thisNode();
        setIter.next();

        MFnSet setFn(node, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        if (setFn.name() == MString("defaultLightSet")) {
            break;
        }
    }

    return node;
}

bool UsdMayaUtil::isAncestorDescendentRelationship(const MDagPath& path1, const MDagPath& path2)
{
    unsigned int length1 = path1.length();
    unsigned int length2 = path2.length();
    unsigned int diff;

    if (length1 == length2 && !(path1 == path2))
        return false;
    MDagPath ancestor, descendent;
    if (length1 > length2) {
        ancestor = path2;
        descendent = path1;
        diff = length1 - length2;
    } else {
        ancestor = path1;
        descendent = path2;
        diff = length2 - length1;
    }

    descendent.pop(diff);

    return ancestor == descendent;
}

// returns 0 if static, 1 if sampled, and 2 if a curve
int UsdMayaUtil::getSampledType(const MPlug& iPlug, const bool includeConnectedChildren)
{
    MPlugArray conns;

    iPlug.connectedTo(conns, true, false);

    // it's possible that only some element of an array plug or
    // some component of a compound plus is connected
    if (conns.length() == 0) {
        if (iPlug.isArray()) {
            unsigned int numConnectedElements = iPlug.numConnectedElements();
            for (unsigned int e = 0; e < numConnectedElements; e++) {
                // For now we assume that when you encounter an array of plugs,
                // we always want to include connected children.
                int retVal = getSampledType(iPlug.connectionByPhysicalIndex(e), true);
                if (retVal > 0) {
                    return retVal;
                }
            }
        } else if (
            iPlug.isCompound() && iPlug.numConnectedChildren() > 0 && includeConnectedChildren) {
            unsigned int numChildren = iPlug.numChildren();
            for (unsigned int c = 0; c < numChildren; c++) {
                int retVal = getSampledType(iPlug.child(c), true);
                if (retVal > 0)
                    return retVal;
            }
        }
        return 0;
    }

    MObject           ob;
    MFnDependencyNode nodeFn;
    for (unsigned i = 0; i < conns.length(); i++) {
        ob = conns[i].node();
        MFn::Type type = ob.apiType();

        switch (type) {
        case MFn::kAnimCurveTimeToAngular:
        case MFn::kAnimCurveTimeToDistance:
        case MFn::kAnimCurveTimeToTime:
        case MFn::kAnimCurveTimeToUnitless: {
            nodeFn.setObject(ob);
            MPlug incoming = nodeFn.findPlug("i", true);

            // sampled
            if (incoming.isConnected())
                return 1;

            // curve
            else
                return 2;
        } break;

        case MFn::kMute: {
            nodeFn.setObject(ob);
            MPlug mutePlug = nodeFn.findPlug("mute", true);

            // static
            if (mutePlug.asBool())
                return 0;
            // curve
            else
                return 2;
        } break;

        default: break;
        }
    }

    return 1;
}

// does this cover all cases?
bool UsdMayaUtil::isAnimated(const MObject& mayaObject, const bool checkParent)
{
    // MItDependencyGraph takes a non-const MObject as a constructor parameter,
    // so we have to make a copy of mayaObject here.
    MObject mayaObjectCopy(mayaObject);

    MStatus            status;
    MItDependencyGraph iter(
        mayaObjectCopy,
        MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &status);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "Unable to create DG iterator for Maya node '%s'", GetMayaNodeName(mayaObject).c_str());
    }

    // MAnimUtil::isAnimated(node) will search the history of the node
    // for any animation curve nodes. It will return true for those nodes
    // that have animation curve in their history.
    // The average time complexity is O(n^2) where n is the number of history
    // nodes. But we can improve the best case by splitting the loop into two.
    std::vector<MObject> nodesToCheckAnimCurve;

    for (; !iter.isDone(); iter.next()) {
        MObject node = iter.thisNode();

        if (node.hasFn(MFn::kPluginDependNode) || node.hasFn(MFn::kConstraint)
            || node.hasFn(MFn::kPointConstraint) || node.hasFn(MFn::kAimConstraint)
            || node.hasFn(MFn::kOrientConstraint) || node.hasFn(MFn::kScaleConstraint)
            || node.hasFn(MFn::kGeometryConstraint) || node.hasFn(MFn::kNormalConstraint)
            || node.hasFn(MFn::kTangentConstraint) || node.hasFn(MFn::kParentConstraint)
            || node.hasFn(MFn::kPoleVectorConstraint) || node.hasFn(MFn::kParentConstraint)
            || node.hasFn(MFn::kTime) || node.hasFn(MFn::kJoint) || node.hasFn(MFn::kGeometryFilt)
            || node.hasFn(MFn::kTweak) || node.hasFn(MFn::kPolyTweak) || node.hasFn(MFn::kSubdTweak)
            || node.hasFn(MFn::kCluster) || node.hasFn(MFn::kFluid)
            || node.hasFn(MFn::kPolyBoolOp)) {
            return true;
        }

        if (node.hasFn(MFn::kExpression)) {
            MFnExpression fn(node, &status);
            if (status == MS::kSuccess && fn.isAnimated()) {
                return true;
            }
        }

        nodesToCheckAnimCurve.push_back(node);
    }

    for (const MObject& node : nodesToCheckAnimCurve) {
        if (MAnimUtil::isAnimated(node, checkParent)) {
            return true;
        }
    }

    return false;
}

bool UsdMayaUtil::isPlugAnimated(const MPlug& plug)
{
    if (plug.isNull()) {
        return false;
    }
    if (MAnimUtil::isAnimated(plug)) {
        return true;
    }
    if (plug.isDestination()) {
        const MPlug source(GetConnected(plug));
        if (!source.isNull() && MAnimUtil::isAnimated(source.node())) {
            return true;
        }
    }
    return false;
}

bool UsdMayaUtil::isIntermediate(const MObject& object)
{
    MStatus status;

    const MFnDagNode dagNodeFn(object, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const bool isIntermediateObj = dagNodeFn.isIntermediateObject(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    return isIntermediateObj;
}

bool UsdMayaUtil::isRenderable(const MObject& object)
{
    MStatus    stat;
    MFnDagNode mFn(object);

    // templated turned on?  return false
    MPlug plug = mFn.findPlug("template", false, &stat);
    if (stat == MS::kSuccess && plug.asBool())
        return false;

    // visibility or lodVisibility off?  return false
    plug = mFn.findPlug("visibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool()) {
        // the value is off. let's check if it has any in-connection,
        // otherwise, it means it is not animated.
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0) {
            return false;
        }
    }

    plug = mFn.findPlug("lodVisibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool()) {
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0) {
            return false;
        }
    }

    // this shape is renderable
    return true;
}

bool UsdMayaUtil::isWritable(const MObject& object)
{
    MStatus status;

    const MFnDependencyNode depNodeFn(object, &status);
    if (status != MS::kSuccess) {
        return true;
    }

    const bool isDefaultNode = depNodeFn.isDefaultNode(&status);
    if (status != MS::kSuccess) {
        return true;
    }

    const bool canBeWritten = depNodeFn.canBeWritten(&status);
    if (status != MS::kSuccess) {
        return true;
    }

    return (!isDefaultNode && canBeWritten);
}

std::string UsdMayaUtil::stripNamespaces(const std::string& nodeName, const int nsDepth)
{
    if (nodeName.empty() || nsDepth == 0) {
        return nodeName;
    }

    std::stringstream ss;

    const std::vector<std::string> nodeNameParts
        = TfStringSplit(nodeName, UsdMayaUtil::MayaDagDelimiter);

    const bool isAbsolute = TfStringStartsWith(nodeName, UsdMayaUtil::MayaDagDelimiter);

    for (size_t i = 0u; i < nodeNameParts.size(); ++i) {
        if (i == 0u && isAbsolute) {
            // If nodeName was absolute, the first element in nodeNameParts
            // will be empty, so just skip it. The output path will be made
            // absolute with the next iteration.
            continue;
        }

        if (i != 0u) {
            ss << UsdMayaUtil::MayaDagDelimiter;
        }

        const std::vector<std::string> nsNameParts
            = TfStringSplit(nodeNameParts[i], UsdMayaUtil::MayaNamespaceDelimiter);

        const size_t nodeNameIndex = nsNameParts.size() - 1u;

        auto startIter = nsNameParts.begin();
        if (nsDepth < 0) {
            // If nsDepth is negative, we don't keep any namespaces, so advance
            // startIter to the last element in the vector, which is just the
            // node name.
            startIter += nodeNameIndex;
        } else {
            // Otherwise we strip as many namespaces as possible up to nsDepth,
            // but no more than what would leave us with just the node name.
            startIter += std::min(static_cast<size_t>(nsDepth), nodeNameIndex);
        }

        ss << TfStringJoin(
            startIter, nsNameParts.end(), UsdMayaUtil::MayaNamespaceDelimiter.c_str());
    }

    return ss.str();
}

std::string UsdMayaUtil::SanitizeName(const std::string& name)
{
    return TfStringReplace(name, ":", "_");
}

// This to allow various pipeline to sanitize the colorset name for output
std::string UsdMayaUtil::SanitizeColorSetName(const std::string& name)
{
    // We sanitize the name since certain pipeline like Pixar's, we have rman_
    // in front of all color sets that need to be exportred. We now export all
    // colorsets.
    size_t                   namePos = 0u;
    static const std::string RMAN_PREFIX("rman_");
    if (name.find(RMAN_PREFIX) == 0) {
        namePos = RMAN_PREFIX.size();
    }
    return name.substr(namePos);
}

// Get array (constant or per component) of attached shaders
// Pass a non-zero value for numComponents when retrieving shaders on an object
// that supports per-component shader assignment (e.g. faces of a polymesh).
// In this case, shaderObjs will be the length of the number of shaders
// assigned to the object. assignmentIndices will be the length of
// numComponents, with values indexing into shaderObjs.
// When numComponents is zero, shaderObjs will be of length 1 and
// assignmentIndices will be empty.
static bool _getAttachedMayaShaderObjects(
    const MFnDagNode&  node,
    const unsigned int numComponents,
    MObjectArray*      shaderObjs,
    VtIntArray*        assignmentIndices)
{
    bool    hasShader = false;
    MStatus status;

    // This structure maps shader object names to their indices in the
    // shaderObjs array. We use this to make sure that we add each unique
    // shader to shaderObjs only once.
    TfHashMap<std::string, size_t, TfHash> shaderPlugsMap;

    shaderObjs->clear();
    assignmentIndices->clear();

    MObjectArray setObjs;
    MObjectArray compObjs;
    node.getConnectedSetsAndMembers(
        0, setObjs, compObjs, true); // Assuming that not using instancing

    // If we have multiple components and either multiple sets or one set with
    // only a subset of the object in it, we'll keep track of the assignments
    // for all components in assignmentIndices. We initialize all of the
    // assignments as unassigned using a value of -1.
    if (numComponents > 1
        && (setObjs.length() > 1 || (setObjs.length() == 1 && !compObjs[0].isNull()))) {
        assignmentIndices->assign((size_t)numComponents, -1);
    }

    for (unsigned int i = 0; i < setObjs.length(); ++i) {
        // Get associated Set and Shading Group
        MFnSet setFn(setObjs[i], &status);
        MPlug  seSurfaceShaderPlg = setFn.findPlug("surfaceShader", true, &status);

        // Find connection shader->shadingGroup
        MPlugArray plgCons;
        seSurfaceShaderPlg.connectedTo(plgCons, true, false, &status);
        if (plgCons.length() == 0) {
            continue;
        }

        hasShader = true;
        MPlug   shaderPlug = plgCons[0];
        MObject shaderObj = shaderPlug.node();

        auto inserted = shaderPlugsMap.insert(
            std::pair<std::string, size_t>(shaderPlug.name().asChar(), shaderObjs->length()));
        if (inserted.second) {
            shaderObjs->append(shaderObj);
        }

        // If we are tracking per-component assignments, mark all components of
        // this set as assigned to this shader.
        if (!assignmentIndices->empty()) {
            size_t shaderIndex = inserted.first->second;

            MItMeshPolygon faceIt(node.dagPath(), compObjs[i]);
            for (faceIt.reset(); !faceIt.isDone(); faceIt.next()) {
                (*assignmentIndices)[faceIt.index()] = shaderIndex;
            }
        }
    }

    return hasShader;
}

bool _GetColorAndTransparencyFromLambert(const MObject& shaderObj, GfVec3f* rgb, float* alpha)
{
    MStatus          status;
    MFnLambertShader lambertFn(shaderObj, &status);
    if (status == MS::kSuccess) {
        if (rgb) {
            GfVec3f displayColor;
            MColor  color = lambertFn.color();
            for (int j = 0; j < 3; j++) {
                displayColor[j] = color[j];
            }
            displayColor *= lambertFn.diffuseCoeff();
            *rgb = UsdMayaColorSpace::ConvertMayaToLinear(displayColor);
        }
        if (alpha) {
            MColor trn = lambertFn.transparency();
            // Assign Alpha as 1.0 - average of shader transparency
            // and check if they are all the same
            *alpha = 1.0f - ((trn[0] + trn[1] + trn[2]) / 3.0f);
        }
        return true;
    }

    return false;
}

bool _GetColorAndTransparencyFromStandardSurface(
    const MObject& shaderObj,
    GfVec3f*       rgb,
    float*         alpha)
{
#if MAYA_API_VERSION >= 20200000
    MStatus                  status;
    MFnStandardSurfaceShader surfaceFn(shaderObj, &status);
    if (status == MS::kSuccess) {
        if (rgb) {
            GfVec3f displayColor;
            MColor  color = surfaceFn.baseColor();
            for (int j = 0; j < 3; j++) {
                displayColor[j] = color[j];
            }
            displayColor *= surfaceFn.base();
            *rgb = UsdMayaColorSpace::ConvertMayaToLinear(displayColor);
        }
        if (alpha) {
            *alpha = 1.0f - surfaceFn.transmission();
        }
        return true;
    }
#endif
    return false;
}

bool _GetColorAndTransparencyFromDepNode(const MObject& shaderObj, GfVec3f* rgb, float* alpha)
{
    MStatus           status;
    MFnDependencyNode d(shaderObj);
    if (rgb) {
        MPlug colorPlug = d.findPlug("color", true, &status);
        if (!status) {
            // For UsdPreviewSurface support:
            colorPlug = d.findPlug("diffuseColor", true, &status);
            if (!status) {
                return false;
            }
        }
        GfVec3f displayColor;
        for (int j = 0; j < 3; j++) {
            colorPlug.child(j).getValue(displayColor[j]);
        }
        *rgb = UsdMayaColorSpace::ConvertMayaToLinear(displayColor);
    }

    if (alpha) {
        MPlug transparencyPlug = d.findPlug("transparency", true, &status);
        if (status) {
            float trans = 0.f;
            for (int j = 0; j < 3; j++) {
                float t = 0.f;
                transparencyPlug.child(j).getValue(t);
                trans += t / 3.f;
            }
            *alpha = 1.f - trans;
        } else {
            // For UsdPreviewSurface support:
            MPlug opacityPlug = d.findPlug("opacity", true, &status);
            if (!status) {
                return false;
            }
            *alpha = opacityPlug.asFloat();
        }
    }
    return true;
}

static bool
_getMayaShadersColor(const MObjectArray& shaderObjs, VtVec3fArray* RGBData, VtFloatArray* AlphaData)
{
    if (shaderObjs.length() == 0u) {
        return false;
    }

    if (RGBData) {
        RGBData->resize(shaderObjs.length());
    }
    if (AlphaData) {
        AlphaData->resize(shaderObjs.length());
    }

    bool gotValues = false;

    for (unsigned int i = 0u; i < shaderObjs.length(); ++i) {
        // Initialize RGB and Alpha to (1,1,1,1)
        if (RGBData) {
            (*RGBData)[i][0u] = 1.0f;
            (*RGBData)[i][1u] = 1.0f;
            (*RGBData)[i][2u] = 1.0f;
        }
        if (AlphaData) {
            (*AlphaData)[i] = 1.0f;
        }

        if (shaderObjs[i].isNull()) {
            TF_RUNTIME_ERROR(
                "Invalid Maya shader object at index %d. "
                "Unable to retrieve shader base color.",
                i);
            continue;
        }

        // First, we assume the shader is a lambert and try that API. If not,
        // we try our next best guess.
        const bool gotShaderValues = _GetColorAndTransparencyFromLambert(
                                         shaderObjs[i],
                                         RGBData ? &(*RGBData)[i] : nullptr,
                                         AlphaData ? &(*AlphaData)[i] : nullptr)

            || _GetColorAndTransparencyFromStandardSurface(
                                         shaderObjs[i],
                                         RGBData ? &(*RGBData)[i] : nullptr,
                                         AlphaData ? &(*AlphaData)[i] : nullptr)

            || _GetColorAndTransparencyFromDepNode(
                                         shaderObjs[i],
                                         RGBData ? &(*RGBData)[i] : nullptr,
                                         AlphaData ? &(*AlphaData)[i] : nullptr);

        gotValues |= gotShaderValues;
    }

    return gotValues;
}

static bool _GetLinearShaderColor(
    const MFnDagNode&  node,
    const unsigned int numComponents,
    VtVec3fArray*      RGBData,
    VtFloatArray*      AlphaData,
    TfToken*           interpolation,
    VtIntArray*        assignmentIndices)
{
    MObjectArray shaderObjs;
    bool         hasAttachedShader
        = _getAttachedMayaShaderObjects(node, numComponents, &shaderObjs, assignmentIndices);
    if (hasAttachedShader) {
        _getMayaShadersColor(shaderObjs, RGBData, AlphaData);
    }

    if (assignmentIndices && interpolation) {
        if (assignmentIndices->empty()) {
            *interpolation = UsdGeomTokens->constant;
        } else {
            *interpolation = UsdGeomTokens->uniform;
        }
    }

    return hasAttachedShader;
}

bool UsdMayaUtil::GetLinearShaderColor(
    const MFnDagNode& node,
    VtVec3fArray*     RGBData,
    VtFloatArray*     AlphaData,
    TfToken*          interpolation,
    VtIntArray*       assignmentIndices)
{
    return _GetLinearShaderColor(node, 0, RGBData, AlphaData, interpolation, assignmentIndices);
}

bool UsdMayaUtil::GetLinearShaderColor(
    const MFnMesh& mesh,
    VtVec3fArray*  RGBData,
    VtFloatArray*  AlphaData,
    TfToken*       interpolation,
    VtIntArray*    assignmentIndices)
{
    unsigned int numComponents = mesh.numPolygons();
    return _GetLinearShaderColor(
        mesh, numComponents, RGBData, AlphaData, interpolation, assignmentIndices);
}

namespace {

template <typename T> struct _ValuesHash
{
    std::size_t operator()(const T& value) const { return hash_value(value); }
};

// There is no globally defined hash_value for numeric types
// so we need an explicit opt-in here.
template <> struct _ValuesHash<float>
{
    std::size_t operator()(const float& value) const { return boost::hash_value(value); }
};

template <typename T> struct _ValuesEqual
{
    bool operator()(const T& a, const T& b) const { return GfIsClose(a, b, 1e-9); }
};

} // anonymous namespace

template <typename T>
static void _MergeEquivalentIndexedValues(VtArray<T>* valueData, VtIntArray* assignmentIndices)
{
    if (!valueData || !assignmentIndices) {
        return;
    }

    const size_t numValues = valueData->size();
    if (numValues == 0u) {
        return;
    }

    // We maintain a map of values to that value's index in our uniqueValues
    // array.
    std::unordered_map<T, size_t, _ValuesHash<T>, _ValuesEqual<T>> valuesMap;
    VtArray<T>                                                     uniqueValues;
    VtIntArray                                                     uniqueIndices;

    for (int index : *assignmentIndices) {
        if (index < 0 || static_cast<size_t>(index) >= numValues) {
            // This is an unassigned or otherwise unknown index, so just keep it.
            uniqueIndices.push_back(index);
            continue;
        }

        const T value = (*valueData)[index];

        int uniqueIndex = -1;

        auto inserted = valuesMap.insert(std::pair<T, size_t>(value, uniqueValues.size()));
        if (inserted.second) {
            // This is a new value, so add it to the array.
            uniqueValues.push_back(value);
            uniqueIndex = uniqueValues.size() - 1;
        } else {
            // This is an existing value, so re-use the original's index.
            uniqueIndex = inserted.first->second;
        }

        uniqueIndices.push_back(uniqueIndex);
    }

    // If we reduced the number of values by merging, copy the results back.
    if (uniqueValues.size() < numValues) {
        (*valueData) = uniqueValues;
        (*assignmentIndices) = uniqueIndices;
    }
}

void UsdMayaUtil::MergeEquivalentIndexedValues(
    VtFloatArray* valueData,
    VtIntArray*   assignmentIndices)
{
    return _MergeEquivalentIndexedValues<float>(valueData, assignmentIndices);
}

void UsdMayaUtil::MergeEquivalentIndexedValues(
    VtVec2fArray* valueData,
    VtIntArray*   assignmentIndices)
{
    return _MergeEquivalentIndexedValues<GfVec2f>(valueData, assignmentIndices);
}

void UsdMayaUtil::MergeEquivalentIndexedValues(
    VtVec3fArray* valueData,
    VtIntArray*   assignmentIndices)
{
    return _MergeEquivalentIndexedValues<GfVec3f>(valueData, assignmentIndices);
}

void UsdMayaUtil::MergeEquivalentIndexedValues(
    VtVec4fArray* valueData,
    VtIntArray*   assignmentIndices)
{
    return _MergeEquivalentIndexedValues<GfVec4f>(valueData, assignmentIndices);
}

void UsdMayaUtil::CompressFaceVaryingPrimvarIndices(
    const MFnMesh& mesh,
    TfToken*       interpolation,
    VtIntArray*    assignmentIndices)
{
    if (!interpolation || !assignmentIndices || assignmentIndices->size() == 0u) {
        return;
    }

    // Use -2 as the initial "un-stored" sentinel value, since -1 is the
    // default unauthored value index for primvars.
    int        numPolygons = mesh.numPolygons();
    VtIntArray uniformAssignments;
    uniformAssignments.assign((size_t)numPolygons, -2);

    int        numVertices = mesh.numVertices();
    VtIntArray vertexAssignments;
    vertexAssignments.assign((size_t)numVertices, -2);

    // We assume that the data is constant/uniform/vertex until we can
    // prove otherwise that two components have differing values.
    bool isConstant = true;
    bool isUniform = true;
    bool isVertex = true;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int      fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int faceIndex = itFV.faceId();
        int vertexIndex = itFV.vertId();

        int assignedIndex = (*assignmentIndices)[fvi];

        if (isConstant) {
            if (assignedIndex != (*assignmentIndices)[0]) {
                isConstant = false;
            }
        }

        if (isUniform) {
            if (uniformAssignments[faceIndex] < -1) {
                // No value for this face yet, so store one.
                uniformAssignments[faceIndex] = assignedIndex;
            } else if (assignedIndex != uniformAssignments[faceIndex]) {
                isUniform = false;
            }
        }

        if (isVertex) {
            if (vertexAssignments[vertexIndex] < -1) {
                // No value for this vertex yet, so store one.
                vertexAssignments[vertexIndex] = assignedIndex;
            } else if (assignedIndex != vertexAssignments[vertexIndex]) {
                isVertex = false;
            }
        }

        if (!isConstant && !isUniform && !isVertex) {
            // No compression will be possible, so stop trying.
            break;
        }
    }

    if (isConstant) {
        assignmentIndices->resize(1);
        *interpolation = UsdGeomTokens->constant;
    } else if (isUniform) {
        *assignmentIndices = uniformAssignments;
        *interpolation = UsdGeomTokens->uniform;
    } else if (isVertex) {
        *assignmentIndices = vertexAssignments;
        *interpolation = UsdGeomTokens->vertex;
    } else {
        *interpolation = UsdGeomTokens->faceVarying;
    }
}

bool UsdMayaUtil::IsAuthored(const MPlug& plug)
{
    MStatus status;

    if (plug.isNull(&status) || status != MS::kSuccess) {
        return false;
    }

    // Plugs that are the destination of a connection are considered authored,
    // since their value comes from an upstream dependency. If the plug is only
    // the source of a connection or is not connected at all, it's
    // authored-ness only depends on its own value, which is checked below.
    if (plug.isDestination(&status)) {
        return true;
    }

    // MPlug::getSetAttrCmds() is currently not declared const, so we have to
    // make a copy of plug here.
    MPlug plugCopy(plug);

    MStringArray setAttrCmds;
    status = plugCopy.getSetAttrCmds(setAttrCmds, MPlug::kChanged);
    CHECK_MSTATUS_AND_RETURN(status, false);

    for (unsigned int i = 0u; i < setAttrCmds.length(); ++i) {
        if (setAttrCmds[i].numChars() > 0u) {
            return true;
        }
    }

    return false;
}

MPlug UsdMayaUtil::GetConnected(const MPlug& plug)
{
    MStatus    status = MS::kFailure;
    MPlugArray conn;
    plug.connectedTo(conn, true, false, &status);
    if (!status || conn.length() != 1) {
        return MPlug();
    }
    return conn[0];
}

void UsdMayaUtil::Connect(const MPlug& srcPlug, const MPlug& dstPlug, const bool clearDstPlug)
{
    MStatus     status;
    MDGModifier dgMod;

    if (clearDstPlug) {
        MPlugArray plgCons;
        dstPlug.connectedTo(plgCons, true, false, &status);
        for (unsigned int i = 0; i < plgCons.length(); ++i) {
            status = dgMod.disconnect(plgCons[i], dstPlug);
        }
    }

    // Execute the disconnect/connect
    status = dgMod.connect(srcPlug, dstPlug);
    dgMod.doIt();
}

MPlug UsdMayaUtil::FindChildPlugByName(const MPlug& plug, const MString& name)
{
    unsigned int numChildren = plug.numChildren();
    for (unsigned int i = 0; i < numChildren; ++i) {
        MPlug child = plug.child(i);

        // We can't get at the name of just the *component*
        // plug.name() gives us node.plug[index].compound, etc.
        // partialName() also has no form that just gives us the name.
        MString childName = child.name();
        if (childName.length() > name.length()) {
            int index = childName.rindex('.');
            if (index >= 0) {
                MString childSuffix = childName.substring(index + 1, childName.length());
                if (childSuffix == name) {
                    return child;
                }
            }
        }
    }
    return MPlug();
}

// XXX: see logic in UsdMayaTransformWriter.  It's unfortunate that this
// logic is in 2 places.  we should merge.
static bool _IsShape(const MDagPath& dagPath)
{
    if (dagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    // go to the parent
    MDagPath parentDagPath = dagPath;
    parentDagPath.pop();
    if (!parentDagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    unsigned int numberOfShapesDirectlyBelow = 0;
    parentDagPath.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    return (numberOfShapesDirectlyBelow == 1);
}

SdfPath UsdMayaUtil::MayaNodeNameToSdfPath(const std::string& nodeName, const bool stripNamespaces)
{
    std::string pathString = nodeName;

    if (stripNamespaces) {
        // Drop namespaces instead of making them part of the path.
        pathString = UsdMayaUtil::stripNamespaces(pathString);
    }

    std::replace(
        pathString.begin(),
        pathString.end(),
        UsdMayaUtil::MayaDagDelimiter[0],
        SdfPathTokens->childDelimiter.GetString()[0]);
    std::replace(pathString.begin(), pathString.end(), UsdMayaUtil::MayaNamespaceDelimiter[0], '_');

    return SdfPath(pathString);
}

SdfPath UsdMayaUtil::MDagPathToUsdPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces)
{
    SdfPath usdPath
        = UsdMayaUtil::MayaNodeNameToSdfPath(dagPath.fullPathName().asChar(), stripNamespaces);

    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

bool UsdMayaUtil::GetBoolCustomData(
    const UsdAttribute& obj,
    const TfToken&      key,
    const bool          defaultValue)
{
    bool    returnValue = defaultValue;
    VtValue data = obj.GetCustomDataByKey(key);
    if (!data.IsEmpty()) {
        if (data.IsHolding<bool>()) {
            return data.Get<bool>();
        } else {
            TF_RUNTIME_ERROR(
                "customData at key '%s' is not of type bool. Skipping...", key.GetText());
        }
    }
    return returnValue;
}

template <typename T> static T _GetVec(const UsdAttribute& attr, const VtValue& val)
{
    const T ret = val.UncheckedGet<T>();

    if (attr.GetRoleName() == SdfValueRoleNames->Color) {
        return UsdMayaColorSpace::ConvertMayaToLinear(ret);
    }

    return ret;
}

MMatrix UsdMayaUtil::GfMatrixToMMatrix(const GfMatrix4d& mx)
{
    MMatrix mayaMx;
    std::copy(mx.GetArray(), mx.GetArray() + 16, mayaMx[0]);
    return mayaMx;
}

bool UsdMayaUtil::getPlugMatrix(
    const MFnDependencyNode& depNode,
    const MString&           attr,
    MMatrix*                 outVal)
{
    MStatus status;
    MPlug   plug = depNode.findPlug(attr, true, &status);
    if (!status) {
        return false;
    }

    MObject plugObj = plug.asMObject(&status);
    if (!status) {
        return false;
    }

    MFnMatrixData plugMatrixData(plugObj, &status);
    if (!status) {
        return false;
    }

    *outVal = plugMatrixData.matrix();
    return true;
}

bool UsdMayaUtil::setPlugMatrix(
    const MFnDependencyNode& depNode,
    const MString&           attr,
    const GfMatrix4d&        mx)
{
    MStatus status;
    MPlug   plug = depNode.findPlug(attr, true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    return setPlugMatrix(mx, plug);
}

bool UsdMayaUtil::setPlugMatrix(const GfMatrix4d& mx, MPlug& plug)
{
    MStatus status;
    MObject mxObj = MFnMatrixData().create(GfMatrixToMMatrix(mx), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = plug.setValue(mxObj);
    CHECK_MSTATUS_AND_RETURN(status, false);
    return true;
}

bool UsdMayaUtil::setPlugValue(const UsdAttribute& usdAttr, MPlug& attrPlug)
{
    return setPlugValue(usdAttr, UsdTimeCode::Default(), attrPlug);
}

bool UsdMayaUtil::setPlugValue(const UsdAttribute& usdAttr, const UsdTimeCode time, MPlug& attrPlug)
{
    VtValue val;
    if (!usdAttr.Get(&val, time)) {
        return false;
    }

    MStatus status = MStatus::kFailure;

    if (val.IsHolding<double>()) {
        status = attrPlug.setDouble(val.UncheckedGet<double>());
    } else if (val.IsHolding<float>()) {
        status = attrPlug.setFloat(val.UncheckedGet<float>());
    } else if (val.IsHolding<int>()) {
        status = attrPlug.setInt(val.UncheckedGet<int>());
    } else if (val.IsHolding<short>()) {
        status = attrPlug.setShort(val.UncheckedGet<short>());
    } else if (val.IsHolding<bool>()) {
        status = attrPlug.setBool(val.UncheckedGet<bool>());
    } else if (val.IsHolding<SdfAssetPath>()) {
        // Assume that Ar and Maya will resolve paths the same.  This the best
        // we can do w.r.t. to round-tripping.
        status = attrPlug.setString(val.UncheckedGet<SdfAssetPath>().GetAssetPath().c_str());
    } else if (val.IsHolding<std::string>()) {
        status = attrPlug.setString(val.UncheckedGet<std::string>().c_str());
    } else if (val.IsHolding<TfToken>()) {
        const TfToken token(val.UncheckedGet<TfToken>());
        const MObject attrObj = attrPlug.attribute(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        if (attrObj.hasFn(MFn::kEnumAttribute)) {
            MFnEnumAttribute attrEnumFn(attrObj, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            short enumVal = attrEnumFn.fieldIndex(token.GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = attrPlug.setShort(enumVal);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<GfVec2d>()) {
        if (attrPlug.isCompound()) {
            const GfVec2d& vecVal = val.UncheckedGet<GfVec2d>();
            for (size_t i = 0u; i < GfVec2d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfVec2f>()) {
        if (attrPlug.isCompound()) {
            const GfVec2f& vecVal = val.UncheckedGet<GfVec2f>();
            for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfVec3d>()) {
        if (attrPlug.isCompound()) {
            const GfVec3d vecVal = _GetVec<GfVec3d>(usdAttr, val);
            for (size_t i = 0u; i < GfVec3d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfVec3f>()) {
        if (attrPlug.isCompound()) {
            const GfVec3f vecVal = _GetVec<GfVec3f>(usdAttr, val);
            for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfVec4d>()) {
        if (attrPlug.isCompound()) {
            const GfVec4d vecVal = _GetVec<GfVec4d>(usdAttr, val);
            for (size_t i = 0u; i < GfVec4d::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfVec4f>()) {
        if (attrPlug.isCompound()) {
            const GfVec4f vecVal = _GetVec<GfVec4f>(usdAttr, val);
            for (size_t i = 0u; i < GfVec4f::dimension; ++i) {
                MPlug childPlug = attrPlug.child(static_cast<unsigned int>(i), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[i]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<GfMatrix4d>()) {
        MObject object = attrPlug.attribute();
        if (object.hasFn(MFn::kTypedAttribute)
            && MFnTypedAttribute(object).attrType() == MFnData::kMatrix) {
            GfMatrix4d mat = val.Get<GfMatrix4d>();
            MMatrix    mayaMat;
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    mayaMat[i][j] = mat[i][j];
                }
            }
            MFnMatrixData data;
            MObject       dataObj = data.create();
            data.set(mayaMat);
            status = attrPlug.setValue(dataObj);
        }
    } else if (val.IsHolding<VtDoubleArray>()) {
        const VtDoubleArray& valArray = val.UncheckedGet<VtDoubleArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setDouble(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtFloatArray>()) {
        const VtFloatArray& valArray = val.UncheckedGet<VtFloatArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setFloat(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtIntArray>()) {
        const VtIntArray& valArray = val.UncheckedGet<VtIntArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setInt(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtShortArray>()) {
        const VtShortArray& valArray = val.UncheckedGet<VtShortArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setShort(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtBoolArray>()) {
        const VtBoolArray& valArray = val.UncheckedGet<VtBoolArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setBool(valArray[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtStringArray>()) {
        const VtStringArray& valArray = val.UncheckedGet<VtStringArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            status = elemPlug.setString(valArray[i].c_str());
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
    } else if (val.IsHolding<VtVec2dArray>()) {
        const VtVec2dArray valArray = val.UncheckedGet<VtVec2dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            const GfVec2d& vecVal = valArray[i];
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec2d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<VtVec2fArray>()) {
        const VtVec2fArray valArray = val.UncheckedGet<VtVec2fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            const GfVec2f& vecVal = valArray[i];
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec2f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<VtVec3dArray>()) {
        const VtVec3dArray valArray = val.UncheckedGet<VtVec3dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec3d vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec3d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<VtVec3fArray>()) {
        const VtVec3fArray valArray = val.UncheckedGet<VtVec3fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec3f vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec3f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<VtVec4dArray>()) {
        const VtVec4dArray valArray = val.UncheckedGet<VtVec4dArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec4d vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec4d::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setDouble(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else if (val.IsHolding<VtVec4fArray>()) {
        const VtVec4fArray valArray = val.UncheckedGet<VtVec4fArray>();
        status = attrPlug.setNumElements(static_cast<unsigned int>(valArray.size()));
        CHECK_MSTATUS_AND_RETURN(status, false);
        for (size_t i = 0u; i < valArray.size(); ++i) {
            GfVec4f vecVal = valArray[i];
            if (usdAttr.GetRoleName() == SdfValueRoleNames->Color) {
                vecVal = UsdMayaColorSpace::ConvertMayaToLinear(vecVal);
            }
            MPlug elemPlug = attrPlug.elementByPhysicalIndex(static_cast<unsigned int>(i), &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            for (size_t j = 0u; j < GfVec4f::dimension; ++j) {
                MPlug childPlug = elemPlug.child(static_cast<unsigned int>(j), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = childPlug.setFloat(vecVal[j]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    } else {
        TF_CODING_ERROR(
            "Unsupported type '%s' for USD attribute '%s'",
            usdAttr.GetTypeName().GetAsToken().GetText(),
            usdAttr.GetPath().GetText());
        return false;
    }

    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

bool UsdMayaUtil::SetNotes(MFnDependencyNode& depNode, const std::string& notes)
{
    MStatus           status = MStatus::kFailure;
    MFnTypedAttribute typedAttrFn;
    MObject           attrObj
        = typedAttrFn.create("notes", "nts", MFnData::kString, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = depNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN(status, false);

    depNode.findPlug(attrObj, true).setString(notes.c_str());
    return true;
}

bool UsdMayaUtil::SetHiddenInOutliner(MFnDependencyNode& depNode, const bool hidden)
{
    MPlug plug = depNode.findPlug("hiddenInOutliner", true);
    if (!plug.isNull()) {
        plug.setBool(hidden);
        return true;
    }
    return false;
}

UsdMayaUtil::MDataHandleHolder::MDataHandleHolder(const MPlug& plug, MDataHandle dataHandle)
    : _plug(plug)
    , _dataHandle(dataHandle)
{
}

UsdMayaUtil::MDataHandleHolder::~MDataHandleHolder()
{
    if (!_plug.isNull()) {
        _plug.destructHandle(_dataHandle);
    }
}

TfRefPtr<UsdMayaUtil::MDataHandleHolder> UsdMayaUtil::MDataHandleHolder::New(const MPlug& plug)
{
    MStatus status;

    MDataHandle dataHandle = plug.asMDataHandle(&status);
    if (!status.error()) {
        return TfCreateRefPtr(new UsdMayaUtil::MDataHandleHolder(plug, dataHandle));
    } else {
        return nullptr;
    }
}

TfRefPtr<UsdMayaUtil::MDataHandleHolder> UsdMayaUtil::GetPlugDataHandle(const MPlug& plug)
{
    return UsdMayaUtil::MDataHandleHolder::New(plug);
}

VtDictionary UsdMayaUtil::GetDictionaryFromArgDatabase(
    const MArgDatabase& argData,
    const VtDictionary& guideDict)
{
    // We handle three types of arguments:
    // 1 - bools: Some bools are actual boolean flags (t/f) in Maya, and others
    //     are false if omitted, true if present (simple flags).
    // 2 - strings: Just strings!
    // 3 - vectors (multi-use args): Try to mimic the way they're passed in the
    //     Python command API. If single arg per flag, make it a vector of
    //     strings. Multi arg per flag, vector of vector of strings.
    VtDictionary args;
    for (const auto& entry : guideDict) {
        const std::string& key = entry.first;
        const VtValue&     guideValue = entry.second;
        if (!argData.isFlagSet(key.c_str())) {
            continue;
        }

        // The usdExport command must handle bools, strings, and vectors.
        if (guideValue.IsHolding<bool>()) {
            // The flag should be either 0-arg or 1-arg. If 0-arg, it's true by
            // virtue of being present (getFlagArgument won't change val). If
            // it's 1-arg, then getFlagArgument will set the appropriate true
            // or false value.
            bool val = true;
            argData.getFlagArgument(key.c_str(), 0, val);
            args[key] = val;
        } else if (guideValue.IsHolding<std::string>()) {
            const std::string val = argData.flagArgumentString(key.c_str(), 0).asChar();
            args[key] = val;
        } else if (guideValue.IsHolding<std::vector<VtValue>>()) {
            unsigned int count = argData.numberOfFlagUses(entry.first.c_str());
            if (!TF_VERIFY(count > 0)) {
                // There should be at least one use if isFlagSet() = true.
                continue;
            }

            std::vector<MArgList> argLists(count);
            for (unsigned int i = 0; i < count; ++i) {
                argData.getFlagArgumentList(key.c_str(), i, argLists[i]);
            }

            // The flag is either 1-arg or multi-arg. If it's 1-arg, make this
            // a 1-d vector [arg, arg, ...]. If it's multi-arg, make this a
            // 2-d vector [[arg1, arg2, ...], [arg1, arg2, ...], ...].
            std::vector<VtValue> val;
            if (argLists[0].length() == 1) {
                for (const MArgList& argList : argLists) {
                    const std::string arg = argList.asString(0).asChar();
                    val.emplace_back(arg);
                }
            } else {
                for (const MArgList& argList : argLists) {
                    std::vector<VtValue> subList;
                    for (unsigned int i = 0; i < argList.length(); ++i) {
                        const std::string arg = argList.asString(i).asChar();
                        subList.emplace_back(arg);
                    }
                    val.emplace_back(subList);
                }
            }
            args[key] = val;
        } else {
            TF_CODING_ERROR("Can't handle type '%s'", guideValue.GetTypeName().c_str());
        }
    }

    return args;
}

namespace {
VtValue _ParseArgumentValue(const JsValue& jsValue, const VtValue& guideValue)
{
    if (guideValue.IsHolding<bool>()) {
        if (jsValue.GetType() == JsValue::StringType) {
            return VtValue(TfUnstringify<bool>(jsValue.GetString()));
        } else {
            return VtValue();
        }
    } else if (guideValue.IsHolding<std::string>()) {
        if (jsValue.GetType() == JsValue::StringType) {
            return VtValue(jsValue.GetString());
        } else {
            return VtValue();
        }
    } else if (guideValue.IsHolding<std::vector<VtValue>>()) {
        if (jsValue.GetType() == JsValue::ArrayType) {
            const JsArray&       jsArray = jsValue.GetJsArray();
            std::vector<VtValue> guideVector = guideValue.Get<std::vector<VtValue>>();
            std::vector<VtValue> vtValue;
            if (guideVector.empty() && !jsArray.empty()) {
                TF_CODING_ERROR("Guide vector is empty. Can not recursively parse.");
                return VtValue(vtValue);
            }
            for (const JsValue& jsItem : jsArray) {
                vtValue.push_back(_ParseArgumentValue(jsItem, guideVector.front()));
            }
            return VtValue(vtValue);
        } else {
            return VtValue();
        }
    }
    return VtValue();
}

VtValue _ParseArgumentValue(const std::string& value, const VtValue& guideValue)
{
    // The export UI only has boolean and string parameters.
    if (guideValue.IsHolding<bool>()) {
        return VtValue(TfUnstringify<bool>(value));
    } else if (guideValue.IsHolding<std::string>()) {
        return VtValue(value);
    } else if (guideValue.IsHolding<std::vector<VtValue>>()) {
        // To prevent quoting issues in MEL scripts, we expect a string that is
        // completely unquoted, containing an array of booleans, tokens, or
        // other arrays:
        //       [0,1,true,false]
        //       [none,default]
        //       [[useRegistry,UsdPreviewSurface],[displayColors,default]]
        // So, to be able to parse this as valid JSON, we need to add quotes at
        // each "[[," to alphanumeric transition.
        // NOTE: The array contains *tokens*, not freeform strings. So we can
        //       skip spaces and consider all commas to be element separators.
        bool        wasArrayDelimiter = true;
        std::string quotedString;
        for (auto c = value.cbegin(); c != value.cend(); ++c) {
            if (*c == ' ') {
                continue;
            }
            bool isArrayDelimiter = *c == '[' || *c == ']' || *c == ',';
            if (isArrayDelimiter != wasArrayDelimiter) {
                quotedString += "\"";
                wasArrayDelimiter = isArrayDelimiter;
            }
            quotedString += *c;
        }

        JsParseError  jsError;
        const JsValue jsValue = JsParseString(quotedString, &jsError);
        if (!jsValue) {
            TF_CODING_ERROR(
                "Failed to parse vector parameter '%s'. Invalid JSON: '%s'",
                value.c_str(),
                jsError.reason.c_str());
            return VtValue();
        }
        return _ParseArgumentValue(jsValue, guideValue);
    }

    return VtValue();
}
} // namespace

VtValue UsdMayaUtil::ParseArgumentValue(
    const std::string&  key,
    const std::string&  value,
    const VtDictionary& guideDict)
{
    // We handle two types of arguments:
    // 1 - bools: Should be encoded by translator UI as a "1" or "0" string.
    // 2 - strings: Just strings!
    // 3 - vectors: We expect [token1,token2] or [[token1,token2],[t3,t4]]
    //     tokens are unquoted alphanumeric strings
    //       vector<vector<string>> is passed for shadingMode
    auto iter = guideDict.find(key);
    if (iter != guideDict.end()) {
        const VtValue& guideValue = iter->second;
        return _ParseArgumentValue(value, guideValue);
    } else {
        TF_CODING_ERROR("Unknown flag '%s'", key.c_str());
    }

    return VtValue();
}

std::pair<bool, std::string> UsdMayaUtil::ValueToArgument(const VtValue& value)
{
    if (value.IsHolding<bool>()) {
        return std::make_pair(true, std::string(value.Get<bool>() ? "1" : "0"));
    } else if (value.IsHolding<std::string>()) {
        return std::make_pair(true, value.Get<std::string>());
    } else if (value.IsHolding<std::vector<VtValue>>()) {
        std::string arrayValue { "[" };
        bool        firstElement = true;
        for (auto const& elemValue : value.Get<std::vector<VtValue>>()) {
            if (firstElement) {
                firstElement = false;
            } else {
                arrayValue += ",";
            }
            bool        canConvert;
            std::string elemString;
            std::tie(canConvert, elemString) = ValueToArgument(elemValue);
            if (canConvert) {
                arrayValue += elemString;
            } else {
                return std::make_pair(false, std::string());
            }
        }
        arrayValue += "]";
        return std::make_pair(true, arrayValue);
    }
    return std::make_pair(false, std::string());
}

std::vector<std::string> UsdMayaUtil::GetAllAncestorMayaNodeTypes(const std::string& ty)
{
    const MString inheritedTypesMel
        = TfStringPrintf("nodeType -isTypeName -inherited %s", ty.c_str()).c_str();
    MStringArray inheritedTypes;
    if (!MGlobal::executeCommand(inheritedTypesMel, inheritedTypes, false, false)) {
        TF_RUNTIME_ERROR(
            "Failed to query ancestor types of '%s' via MEL (does the type "
            "exist?)",
            ty.c_str());
        return std::vector<std::string>();
    }

    std::vector<std::string> inheritedTypesVector;
    inheritedTypesVector.reserve(inheritedTypes.length());
    for (unsigned int i = 0; i < inheritedTypes.length(); ++i) {
        inheritedTypesVector.emplace_back(inheritedTypes[i].asChar());
    }
    return inheritedTypesVector;
}

bool UsdMayaUtil::FindAncestorSceneAssembly(const MDagPath& dagPath, MDagPath* assemblyPath)
{
    MDagPath currentPath(dagPath);
    while (currentPath.length()) {
        if (currentPath.hasFn(MFn::kAssembly)) {
            if (assemblyPath) {
                *assemblyPath = currentPath;
            }
            return true;
        }
        currentPath.pop();
    }
    return false;
}

MBoundingBox UsdMayaUtil::GetInfiniteBoundingBox()
{
    constexpr double inf = std::numeric_limits<double>::infinity();
    return MBoundingBox(MPoint(-inf, -inf, -inf), MPoint(inf, inf, inf));
}

MString UsdMayaUtil::convert(const TfToken& token)
{
    return MString(token.GetText(), token.size());
}

std::string UsdMayaUtil::convert(const MString& str)
{
    return std::string(str.asChar(), str.length());
}

MString UsdMayaUtil::convert(const std::string& str)
{
    return MString(str.data(), static_cast<int>(str.size()));
}

MDagPath UsdMayaUtil::getDagPath(const MFnDependencyNode& depNodeFn, const bool reportError)
{
    try {
        const MFnDagNode& dagNodeFn = dynamic_cast<const MFnDagNode&>(depNodeFn);

        MStatus        status;
        const MDagPath dagPath = dagNodeFn.dagPath(&status);
        if (status == MS::kSuccess) {
            const bool dagPathIsValid = dagPath.isValid(&status);
            if (status == MS::kSuccess && dagPathIsValid) {
                return dagPath;
            }
        }

        if (reportError) {
            TF_CODING_ERROR(
                "Invalid MDagPath for MFnDagNode '%s'. Verify that it was "
                "constructed using an MDagPath.",
                dagNodeFn.fullPathName().asChar());
        }
    } catch (const std::bad_cast& /* e */) {
        // This is not a DAG node, so it can't have a DAG path.
    }

    return MDagPath();
}

UsdMayaUtil::MDagPathMap<SdfPath>
UsdMayaUtil::getDagPathMap(const MFnDependencyNode& depNodeFn, const SdfPath& usdPath)
{
    const MDagPath dagPath = UsdMayaUtil::getDagPath(depNodeFn, /* reportError = */ false);
    if (dagPath.isValid()) {
        return UsdMayaUtil::MDagPathMap<SdfPath>({ { dagPath, usdPath } });
    }

    return UsdMayaUtil::MDagPathMap<SdfPath>({});
}

VtIntArray UsdMayaUtil::shiftIndices(const VtIntArray& array, int shift)
{
    VtIntArray output(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
        output[i] = std::max(0, array[i] + shift);
    }
    return output;
}

VtValue UsdMayaUtil::pushFirstValue(const VtValue& arr, const VtValue& defaultValue)
{
    if (arr.IsHolding<VtArray<float>>() && defaultValue.IsHolding<float>()) {
        return UsdMayaUtil::PushFirstValue(
            arr.UncheckedGet<VtArray<float>>(), defaultValue.UncheckedGet<float>());
    } else if (arr.IsHolding<VtArray<GfVec2f>>() && defaultValue.IsHolding<GfVec2f>()) {
        return UsdMayaUtil::PushFirstValue(
            arr.UncheckedGet<VtArray<GfVec2f>>(), defaultValue.UncheckedGet<GfVec2f>());
    } else if (arr.IsHolding<VtArray<GfVec3f>>() && defaultValue.IsHolding<GfVec3f>()) {
        return UsdMayaUtil::PushFirstValue(
            arr.UncheckedGet<VtArray<GfVec3f>>(), defaultValue.UncheckedGet<GfVec3f>());
    } else if (arr.IsHolding<VtArray<GfVec4f>>() && defaultValue.IsHolding<GfVec4f>()) {
        return UsdMayaUtil::PushFirstValue(
            arr.UncheckedGet<VtArray<GfVec4f>>(), defaultValue.UncheckedGet<GfVec4f>());
    }

    TF_CODING_ERROR("Unsupported type");
    return VtValue();
};

VtValue UsdMayaUtil::popFirstValue(const VtValue& arr)
{
    if (arr.IsHolding<VtArray<float>>()) {
        return UsdMayaUtil::PopFirstValue(arr.UncheckedGet<VtArray<float>>());
    } else if (arr.IsHolding<VtArray<GfVec2f>>()) {
        return UsdMayaUtil::PopFirstValue(arr.UncheckedGet<VtArray<GfVec2f>>());
    } else if (arr.IsHolding<VtArray<GfVec3f>>()) {
        return UsdMayaUtil::PopFirstValue(arr.UncheckedGet<VtArray<GfVec3f>>());
    } else if (arr.IsHolding<VtArray<GfVec4f>>()) {
        return UsdMayaUtil::PopFirstValue(arr.UncheckedGet<VtArray<GfVec4f>>());
    }

    TF_CODING_ERROR("Unsupported type");
    return VtValue();
};

bool UsdMayaUtil::containsUnauthoredValues(const VtIntArray& indices)
{
    return std::any_of(indices.begin(), indices.end(), [](const int& i) { return i < 0; });
}

MDagPath UsdMayaUtil::nameToDagPath(const std::string& name)
{
    MSelectionList selection;
    selection.add(MString(name.c_str()));
    MDagPath dag;
    MStatus  status = selection.getDagPath(0, dag);
    CHECK_MSTATUS(status);
    return dag;
}

void UsdMayaUtil::GetFilteredSelectionToExport(
    bool                      exportSelected,
    MSelectionList&           objectList,
    UsdMayaUtil::MDagPathSet& dagPaths)
{
    dagPaths.clear();

    bool filterInput = true;

    // There are three cases depending on the input:
    // If exportSelected is true then we will grab the active selection
    // If objectList is empty then we will grab all immediate children of the world root.
    // Else there was a populated list of objects to use, most likely passed explicitly to the
    // command.
    if (exportSelected) {
        MGlobal::getActiveSelectionList(objectList);
    } else if (objectList.isEmpty()) {
        objectList.add("|*", true);
        // By construction, the list will only include the single top level objects
        // when we get the input list with |*, so no need to filter selection.
        filterInput = false;
    }

    unsigned int nbObj = objectList.length();
    if (0 == nbObj) {
        return;
    }

    MStatus status;

    // Easiest way to filter by hierarchy is to:
    // 1. Put the input into a set that is sorted by distance from the world root.
    // 2. For each input object we iterate up its hierarchy checking if any parent is in the set.
    // 3. If no parent is in the set then we can add it.
    UsdMayaUtil::MDagPathSet sortedInput;
    for (unsigned int i = 0; i < nbObj; i++) {
        MDagPath dagPath;
        status = objectList.getDagPath(i, dagPath);
        if (status == MS::kSuccess)
            sortedInput.emplace(dagPath);
    }

    for (auto pIter : sortedInput) {
        if (!filterInput || shouldAddToSet(pIter, dagPaths))
            dagPaths.emplace(pIter);
    }
}

double UsdMayaUtil::ConvertMTimeUnitToDouble(const MTime::Unit& unit)
{
    double ret = 0.0;
    switch (unit) {
    case MTime::k2FPS: {
        ret = 2.0;
    } break;
    case MTime::k3FPS: {
        ret = 3.0;
    } break;
    case MTime::k4FPS: {
        ret = 4.0;
    } break;
    case MTime::k5FPS: {
        ret = 5.0;
    } break;
    case MTime::k6FPS: {
        ret = 6.0;
    } break;
    case MTime::k8FPS: {
        ret = 8.0;
    } break;
    case MTime::k10FPS: {
        ret = 10.0;
    } break;
    case MTime::k12FPS: {
        ret = 12.0;
    } break;
    case MTime::k15FPS: {
        ret = 15.0;
    } break;
    case MTime::k16FPS: {
        ret = 16.0;
    } break;
    case MTime::k20FPS: {
        ret = 20.0;
    } break;
    case MTime::k23_976FPS: {
        ret = (24.0 * 1000.0) / 1001.0;
    } break;
    case MTime::k24FPS: {
        ret = 24.0;
    } break;
    case MTime::k25FPS: {
        ret = 25.0;
    } break;
    case MTime::k29_97FPS: {
        ret = (30.0 * 1000.0) / 1001.0;
    } break;
    case MTime::k29_97DF: {
        ret = (30.0 * 1000.0) / 1001.0;
    } break;
    case MTime::k30FPS: {
        ret = 30.0;
    } break;
    case MTime::k40FPS: {
        ret = 40.0;
    } break;
    case MTime::k47_952FPS: {
        ret = (48.0 * 1000.0) / 1001.0;
    } break;
    case MTime::k48FPS: {
        ret = 48.0;
    } break;
    case MTime::k50FPS: {
        ret = 50.0;
    } break;
    case MTime::k59_94FPS: {
        ret = (60.0 * 1000.0) / 1001.0;
    } break;
    case MTime::k60FPS: {
        ret = 60.0;
    } break;
    case MTime::k75FPS: {
        ret = 75.0;
    } break;
    case MTime::k80FPS: {
        ret = 80.0;
    } break;
#if MAYA_API_VERSION >= 20200000
    case MTime::k90FPS: {
        ret = 90.0;
    } break;
#endif
    case MTime::k100FPS: {
        ret = 100.0;
    } break;
    case MTime::k120FPS: {
        ret = 120.0;
    } break;
    case MTime::k125FPS: {
        ret = 125.0;
    } break;
    case MTime::k150FPS: {
        ret = 150.0;
    } break;
    case MTime::k200FPS: {
        ret = 200.0;
    } break;
    case MTime::k240FPS: {
        ret = 240.0;
    } break;
    case MTime::k250FPS: {
        ret = 250.0;
    } break;
    case MTime::k300FPS: {
        ret = 300.0;
    } break;
    case MTime::k375FPS: {
        ret = 375.0;
    } break;
    case MTime::k400FPS: {
        ret = 400.0;
    } break;
    case MTime::k500FPS: {
        ret = 500.0;
    } break;
    case MTime::k600FPS: {
        ret = 600.0;
    } break;
    case MTime::k750FPS: {
        ret = 750.0;
    } break;
    case MTime::k1200FPS: {
        ret = 1200.0;
    } break;
    case MTime::k1500FPS: {
        ret = 1500.0;
    } break;
    case MTime::k2000FPS: {
        ret = 2000.0;
    } break;
    case MTime::k3000FPS: {
        ret = 3000.0;
    } break;
    case MTime::k6000FPS: {
        ret = 6000.0;
    } break;
    case MTime::k44100FPS: {
        ret = 44100.0;
    } break;
    case MTime::k48000FPS: {
        ret = 48000.0;
    } break;
    case MTime::kHours: {
        ret = (1.0 / 3600.0);
    } break;
    case MTime::kMinutes: {
        ret = (1.0 / 60.0);
    } break;
    case MTime::kSeconds: {
        ret = 1.0;
    } break;
    case MTime::kMilliseconds: {
        ret = 1000.0;
    } break;
    default: {
        ret = 0.0;
    } break;
    }
    return ret;
}

double UsdMayaUtil::GetSceneMTimeUnitAsDouble()
{
    const MTime::Unit sceneUnit = MTime::uiUnit();
    return UsdMayaUtil::ConvertMTimeUnitToDouble(sceneUnit);
}

bool UsdMayaUtil::mayaSearchMIntArray(const int a, const MIntArray& array, unsigned int* idx)
{
    for (unsigned int i = 0; i < array.length(); ++i) {
        if (array[i] == a) {
            if (idx != nullptr) {
                *idx = i;
            }
            return true;
        }
    }
    if (idx != nullptr) {
        *idx = -1;
    }
    return false;
}

MStatus UsdMayaUtil::GetAllIndicesFromComponentListDataPlug(const MPlug& plg, MIntArray& indices)
{
    MStatus     status;
    MDataHandle dh = plg.asMDataHandle(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MObject indicesData = dh.data();
    if (indicesData.isNull() || !indicesData.hasFn(MFn::kComponentListData)) {
        return MStatus::kFailure;
    }
    MFnComponentListData fnComponentListData(indicesData, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    indices.clear();
    unsigned int numIndices = fnComponentListData.length();
    if (numIndices == 0) {
        return MStatus::kSuccess;
    }
    for (unsigned int i = 0; i < numIndices; ++i) {
        MObject                   curComponent = fnComponentListData[i];
        MFnSingleIndexedComponent fnSingleIndexedComponent(curComponent, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        MIntArray curIndices;
        status = fnSingleIndexedComponent.getElements(curIndices);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        for (unsigned int j = 0; j < curIndices.length(); ++j) {
            indices.append(curIndices[j]);
        }
    }

    return status;
}

bool UsdMayaUtil::CheckMeshUpstreamForBlendShapes(const MObject& mesh)
{
    MStatus stat;
    if (!MObjectHandle(mesh).isValid()) {
        return false;
    }
    MObject            searchObj = MObject(mesh);
    MItDependencyGraph itDg(
        searchObj,
        MFn::kBlendShape,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &stat);
    CHECK_MSTATUS_AND_RETURN(stat, false);
    for (; !itDg.isDone(); itDg.next()) {
        MObject curBlendShape = itDg.currentItem();
        if (curBlendShape.hasFn(MFn::kBlendShape)) {
            return true;
        }
    }

    return false;
}
