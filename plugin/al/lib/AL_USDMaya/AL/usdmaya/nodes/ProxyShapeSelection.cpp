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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Scope.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include <maya/MFnDagNode.h>
#include <maya/MProfiler.h>
#include <maya/MPxCommand.h>

#include <cinttypes>

namespace AL {
namespace usdmaya {
namespace nodes {
namespace {

const int _proxyShapeSelectionProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "AL_usdmaya_ProxyShape_selection",
    "AL_usdmaya_ProxyShape_selection"
#else
    "AL_usdmaya_ProxyShape_selection"
#endif
);

typedef void (
    *proxy_function_prototype)(void* userData, AL::usdmaya::nodes::ProxyShape* proxyInstance);
} // namespace

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::printRefCounts() const
{
    for (auto it = m_requiredPaths.begin(); it != m_requiredPaths.end(); ++it) {
        std::cout << it->first.GetText() << " (" << MFnDagNode(it->second.node()).typeName()
                  << ") :- ";
        it->second.printRefCounts();
    }
}

//----------------------------------------------------------------------------------------------------------------------
inline bool ProxyShape::TransformReference::decRef(const TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::decRef %" PRIu16 " %" PRIu16 " %" PRIu16 "\n",
            m_selected,
            m_refCount,
            m_required);
    switch (reason) {
    case kSelection:
        if (m_selected > 0)
            --m_selected;
        break;
    case kRequested:
        if (m_refCount > 0)
            --m_refCount;
        break;
    case kRequired:
        if (m_required > 0)
            --m_required;
        break;
    default: assert(0); break;
    }

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::decRefEnd %" PRIu16 " %" PRIu16 " %" PRIu16
            "\n",
            m_selected,
            m_refCount,
            m_required);
    return !m_required && !m_selected && !m_refCount;
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::TransformReference::incRef(const TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::incRef %" PRIu16 " %" PRIu16 " %" PRIu16 "\n",
            m_selected,
            m_refCount,
            m_required);
    switch (reason) {
    case kSelection: ++m_selected; break;
    case kRequested: ++m_refCount; break;
    case kRequired: ++m_required; break;
    default: assert(0); break;
    }

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::incRefEnd %" PRIu16 " %" PRIu16 " %" PRIu16
            "\n",
            m_selected,
            m_refCount,
            m_required);
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::TransformReference::checkIncRef(const TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::checkIncRef %" PRIu16 " %" PRIu16 " %" PRIu16
            "\n",
            m_selected,
            m_refCount,
            m_required);
    switch (reason) {
    case kSelection: ++m_selectedTemp; break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
inline bool ProxyShape::TransformReference::checkRef(const TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::TransformReference::checkRef %" PRIu16 " : %" PRIu16 " %" PRIu16
            " %" PRIu16 "\n",
            m_selectedTemp,
            m_selected,
            m_refCount,
            m_required);
    uint32_t sl = m_selected;
    uint32_t rc = m_refCount;
    uint32_t rq = m_required;

    switch (reason) {
    case kSelection:
        assert(m_selected);
        --sl;
        --m_selectedTemp;
        break;
    case kRequested:
        assert(m_refCount);
        --rc;
        break;
    case kRequired:
        assert(m_required);
        --rq;
        break;
    default: assert(0); break;
    }
    return !rq && !m_selectedTemp && !rc;
}

//----------------------------------------------------------------------------------------------------------------------
inline ProxyShape::TransformReference::TransformReference(
    const MObject&        node,
    const TransformReason reason)
    : m_node(node)
    , m_transform(nullptr)
{
    m_required = 0;
    m_selected = 0;
    m_selectedTemp = 0;
    m_refCount = 0;
    m_transform = getTransformNode();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::makeTransformReference(
    const SdfPath&  path,
    const MObject&  node,
    TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::makeTransformReference %s\n", path.GetText());

    SdfPath       tempPath = path;
    MDagPath      dagPath;
    MStatus       status;
    MObjectHandle handle(node);
    if (handle.isAlive() && handle.isValid()) {
        MFnDagNode fn(node, &status);
        status = fn.getPath(dagPath);
        while (tempPath != SdfPath("/")) {
            MObject tempNode = dagPath.node(&status);
            auto    existing = m_requiredPaths.find(tempPath);
            if (existing != m_requiredPaths.end()) {
                existing->second.incRef(reason);
            } else {
                TransformReference ref(tempNode, reason);
                ref.incRef(reason);
                m_requiredPaths.emplace(tempPath, ref);
                TF_DEBUG(ALUSDMAYA_SELECTION)
                    .Msg(
                        "ProxyShapeSelection::makeTransformReference m_requiredPaths added "
                        "TransformReference: %s\n",
                        tempPath.GetText());
            }
            status = dagPath.pop();
            tempPath = tempPath.GetParentPath();
        }
    } else {
        while (tempPath != SdfPath("/")) {
            auto existing = m_requiredPaths.find(tempPath);
            if (existing != m_requiredPaths.end()) {
                existing->second.incRef(reason);
            } else {
                MGlobal::displayError(
                    "invalid MObject encountered when making transform reference");
            }
            tempPath = tempPath.GetParentPath();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::prepSelect()
{
    for (auto& iter : m_requiredPaths) {
        iter.second.prepSelect();
    }
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain_internal(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason,
    MDGModifier*    modifier2,
    uint32_t*       createCount,
    MString*        resultingPath,
    bool            pushToPrim,
    bool            readAnimatedValues)
{
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::makeUsdTransformChain_internal\n");

    const MPlug outTimeAttr = outTimePlug();
    const MPlug outStageAttr = outStageDataPlug();

    // makes the assumption that instancing isn't supported.
    MFnDagNode    fn(thisMObject());
    const MObject parent = fn.parent(0);
    auto          ret = makeUsdTransformChain(
        usdPrim,
        outStageAttr,
        outTimeAttr,
        parent,
        modifier,
        reason,
        modifier2,
        createCount,
        resultingPath,
        pushToPrim,
        readAnimatedValues);
    return ret;
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason,
    MDGModifier*    modifier2,
    uint32_t*       createCount,
    bool            pushToPrim,
    bool            readAnimatedValues)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Make Usd transform chain");

    if (!usdPrim) {
        return MObject::kNullObj;
    }

    // special case for selection. Do not allow duplicate paths to be selected.
    if (reason == kSelection) {
        auto insertResult = m_selectedPaths.insert(usdPrim.GetPath());
        if (!insertResult.second) {
            TransformReferenceMap::iterator previous = m_requiredPaths.find(usdPrim.GetPath());
            return previous->second.node();
        }
    }

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("ProxyShape::makeUsdTransformChain on %s\n", usdPrim.GetPath().GetText());
    MObject newNode = makeUsdTransformChain_internal(
        usdPrim, modifier, reason, modifier2, createCount, 0, pushToPrim, readAnimatedValues);
    insertTransformRefs({ std::pair<SdfPath, MObject>(usdPrim.GetPath(), newNode) }, reason);
    return newNode;
}

static void createMayaNode(
    const UsdPrim& usdPrim,
    MObject&       node,
    const MObject& parentNode,
    MDagModifier&  modifier,
    MDGModifier*   modifier2,
    const MPlug&   outStage,
    const MPlug&   outTime,
    bool           pushToPrim,
    bool           readAnimatedValues)
{
    MFnDagNode fn;

    bool isUsdXFormable = usdPrim.IsA<UsdGeomXformable>() && !usdPrim.IsInstanceProxy();

    enum MayaNodeTypeCreated
    {
        customMayaTransforn,
        AL_USDMayaScope,
        AL_USDMayaTransform
    };

    std::string transformType;

    MayaNodeTypeCreated createdNodeType = AL_USDMayaTransform;

    bool hasMetadata = usdPrim.GetMetadata(Metadata::transformType, &transformType);
    if (hasMetadata && !transformType.empty()) {
        TF_DEBUG(ALUSDMAYA_SELECTION)
            .Msg("ProxyShapeSelection::createMayaNode: creating custom transform node\n");
        node = modifier.createNode(AL::maya::utils::convert(transformType), parentNode);
        createdNodeType = customMayaTransforn;
    } else if (usdPrim.IsA<UsdGeomScope>()) {
        TF_DEBUG(ALUSDMAYA_SELECTION)
            .Msg("ProxyShapeSelection::createMayaNode: creating scope node\n");
        node = modifier.createNode(Scope::kTypeId, parentNode);
        transformType = "AL_usdmaya_Scope";
        createdNodeType = AL_USDMayaScope;
    } else {
        // for any other USD type, we'll create an AL_USDMaya Transform node
        TF_DEBUG(ALUSDMAYA_SELECTION)
            .Msg("ProxyShapeSelection::createMayaNode: creating transform node\n");
        node = modifier.createNode(Transform::kTypeId, parentNode);
        transformType = "AL_usdmaya_Transform";
        createdNodeType = AL_USDMayaTransform;
    }

    if (fn.setObject(node)) {
        TF_DEBUG(ALUSDMAYA_SELECTION)
            .Msg(
                "ProxyShape::createMayaNode created transformType=%s name=%s\n",
                transformType.c_str(),
                usdPrim.GetName().GetText());
    } else {
        MString err;
        err.format(
            "USDMaya is unable to create the node with transformType=^1s, name=^2s.",
            transformType.c_str(),
            usdPrim.GetName().GetText());
        MGlobal::displayError(err);
    }

    fn.setName(AL::maya::utils::convert(usdPrim.GetName().GetString()));
    const SdfPath path { usdPrim.GetPath() };

    if (createdNodeType == AL_USDMayaTransform) {
        Transform* ptrNode = (Transform*)fn.userNode();
        MPlug      inStageData = ptrNode->inStageDataPlug();
        MPlug      inTime = ptrNode->timePlug();

        if (modifier2) {
            if (pushToPrim)
                modifier2->newPlugValueBool(ptrNode->pushToPrimPlug(), pushToPrim);
            modifier2->newPlugValueBool(ptrNode->readAnimatedValuesPlug(), readAnimatedValues);
        }

        if (!isUsdXFormable) {
            // It's not a USD Xform - so no point in allowing this stuff to be writeable
            MPlug(node, MPxTransform::translate).setLocked(true);
            MPlug(node, MPxTransform::rotate).setLocked(true);
            MPlug(node, MPxTransform::scale).setLocked(true);
            MPlug(node, MPxTransform::transMinusRotatePivot).setLocked(true);
            MPlug(node, MPxTransform::rotateAxis).setLocked(true);
            MPlug(node, MPxTransform::scalePivotTranslate).setLocked(true);
            MPlug(node, MPxTransform::scalePivot).setLocked(true);
            MPlug(node, MPxTransform::rotatePivotTranslate).setLocked(true);
            MPlug(node, MPxTransform::rotatePivot).setLocked(true);
            MPlug(node, MPxTransform::shearXY).setLocked(true);
            MPlug(node, MPxTransform::shearXZ).setLocked(true);
            MPlug(node, MPxTransform::shearYZ).setLocked(true);
        } else {
            // only connect time and stage if transform can change
            modifier.connect(outTime, inTime);
            modifier.connect(outStage, inStageData);
        }

        // set the primitive path

        modifier.newPlugValueString(ptrNode->primPathPlug(), path.GetText());
    } else if (createdNodeType == AL_USDMayaScope) {
        Scope* ptrNode = (Scope*)fn.userNode();

        // only connect stage if transform can change
        MPlug inStageData = ptrNode->inStageDataPlug();
        modifier.connect(outStage, inStageData);
        modifier.newPlugValueString(ptrNode->primPathPlug(), path.GetText());
    }
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain(
    UsdPrim         usdPrim,
    const MPlug&    outStage,
    const MPlug&    outTime,
    const MObject&  parentXForm,
    MDagModifier&   modifier,
    TransformReason reason,
    MDGModifier*    modifier2,
    uint32_t*       createCount,
    MString*        resultingPath,
    bool            pushToPrim,
    bool            readAnimatedValues)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::makeUsdTransformChain %s\n", usdPrim.GetPath().GetText());

    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory,
        MProfiler::kColorE_L3,
        "Make Usd transform chain with parent");

    const SdfPath path { usdPrim.GetPath() };

    auto iter = m_requiredPaths.find(path);

    // If this path has been found.
    if (iter != m_requiredPaths.end()) {
        MObject nodeToReturn = iter->second.node();
        switch (reason) {
        case kSelection: {
            while (usdPrim && iter != m_requiredPaths.end()) {
                iter->second.checkIncRef(reason);

                // grab the parent.
                usdPrim = usdPrim.GetParent();

                // if valid, grab reference to path
                if (usdPrim) {
                    iter = m_requiredPaths.find(usdPrim.GetPath());
                }
            };
        } break;

        case kRequested: {
            while (usdPrim && iter != m_requiredPaths.end()) {
                // grab the parent.
                usdPrim = usdPrim.GetParent();

                // if valid, grab reference to path
                if (usdPrim) {
                    iter = m_requiredPaths.find(usdPrim.GetPath());
                }
            };
        } break;

        case kRequired: {
            while (usdPrim && iter != m_requiredPaths.end() && !iter->second.required()) {
                iter->second.checkIncRef(reason);

                // grab the parent.
                usdPrim = usdPrim.GetParent();

                // if valid, grab reference to path
                if (usdPrim) {
                    iter = m_requiredPaths.find(usdPrim.GetPath());
                }
            };
        } break;
        }
        if (resultingPath) {
            MFnDagNode fn(nodeToReturn);
            MDagPath   mpath;
            fn.getPath(mpath);
            *resultingPath = mpath.fullPathName();
        }
        // return the lowest point on the found chain.
        return nodeToReturn;
    }

    MObject parentPath;
    // descend into the parent first
    if (path.GetPathElementCount() > 1) {
        // if there is a parent to this node, continue building the chain.
        parentPath = makeUsdTransformChain(
            usdPrim.GetParent(),
            outStage,
            outTime,
            parentXForm,
            modifier,
            reason,
            modifier2,
            createCount,
            resultingPath,
            pushToPrim,
            readAnimatedValues);
    }

    // if we've hit the top of the chain, make sure we get the correct parent
    if (parentPath == MObject::kNullObj) {
        parentPath = parentXForm;
    }

    if (createCount)
        (*createCount)++;

    MFnDagNode fn;
    MObject    parentNode = parentPath;

    MObject node;

    createMayaNode(
        usdPrim,
        node,
        parentNode,
        modifier,
        modifier2,
        outStage,
        outTime,
        pushToPrim,
        readAnimatedValues);

    if (resultingPath)
        *resultingPath = recordUsdPrimToMayaPath(usdPrim, node);
    else
        recordUsdPrimToMayaPath(usdPrim, node);

    TransformReference transformRef(node, reason);
    transformRef.checkIncRef(reason);
    m_requiredPaths.emplace(path, transformRef);

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::makeUsdTransformChain m_requiredPaths added TransformReference: "
            "%s\n",
            path.GetText());
    return node;
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransforms(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason,
    MDGModifier*    modifier2)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Make Usd transforms");

    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::makeUsdTransforms\n");

    // Ok, so let's go wondering up the transform chain making sure we have all of those transforms
    // created.
    MObject node = makeUsdTransformChain(usdPrim, modifier, reason, modifier2, 0);

    // we only need child transforms if they have been requested
    if (reason == kRequested) {
        makeUsdTransformsInternal(usdPrim, node, modifier, reason, modifier2);
    }

    return node;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::makeUsdTransformsInternal(
    const UsdPrim&  usdPrim,
    const MObject&  parentNode,
    MDagModifier&   modifier,
    TransformReason reason,
    MDGModifier*    modifier2,
    bool            pushToPrim,
    bool            readAnimatedValues)
{
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::makeUsdTransformsInternal\n");
    MFnDagNode fn;

    MPlug outStageAttr = outStageDataPlug();
    MPlug outTimeAttr = outTimePlug();

    for (auto it = usdPrim.GetChildren().begin(), e = usdPrim.GetChildren().end(); it != e; ++it) {
        UsdPrim prim = *it;
        /// must always exist, and never get deleted.
        auto check = m_requiredPaths.find(prim.GetPath());
        if (check == m_requiredPaths.end()) {
            UsdPrim prim = *it;

            MObject node;
            createMayaNode(
                prim,
                node,
                parentNode,
                modifier,
                modifier2,
                outStageAttr,
                outTimeAttr,
                pushToPrim,
                readAnimatedValues);

            TransformReference transformRef(node, reason);
            transformRef.checkIncRef(reason);
            const SdfPath path { usdPrim.GetPath() };
            m_requiredPaths.emplace(path, transformRef);

            TF_DEBUG(ALUSDMAYA_SELECTION)
                .Msg(
                    "ProxyShapeSelection::makeUsdTransformsInternal m_requiredPaths added "
                    "TransformReference: %s\n",
                    path.GetText());

            makeUsdTransformsInternal(
                prim, node, modifier, reason, modifier2, pushToPrim, readAnimatedValues);
        } else {
            makeUsdTransformsInternal(
                prim,
                check->second.node(),
                modifier,
                reason,
                modifier2,
                pushToPrim,
                readAnimatedValues);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain_internal(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory,
        MProfiler::kColorE_L3,
        "Remove Usd transform chain for prim");

    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::removeUsdTransformChain\n");
    UsdPrim parentPrim = usdPrim;
    MObject parentTM = MObject::kNullObj;
    MObject object = MObject::kNullObj;
    while (parentPrim) {
        SdfPath primPath = parentPrim.GetPath();
        auto    it = m_requiredPaths.find(primPath);
        if (it == m_requiredPaths.end()) {
            return;
        }

        if (it->second.checkRef(reason)) {
            MObject object = it->second.node();
            if (object != MObject::kNullObj) {
                modifier.reparentNode(object);
                modifier.deleteNode(object);
            }
        }

        parentPrim = parentPrim.GetParent();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain(
    const SdfPath&  path,
    MDagModifier&   modifier,
    TransformReason reason)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory,
        MProfiler::kColorE_L3,
        "Remove Usd transform chain for path");

    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::removeUsdTransformChain\n");
    SdfPath parentPrim = path;
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShape::removeUsdTransformChain %s\n", path.GetText());
    MObject parentTM = MObject::kNullObj;
    MObject object = MObject::kNullObj;

    while (!parentPrim.IsEmpty()) {
        auto it = m_requiredPaths.find(parentPrim);
        if (it == m_requiredPaths.end()) {
            TF_DEBUG(ALUSDMAYA_SELECTION)
                .Msg("ProxyShape -- %s path has not been found\n", path.GetText());
        } else if (it->second.decRef(reason)) {
            MObject object = it->second.node();
            if (object != MObject::kNullObj) {
                MObjectHandle h = object;

                // The Xform of the shape may have already been deleted when the shape was deleted
                if (h.isAlive() && h.isValid()) {
                    modifier.reparentNode(object);
                    modifier.deleteNode(object);
                }
            }

            m_requiredPaths.erase(it);
            TF_DEBUG(ALUSDMAYA_SELECTION)
                .Msg(
                    "ProxyShapeSelection::removeUsdTransformChain m_requiredPaths removed "
                    "TransformReference: %s\n",
                    it->first.GetText());
        }

        parentPrim = parentPrim.GetParentPath();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::removeUsdTransformChain\n");
    if (!usdPrim) {
        return;
    }

    if (reason == kSelection) {
        auto selectedPath = m_selectedPaths.find(usdPrim.GetPath());
        if (selectedPath != m_selectedPaths.end()) {
            m_selectedPaths.erase(selectedPath);
        } else {
            return;
        }
    }

    UsdPrim parentPrim = usdPrim;
    MObject parentTM = MObject::kNullObj;
    MObject object = MObject::kNullObj;
    while (parentPrim) {
        auto it = m_requiredPaths.find(parentPrim.GetPath());
        if (it == m_requiredPaths.end()) {
            return;
        }

        if (it->second.decRef(reason)) {
            MObject object = it->second.node();
            if (object != MObject::kNullObj) {
                modifier.reparentNode(object);
                modifier.deleteNode(object);
            }

            TF_DEBUG(ALUSDMAYA_SELECTION)
                .Msg(
                    "ProxyShapeSelection::removeUsdTransformChain m_requiredPaths removed "
                    "TransformReference: %s\n",
                    it->first.GetText());
            m_requiredPaths.erase(it);
        }

        parentPrim = parentPrim.GetParent();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformsInternal(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::removeUsdTransformsInternal %s\n", usdPrim.GetPath().GetText());
    // can we find the prim in the current set?
    auto it = m_requiredPaths.find(usdPrim.GetPath());
    if (it == m_requiredPaths.end()) {
        return;
    }

    // first go remove the children
    for (auto iter = usdPrim.GetChildren().begin(), end = usdPrim.GetChildren().end(); iter != end;
         ++iter) {
        removeUsdTransformsInternal(*iter, modifier, ProxyShape::kRequested);
    }

    if (it->second.decRef(reason)) {
        // work around for Maya's love of deleting the parent transforms of custom transform nodes
        // :(
        modifier.reparentNode(it->second.node());
        modifier.deleteNode(it->second.node());
        TF_DEBUG(ALUSDMAYA_SELECTION)
            .Msg(
                "ProxyShapeSelection::removeUsdTransformsInternal m_requiredPaths removed "
                "TransformReference: %s\n",
                it->first.GetText());
        m_requiredPaths.erase(it);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransforms(
    const UsdPrim&  usdPrim,
    MDagModifier&   modifier,
    TransformReason reason)
{
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::removeUsdTransforms\n");

    // can we find the prim in the current set?
    auto it = m_requiredPaths.find(usdPrim.GetPath());
    if (it == m_requiredPaths.end()) {
        return;
    }

    // no need to iterate through children if we are requesting a shape
    if (reason == kRequested) {
        // first go remove the children
        for (auto it = usdPrim.GetChildren().begin(), end = usdPrim.GetChildren().end(); it != end;
             ++it) {
            removeUsdTransformsInternal(*it, modifier, ProxyShape::kRequested);
        }
    }

    // finally walk back up the chain and do magic. I'm not sure I want to do this?
    removeUsdTransformChain(usdPrim, modifier, reason);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::insertTransformRefs(
    const std::vector<std::pair<SdfPath, MObject>>& removedRefs,
    TransformReason                                 reason)
{
    for (auto iter : removedRefs) {
        makeTransformReference(iter.first, iter.second, reason);
    }
}

//----------------------------------------------------------------------------------------------------------------------
SelectionUndoHelper::SelectionUndoHelper(
    nodes::ProxyShape*      proxy,
    const SdfPathHashSet&   paths,
    MGlobal::ListAdjustment mode,
    bool                    internal)
    : m_proxy(proxy)
    , m_paths(paths)
    , m_mode(mode)
    , m_modifier1()
    , m_modifier2()
    , m_insertedRefs()
    , m_removedRefs()
    , m_internal(internal)
{
}

//----------------------------------------------------------------------------------------------------------------------
void SelectionUndoHelper::doIt()
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::SelectionUndoHelper::doIt %lu %lu\n",
            m_insertedRefs.size(),
            m_removedRefs.size());
    m_proxy->m_pleaseIgnoreSelection = true;
    m_modifier1.doIt();
    m_modifier2.doIt();
    m_proxy->insertTransformRefs(m_insertedRefs, nodes::ProxyShape::kSelection);
    m_proxy->removeTransformRefs(m_removedRefs, nodes::ProxyShape::kSelection);
    m_proxy->selectedPaths() = m_paths;
    if (!m_internal) {
        MGlobal::setActiveSelectionList(m_newSelection, MGlobal::kReplaceList);
    }
    m_proxy->m_pleaseIgnoreSelection = false;
    if (m_proxy->isLockPrimFeatureActive()) {
        m_proxy->constructLockPrims();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SelectionUndoHelper::undoIt()
{
    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg(
            "ProxyShapeSelection::SelectionUndoHelper::undoIt %lu %lu\n",
            m_insertedRefs.size(),
            m_removedRefs.size());
    m_proxy->m_pleaseIgnoreSelection = true;
    m_modifier2.undoIt();
    m_modifier1.undoIt();
    m_proxy->insertTransformRefs(m_removedRefs, nodes::ProxyShape::kSelection);
    m_proxy->removeTransformRefs(m_insertedRefs, nodes::ProxyShape::kSelection);
    m_proxy->selectedPaths() = m_previousPaths;
    if (!m_internal) {
        MGlobal::setActiveSelectionList(m_previousSelection, MGlobal::kReplaceList);
    }
    m_proxy->m_pleaseIgnoreSelection = false;
    if (m_proxy->isLockPrimFeatureActive()) {
        m_proxy->constructLockPrims();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeTransformRefs(
    const std::vector<std::pair<SdfPath, MObject>>& removedRefs,
    TransformReason                                 reason)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Remove transform refs");

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::removeTransformRefs %lu\n", removedRefs.size());
    for (auto iter : removedRefs) {
        UsdPrim parentPrim = m_stage->GetPrimAtPath(iter.first);
        while (parentPrim) {
            auto it = m_requiredPaths.find(parentPrim.GetPath());
            if (it != m_requiredPaths.end()) {
                if (it->second.decRef(reason)) {
                    TF_DEBUG(ALUSDMAYA_EVALUATION)
                        .Msg(
                            "ProxyShape::removeTransformRefs m_requiredPaths removed "
                            "TransformReference: %s\n",
                            it->first.GetText());
                    m_requiredPaths.erase(it);
                }
            }

            parentPrim = parentPrim.GetParent();
            if (parentPrim.GetPath() == SdfPath("/")) {
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::removeAllSelectedNodes(SelectionUndoHelper& helper)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Remove all selected nodes");

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::removeAllSelectedNodes %lu\n", m_selectedPaths.size());

    std::vector<TransformReferenceMap::iterator> toRemove;

    auto it = m_requiredPaths.begin();
    auto end = m_requiredPaths.end();
    for (; it != end; ++it) {
        // decrement the ref count. If this comes back as 'please remove'
        if (it->second.checkRef(ProxyShape::kSelection)) {
            // add it to the list of things to kill
            toRemove.push_back(it);
        }
    }

    if (toRemove.size() > 1) {
        // sort the array of iterators so that the transforms with the longest path appear first.
        // Those with shorter paths will appear at the end. This is to ensure the child nodes are
        // deleted before their parents.
        struct compare_length
        {
            bool operator()(
                const TransformReferenceMap::iterator a,
                const TransformReferenceMap::iterator b) const
            {
                return a->first.GetString().size() > b->first.GetString().size();
            }
        };
        std::sort(toRemove.begin(), toRemove.end(), compare_length());
    }

    if (!toRemove.empty()) {
        std::vector<MObjectHandle> tempNodes;

        // now go and delete all of the nodes in order
        for (auto value = toRemove.begin(), e = toRemove.end(); value != e; ++value) {
            // reparent the custom transform under world prior to deleting
            MObject temp = (*value)->second.node();
            helper.m_modifier1.reparentNode(temp);

            // now we can delete (without accidentally nuking all parent transforms in the chain)
            helper.m_modifier1.deleteNode(temp);

            auto& paths = selectedPaths();
            for (auto iter = paths.begin(), end = paths.end(); iter != end; ++iter) {
                if (*iter == (*value)->first) {
                    helper.m_removedRefs.emplace_back((*value)->first, temp);
                    paths.erase(iter);
                    break;
                }
            }
        }
        m_selectedPaths.clear();

        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
