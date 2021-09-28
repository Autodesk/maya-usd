//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "AL/usdmaya/utils/AnimationTranslator.h"

#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/MeshUtils.h"

#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MMatrix.h>
#include <maya/MNodeClass.h>
#include <maya/MProfiler.h>

#include <iostream>
namespace {
const int _animationTranslatorProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "AnimationTranslator",
    "AnimationTranslator"
#else
    "AnimationTranslator"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace utils {

const static std::array<MFn::Type, 4> g_nodeTypesConsiderToBeAnimation {
    MFn::kAnimCurveTimeToAngular,  // 79
    MFn::kAnimCurveTimeToDistance, // 80
    MFn::kAnimCurveTimeToTime,     // 81
    MFn::kAnimCurveTimeToUnitless  // 82
};

//----------------------------------------------------------------------------------------------------------------------
const static AnimationCheckTransformAttributes g_AnimationCheckTransformAttributes;

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::considerToBeAnimation(const MFn::Type nodeType)
{
    return std::binary_search(
        g_nodeTypesConsiderToBeAnimation.cbegin(),
        g_nodeTypesConsiderToBeAnimation.cend(),
        nodeType);
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::isAnimated(MPlug attr, const bool assumeExpressionIsAnimated)
{
    if (attr.isArray()) {
        const uint32_t numElements = attr.numElements();
        for (uint32_t i = 0; i < numElements; ++i) {
            if (isAnimated(attr.elementByLogicalIndex(i), assumeExpressionIsAnimated)) {
                return true;
            }
        }
        return false;
    }

    if (attr.isCompound()) {
        const uint32_t numChildren = attr.numChildren();
        for (uint32_t i = 0; i < numChildren; ++i) {
            if (isAnimated(attr.child(i), assumeExpressionIsAnimated)) {
                return true;
            }
        }
    }

    // is no connections exist, it cannot be animated
    if (!attr.isConnected()) {
        return false;
    } else {
        MPlugArray plugs;
        if (!attr.connectedTo(plugs, true, false)) {
            return false;
        }

        // test to see if we are directly connected to an animation curve
        // or we have some special source attributes.
        int32_t i = 0, n = plugs.length();
        for (; i < n; ++i) {
            // Test if we have animCurves:
            MObject         connectedNode = plugs[i].node();
            const MFn::Type connectedNodeType = connectedNode.apiType();
            if (considerToBeAnimation(connectedNodeType)) {
                // I could use some slightly better heuristics here.
                // If there are 2 or more keyframes on this curve, assume its value changes.
                MFnAnimCurve curve(connectedNode);
                if (curve.numKeys() > 1) {
                    return true;
                }
            } else {
                break;
            }
        }

        // if all connected nodes are anim curves, and all have 1 or zero keys, the plug is not
        // animated.
        if (i == n) {
            return false;
        }
    }

    // if we get here, recurse through the upstream connections looking for a time or expression
    // node
    MStatus            status;
    MItDependencyGraph iter(
        attr,
        MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &status);

    if (!status) {
        MGlobal::displayError("Unable to create DG iterator");
        return false;
    }

    for (; !iter.isDone(); iter.next()) {
        MObject currNode = iter.thisPlug().node();
        if (currNode.hasFn(MFn::kTime)) {
            return true;
        }
        if (assumeExpressionIsAnimated && currNode.hasFn(MFn::kExpression)) {
            return true;
        }
        if ((currNode.hasFn(MFn::kTransform) || currNode.hasFn(MFn::kPluginTransformNode))
            && MAnimUtil::isAnimated(currNode, true)) {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::isAnimatedMesh(const MDagPath& mesh)
{
    if (MAnimUtil::isAnimated(mesh, true)) {
        return true;
    }
    MStatus            status;
    MObject            node = mesh.node();
    MItDependencyGraph iter(
        node,
        MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &status);

    if (!status) {
        MGlobal::displayError("Unable to create DG iterator");
        return false;
    }
    iter.setTraversalOverWorldSpaceDependents(true);

    for (; !iter.isDone(); iter.next()) {
        MObject currNode = iter.thisPlug().node();
        if (((currNode.hasFn(MFn::kTransform) || currNode.hasFn(MFn::kPluginTransformNode))
             && MAnimUtil::isAnimated(currNode, true))
            || currNode.hasFn(MFn::kTime)) {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::inheritTransform(const MDagPath& path)
{
    MStatus       status;
    const MObject transformNode = path.node(&status);
    if (status != MS::kSuccess)
        return false;

    MPlug inheritTransformPlug(
        transformNode, g_AnimationCheckTransformAttributes.inheritTransformAttribute());
    return inheritTransformPlug.asBool();
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::areTransformAttributesConnected(const MDagPath& path)
{
    MStatus       status;
    const MObject transformNode = path.node(&status);
    if (status != MS::kSuccess)
        return false;

    for (const auto& attributeObject : g_AnimationCheckTransformAttributes) {
        const MPlug plug(transformNode, attributeObject);
        if (plug.isDestination(&status))
            return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool AnimationTranslator::isAnimatedTransform(const MObject& transformNode)
{
    if (!transformNode.hasFn(MFn::kTransform))
        return false;

    MStatus    status;
    MFnDagNode fnNode(transformNode, &status);
    if (status != MS::kSuccess)
        return false;

    MDagPath currPath;
    fnNode.getPath(currPath);

    bool transformAttributeConnected = areTransformAttributesConnected(currPath);

    if (transformAttributeConnected)
        return true;
    else if (!inheritTransform(currPath))
        return false;

    while (currPath.pop() == MStatus::kSuccess && currPath.hasFn(MFn::kTransform)
           && inheritTransform(currPath)) {
        if (areTransformAttributesConnected(currPath)) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AnimationCheckTransformAttributes::AnimationCheckTransformAttributes()
{
    if (initialise() != MS::kSuccess) {
        std::cerr << "Unable to initialize common transform attributes for animation test."
                  << std::endl;
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus AnimationCheckTransformAttributes::initialise()
{
    MNodeClass        transformNodeClass("transform");
    MStatus           status;
    const char* const errorString = "Unable to extract attribute for Transform class.";
    m_commonTransformAttributes[0] = transformNodeClass.attribute("translate", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[1] = transformNodeClass.attribute("translateX", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[2] = transformNodeClass.attribute("translateY", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[3] = transformNodeClass.attribute("translateZ", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[4] = transformNodeClass.attribute("rotate", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[5] = transformNodeClass.attribute("rotateX", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[6] = transformNodeClass.attribute("rotateY", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[7] = transformNodeClass.attribute("rotateZ", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[8] = transformNodeClass.attribute("scale", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[9] = transformNodeClass.attribute("scaleX", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[10] = transformNodeClass.attribute("scaleY", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[11] = transformNodeClass.attribute("scaleZ", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    m_commonTransformAttributes[12] = transformNodeClass.attribute("rotateOrder", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);

    m_inheritTransformAttribute = transformNodeClass.attribute("inheritsTransform", &status);
    AL_MAYA_CHECK_ERROR(status, errorString);

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
