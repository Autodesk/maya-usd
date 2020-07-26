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
inline void addObjToSelectionList(MSelectionList& list, const MObject& object)
{
    if (object.hasFn(MFn::kDagNode)) {
        MFnDagNode dgNode(object);
        MDagPath   dg;
        dgNode.getPath(dg);
        list.add(dg, MObject::kNullObj, true);
    } else {
        list.add(object, true);
    }
};
} // namespace

//----------------------------------------------------------------------------------------------------------------------
/// I have to handle the case where maya commands are issued (e.g. select -cl) that will remove our
/// transform nodes from mayas global selection list (but will have left those nodes behind, and
/// left them in the transform refs within the proxy shape). In those cases, it should just be a
/// case of traversing the selected paths on the proxy shape, determine which paths are no longer in
/// the maya selection list, and then issue a command to AL_usdmaya_ProxyShapeSelect to deselct
/// those nodes. This will ensure that the nodes are nicely removed, and insert an item into the
/// undo stack.
//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onSelectionChanged(void* ptr)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Run onSelectionChanged");

    TF_DEBUG(ALUSDMAYA_SELECTION)
        .Msg("ProxyShapeSelection::onSelectionChanged %d\n", MGlobal::isUndoing());

    const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
    if (selectionMode) {
        ProxyShape* proxy = (ProxyShape*)ptr;
        if (!proxy)
            return;

        if (proxy->m_pleaseIgnoreSelection)
            return;

        if (proxy->selectedPaths().empty())
            return;

        auto stage = proxy->getUsdStage();

        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);

        std::vector<SdfPath> unselectedSet;
        MFnDagNode           fnDag;

        // now attempt to find any items that have been selected via maya (e.g. by clicking on the
        // parent node in the outliner)
        bool    hasNewItems = false;
        MString precommand = "AL_usdmaya_ProxyShapeSelect -i -a";
        for (uint32_t i = 0; i < sl.length(); ++i) {
            MObject obj;
            sl.getDependNode(i, obj);
            SdfPath path;
            if (!proxy->isSelectedMObject(obj, path)) {
                if (path.IsAbsolutePath()) {
                    precommand += " -pp \"";
                    precommand += path.GetText();
                    precommand += "\"";
                    hasNewItems = true;
                }
            }
        }

        struct compare_length
        {
            bool operator()(const SdfPath& a, const SdfPath& b) const
            {
                return a.GetString().size() > b.GetString().size();
            }
        };
        std::sort(unselectedSet.begin(), unselectedSet.end(), compare_length());

        // construct command to unselect the nodes (specifying the internal flag to ensure the
        // selection list is not modified)
        MString command = "AL_usdmaya_ProxyShapeSelect -i -d";
        for (auto removed : unselectedSet) {
            command += " -pp \"";
            command += removed.GetText();
            command += "\"";
        }

        fnDag.setObject(proxy->thisMObject());

        command += " \"";
        command += fnDag.fullPathName();
        command += "\"";

        if (hasNewItems) {
            precommand += " \"";
            precommand += fnDag.fullPathName();
            precommand += "\";";
            if (!unselectedSet.empty()) {
                command = precommand + command;
            } else {
                command = precommand;
            }
        }

        if (unselectedSet.empty() && !hasNewItems) {
            proxy->m_pleaseIgnoreSelection = true;
            MGlobal::executeCommand(command, false, true);
        }
    } else {
        ProxyShape* proxy = (ProxyShape*)ptr;
        if (!proxy)
            return;

        if (proxy->m_pleaseIgnoreSelection)
            return;

        if (proxy->m_hasChangedSelection)
            return;

        if (proxy->selectedPaths().empty())
            return;

        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl, false);

        bool    hasItems = false;
        bool    hasNewItems = false;
        MString precommand = "AL_usdmaya_ProxyShapeSelect -i -a";
        MString command = "AL_usdmaya_ProxyShapeSelect -i -d";

        // maya bug work around.
        auto hasObject = [](MSelectionList sl, MObject node) {
            for (uint32_t i = 0; i < sl.length(); ++i) {
                MObject obj;
                sl.getDependNode(i, obj);
                if (node == obj)
                    return true;
            }
            return false;
        };

        for (auto selected : proxy->selectedPaths()) {
            MObject obj = proxy->findRequiredPath(selected);
            if (!hasObject(sl, obj)) {
                hasItems = true;
                command += " -pp \"";
                command += selected.GetText();
                command += "\"";
            }
        }

        // now attempt to find any items that have been selected via maya (e.g. by clicking on the
        // parent node in the outliner)
        for (uint32_t i = 0; i < sl.length(); ++i) {
            MObject obj;
            sl.getDependNode(i, obj);
            SdfPath path;
            if (!proxy->isSelectedMObject(obj, path)) {
                if (path.IsAbsolutePath()) {
                    precommand += " -pp \"";
                    precommand += path.GetText();
                    precommand += "\"";
                    hasNewItems = true;
                }
            }
        }

        if (hasItems) {
            MFnDagNode fnDag(proxy->thisMObject());
            command += " \"";
            command += fnDag.fullPathName();
            command += "\"";
            if (hasNewItems) {
                precommand += " \"";
                precommand += fnDag.fullPathName();
                precommand += "\";";
                command = precommand + command;
            }
            proxy->m_pleaseIgnoreSelection = true;
            MGlobal::executeCommand(command, false, true);
            proxy->m_pleaseIgnoreSelection = false;
        } else if (hasNewItems) {
            MFnDagNode fnDag(proxy->thisMObject());
            precommand += " \"";
            precommand += fnDag.fullPathName();
            precommand += "\";";
            proxy->m_pleaseIgnoreSelection = true;
            MGlobal::executeCommand(precommand, false, true);
            proxy->m_pleaseIgnoreSelection = false;
        }
    }
}

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

    bool isUsdXFormable = usdPrim.IsA<UsdGeomXformable>();

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

    // build up new lock-prim list
    TfToken lockPropertyToken;
    if (usdPrim.GetMetadata<TfToken>(Metadata::locked, &lockPropertyToken)) {
        if (lockPropertyToken == Metadata::lockTransform) {
            m_lockManager.setLocked(usdPrim.GetPath());
        } else if (lockPropertyToken == Metadata::lockUnlocked) {
            m_lockManager.setUnlocked(usdPrim.GetPath());
        } else if (lockPropertyToken == Metadata::lockInherited) {
            m_lockManager.setInherited(usdPrim.GetPath());
        }
    } else {
        m_lockManager.setInherited(usdPrim.GetPath());
    }

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
            m_lockManager.setInherited(primPath);
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

    // ensure the transforms have been removed from the selectability and lock db's.
    m_lockManager.setInherited(path);
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

                    m_lockManager.setInherited(parentPrim);
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
    if (!MGlobal::optionVarIntValue("AL_usdmaya_ignoreLockPrims")) {
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
    if (!MGlobal::optionVarIntValue("AL_usdmaya_ignoreLockPrims")) {
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
                    m_lockManager.setInherited(iter.first);
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
bool ProxyShape::doSelect(SelectionUndoHelper& helper, const SdfPathVector& orderedPaths)
{
    MProfilingScope profilerScope(
        _proxyShapeSelectionProfilerCategory, MProfiler::kColorE_L3, "Do select");

    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyShapeSelection::doSelect\n");
    auto stage = m_stage;
    if (!stage)
        return false;
    triggerEvent("SelectionStarted");

    m_pleaseIgnoreSelection = true;
    prepSelect();

    MGlobal::getActiveSelectionList(helper.m_previousSelection);

    helper.m_previousPaths = selectedPaths();
    if (MGlobal::kReplaceList == helper.m_mode) {
        if (helper.m_paths.empty()) {
            helper.m_mode = MGlobal::kRemoveFromList;
            helper.m_paths = selectedPaths();
        }
    } else {
        helper.m_newSelection = helper.m_previousSelection;
    }
    MStringArray newlySelectedPaths;

    auto removeFromMSel = [](MSelectionList& sel, const MObject& toRemove) -> bool {
        for (uint32_t i = 0, n = sel.length(); i < n; ++i) {
            MObject obj;
            sel.getDependNode(i, obj);
            if (obj == toRemove) {
                sel.remove(i);
                return true;
            }
        }
        return false;
    };

    switch (helper.m_mode) {
    case MGlobal::kReplaceList: {
        std::vector<SdfPath> keepPrims;
        std::vector<UsdPrim> insertPrims;
        for (auto path : orderedPaths) {
            bool alreadySelected = m_selectedPaths.count(path) > 0;

            auto prim = stage->GetPrimAtPath(path);
            if (prim) {
                if (!alreadySelected)
                    insertPrims.push_back(prim);
                else
                    keepPrims.push_back(path);
            }
        }

        if (keepPrims.empty() && insertPrims.empty()) {
            m_pleaseIgnoreSelection = false;
            triggerEvent("PreSelectionChanged");
            triggerEvent("PostSelectionChanged");
            triggerEvent("SelectionEnded");
            return false;
        }

        std::sort(keepPrims.begin(), keepPrims.end());

        m_selectedPaths.clear();

        uint32_t hasNodesToCreate = 0;
        for (auto prim : insertPrims) {
            if (prim.IsPseudoRoot()) {
                // For pseudo root, just modify maya's selection, don't alter
                // our internal paths
                newlySelectedPaths.append(MFnDagNode(thisMObject()).fullPathName());
                addObjToSelectionList(helper.m_newSelection, thisMObject());
                continue;
            }
            m_selectedPaths.insert(prim.GetPath());
            MString pathName;
            MObject object = makeUsdTransformChain_internal(
                prim,
                helper.m_modifier1,
                ProxyShape::kSelection,
                &helper.m_modifier2,
                &hasNodesToCreate,
                &pathName);
            newlySelectedPaths.append(pathName);
            addObjToSelectionList(helper.m_newSelection, object);
            helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
        }

        for (auto iter : helper.m_previousPaths) {
            auto    temp = m_requiredPaths.find(iter);
            MObject object = temp->second.node();
            if (!std::binary_search(keepPrims.begin(), keepPrims.end(), iter)) {
                auto prim = stage->GetPrimAtPath(iter);
                removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
                helper.m_removedRefs.emplace_back(iter, object);
            } else {
                addObjToSelectionList(helper.m_newSelection, object);
                m_selectedPaths.insert(iter);
            }
        }

        helper.m_paths = m_selectedPaths;
    } break;

    case MGlobal::kAddToHeadOfList:
    case MGlobal::kAddToList: {
        std::vector<UsdPrim> prims;
        for (auto path : orderedPaths) {
            bool alreadySelected = m_selectedPaths.count(path) > 0;

            if (!alreadySelected) {
                auto prim = stage->GetPrimAtPath(path);
                if (prim) {
                    prims.push_back(prim);
                }
            }
        }

        helper.m_paths.insert(helper.m_previousPaths.begin(), helper.m_previousPaths.end());

        uint32_t hasNodesToCreate = 0;
        for (auto prim : prims) {
            if (prim.IsPseudoRoot()) {
                // For pseudo root, just modify maya's selection
                // However, since we don't want the pseudo root to "pollute" our
                // internal selected paths, we also need to make sure we clear
                // it from m_paths.  (We don't need to do those in other modes,
                // because those set m_paths to m_selectePaths).
                newlySelectedPaths.append(MFnDagNode(thisMObject()).fullPathName());
                addObjToSelectionList(helper.m_newSelection, thisMObject());
                helper.m_paths.erase(prim.GetPath());
                continue;
            }

            m_selectedPaths.insert(prim.GetPath());
            MString pathName;
            MObject object = makeUsdTransformChain_internal(
                prim,
                helper.m_modifier1,
                ProxyShape::kSelection,
                &helper.m_modifier2,
                &hasNodesToCreate,
                &pathName);
            newlySelectedPaths.append(pathName);
            addObjToSelectionList(helper.m_newSelection, object);
            helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
        }
    } break;

    case MGlobal::kRemoveFromList: {
        std::vector<UsdPrim> prims;

        // We use helper.m_paths, not orderedPaths here, because
        // if mode was MGlobal::kReplaceList at start, but helper.m_paths
        // was empty, we switch mode to kRemoveFromList, and
        // changed helper.m_paths to previousPaths.  This is fine, though
        // because we only need order so we can get right order for
        // newlySelectedPaths - which is not altered in this branch
        for (auto path : helper.m_paths) {
            bool alreadySelected;
            if (path == SdfPath::AbsoluteRootPath()) {
                // For pseudo-root, remove proxy shape from maya's selection
                alreadySelected = removeFromMSel(helper.m_newSelection, thisMObject());
            } else {
                alreadySelected = m_selectedPaths.count(path) > 0;
            }

            if (alreadySelected) {
                auto prim = stage->GetPrimAtPath(path);

                if (prim) {
                    prims.push_back(prim);
                }
            }
        }

        if (prims.empty()) {
            m_pleaseIgnoreSelection = false;
            triggerEvent("PreSelectionChanged");
            triggerEvent("PostSelectionChanged");
            triggerEvent("SelectionEnded");
            return false;
        }

        for (auto prim : prims) {
            if (prim.IsPseudoRoot()) {
                // We've already removed this when iterating helper.m_paths,
                // we just added to prims so the "prims.empty" early-exit test wouldn't
                // fire
                continue;
            }
            auto    temp = m_requiredPaths.find(prim.GetPath());
            MObject object = temp->second.node();

            m_selectedPaths.erase(prim.GetPath());

            removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
            removeFromMSel(helper.m_newSelection, object);
            helper.m_removedRefs.emplace_back(prim.GetPath(), object);
        }
        helper.m_paths = m_selectedPaths;
    } break;

    case MGlobal::kXORWithList: {
        std::vector<UsdPrim> removePrims;
        std::vector<UsdPrim> insertPrims;
        for (auto path : orderedPaths) {
            bool alreadySelected;
            if (path == SdfPath::AbsoluteRootPath()) {
                // For pseudo-root, remove proxy shape from maya's selection
                alreadySelected = removeFromMSel(helper.m_newSelection, thisMObject());
            } else {
                alreadySelected = m_selectedPaths.count(path) > 0;
            }

            auto prim = stage->GetPrimAtPath(path);
            if (prim) {
                if (alreadySelected)
                    removePrims.push_back(prim);
                else {
                    insertPrims.push_back(prim);
                }
            }
        }

        if (removePrims.empty() && insertPrims.empty()) {
            m_pleaseIgnoreSelection = false;
            triggerEvent("PreSelectionChanged");
            triggerEvent("PostSelectionChanged");
            triggerEvent("SelectionEnded");
            return false;
        }

        for (auto prim : removePrims) {
            if (prim.IsPseudoRoot()) {
                // We've already removed this when iterating orderedPaths,
                // we just added to removePrims so the "removePrims.empty" early-exit test
                // wouldn't fire
                continue;
            }
            auto    temp = m_requiredPaths.find(prim.GetPath());
            MObject object = temp->second.node();

            m_selectedPaths.erase(prim.GetPath());

            removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
            removeFromMSel(helper.m_newSelection, object);
            helper.m_removedRefs.emplace_back(prim.GetPath(), object);
        }

        uint32_t hasNodesToCreate = 0;
        for (auto prim : insertPrims) {
            if (prim.IsPseudoRoot()) {
                // For pseudo root, just modify maya's selection, don't alter
                // our internal paths
                newlySelectedPaths.append(MFnDagNode(thisMObject()).fullPathName());
                addObjToSelectionList(helper.m_newSelection, thisMObject());
                continue;
            }
            m_selectedPaths.insert(prim.GetPath());
            MString pathName;
            MObject object = makeUsdTransformChain_internal(
                prim,
                helper.m_modifier1,
                ProxyShape::kSelection,
                &helper.m_modifier2,
                &hasNodesToCreate,
                &pathName);
            newlySelectedPaths.append(pathName);
            addObjToSelectionList(helper.m_newSelection, object);
            helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
        }
        helper.m_paths = m_selectedPaths;
    } break;
    }

    triggerEvent("PreSelectionChanged");
    if (newlySelectedPaths.length()) {
        MPxCommand::setResult(newlySelectedPaths);
    }
    triggerEvent("PostSelectionChanged");

    m_pleaseIgnoreSelection = false;
    triggerEvent("SelectionEnded");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
