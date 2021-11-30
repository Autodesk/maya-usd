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
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"

#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/MenuBuilder.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"
#include "AL/usdmaya/nodes/Engine.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/usd/sdf/path.h>

#include <maya/M3dView.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MMatrix.h>
#include <maya/MSyntax.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>

namespace {
typedef void (AL::usdmaya::nodes::SelectionList::*SelectionListModifierFunc)(SdfPath);
}

namespace AL {
namespace usdmaya {
namespace cmds {
//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeCommandBase::setUpCommonSyntax()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeCommandBase::setUpCommonSyntax\n");
    MSyntax syntax;
    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
    syntax.addFlag("-p", "-proxy", MSyntax::kString);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MArgDatabase ProxyShapeCommandBase::makeDatabase(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeCommandBase::makeDatabase\n");
    MStatus      status;
    MArgDatabase database(syntax(), args, &status);
    if (!status) {
        std::cout << status.errorString() << std::endl;
        throw status;
    }
    return database;
}

//----------------------------------------------------------------------------------------------------------------------
MDagPath ProxyShapeCommandBase::getShapePath(const MArgDatabase& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeCommandBase::getShapePath\n");
    MSelectionList sl;
    args.getObjects(sl);
    MDagPath path;
    MStatus  status = sl.getDagPath(0, path);
    if (!status) {
        MGlobal::displayError("Argument is not a proxy shape");
        throw status;
    }

    if (path.node().hasFn(MFn::kTransform)) {
        path.extendToShape();
    }

    if (path.node().hasFn(MFn::kPluginShape)) {
        MFnDagNode fn(path);
        if (fn.typeId() == nodes::ProxyShape::kTypeId) {
            return path;
        } else {
            MGlobal::displayError("No usd proxy shape selected");
            throw MS::kFailure;
        }
    } else {
        MGlobal::displayError("No usd proxy shape selected");
        throw MS::kFailure;
    }
    return MDagPath();
}

//----------------------------------------------------------------------------------------------------------------------
nodes::ProxyShape*
ProxyShapeCommandBase::getShapeNode(const MArgDatabase& args, MDagPath* returnedPath)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeCommandBase::getShapeNode\n");
    MDagPath       path;
    MSelectionList sl;
    args.getObjects(sl);

    for (uint32_t i = 0; i < sl.length(); ++i) {
        MStatus status = sl.getDagPath(i, path);
        if (status) {
            if (path.node().hasFn(MFn::kTransform)) {
                path.extendToShape();
            }

            if (path.node().hasFn(MFn::kPluginShape)) {
                MFnDagNode fn(path);
                if (fn.typeId() == nodes::ProxyShape::kTypeId) {
                    if (returnedPath)
                        *returnedPath = path;
                    return (nodes::ProxyShape*)fn.userNode();
                }
            }
        }
    }
    sl.clear();

    {
        if (args.isFlagSet("-p")) {
            MString proxyName;
            if (args.getFlagArgument("-p", 0, proxyName)) {
                sl.add(proxyName);
                if (sl.length()) {
                    MStatus status = sl.getDagPath(0, path);
                    if (status) {
                        if (path.node().hasFn(MFn::kTransform)) {
                            path.extendToShape();
                        }

                        if (path.node().hasFn(MFn::kPluginShape)) {
                            MFnDagNode fn(path);
                            if (fn.typeId() == nodes::ProxyShape::kTypeId) {
                                if (returnedPath)
                                    *returnedPath = path;
                                return (nodes::ProxyShape*)fn.userNode();
                            }
                        }
                    }
                }
            }
            MGlobal::displayError("Invalid ProxyShape specified/selected with -p flag");
        } else
            MGlobal::displayError("No ProxyShape specified/selected");
    }

    throw MS::kFailure;
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr ProxyShapeCommandBase::getShapeNodeStage(const MArgDatabase& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeCommandBase::getShapeNodeStage\n");
    nodes::ProxyShape* node = getShapeNode(args);
    return node ? node->usdStage() : UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeImport, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeImport::createSyntax()
{
    MSyntax syntax;
    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 0);
    syntax.addFlag("-f", "-file", MSyntax::kString);
    syntax.addFlag("-sid", "-stageId", MSyntax::kLong);
    syntax.addFlag("-s", "-session", MSyntax::kString);
    syntax.addFlag("-n", "-name", MSyntax::kString);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-epp", "-excludePrimPath", MSyntax::kString);
    syntax.addFlag("-ctt", "-connectToTime", MSyntax::kBoolean);
    syntax.addFlag("-ul", "-unloaded", MSyntax::kBoolean);
    syntax.addFlag("-fp", "-fullpaths", MSyntax::kBoolean);
    syntax.makeFlagMultiUse("-arp");

    syntax.addFlag("-pmi", "-populationMaskInclude", MSyntax::kString);

    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeImport::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImport::undoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeImport::undoIt\n");
    // undo parenting
    MFnDagNode fn;
    for (uint32_t i = 0; i < m_parentTransforms.length(); ++i) {
        fn.setObject(m_parentTransforms[i]);
        fn.removeChild(m_shape);
    }

    return m_modifier.undoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImport::redoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeImport::redoIt\n");
    MStatus status = m_modifier.doIt();
    if (status) {
        // set the name of the node
        MFnDagNode fnShape(m_shape);

        if (m_createdParent) {
            // if lots of TM's have been specified as parents, just name the shape explicitly
            if (m_parentTransforms.length()) {
                if (m_proxy_name.length()) {
                    fnShape.setName(m_proxy_name + "Shape");
                }
            } else {
                MFnDependencyNode fnTransform(fnShape.parent(0));
                fnShape.setName(fnTransform.name() + "Shape");
                if (m_proxy_name.length()) {
                    fnTransform.setName(m_proxy_name);
                } else {
                    fnTransform.setName("AL_usdmaya_Proxy");
                }
            }
        }

        status = m_modifier2.doIt();
        if (status) {
            // parent under instances
            MFnDagNode fn;
            for (uint32_t i = 0; i < m_parentTransforms.length(); ++i) {
                fn.setObject(m_parentTransforms[i]);
                fn.addChild(m_shape, MFnDagNode::kNextPos, true);
            }
        }
    }

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImport::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeImport::doIt\n");
    MStatus      status;
    MArgDatabase database(syntax(), args, &status);
    AL_MAYA_COMMAND_HELP(database, g_helpText);
    if (!status)
        return status;

    // extract any parent transforms for the command.
    {
        MSelectionList items;
        if (database.getObjects(items)) {
            for (uint32_t i = 0; i < items.length(); ++i) {
                MObject node;
                items.getDependNode(i, node);
                if (node.hasFn(MFn::kTransform)) {
                    m_createdParent = false;
                    m_parentTransforms.append(node);
                }
            }
        }
    }

    MString filePath;
    int     stageId;
    MString sessionLayerSerialized;
    MString primPath;
    MString excludePrimPath;

    MString populationMaskIncludePath;
    bool    connectToTime = true;

    // extract command args
    bool hasFilePath = database.isFlagSet("-f") && database.getFlagArgument("-f", 0, filePath);
    bool hasStageCacheId = database.isFlagSet("-sid")
        && database.getFlagArgument("-sid", 0, stageId) && (stageId != -1);
    if (!hasFilePath && !hasStageCacheId) {
        MGlobal::displayError("Neither file path nor stage Id specified.");
        return MS::kFailure;
    } else if (hasFilePath && hasStageCacheId) {
        MGlobal::displayError("Cannot specify both file path and stage cache Id.");
        return MS::kFailure;
    }

    if (hasStageCacheId) {
        UsdStageCache::Id stageCacheId = UsdStageCache::Id().FromLongInt(stageId);
        if (!StageCache::Get().Contains(stageCacheId)) {
            MGlobal::displayError(
                MString("Cannot find stage with Id ") + stageId + " in stage cache.");
            return MS::kFailure;
        }
    }

    bool hasName = database.isFlagSet("-n");
    bool hasPrimPath = database.isFlagSet("-pp");
    bool hasExclPrimPath = database.isFlagSet("-epp");
    bool hasSession = database.isFlagSet("-s");
    bool hasStagePopulationMaskInclude = database.isFlagSet("-pmi");

    if (hasName) {
        database.getFlagArgument("-n", 0, m_proxy_name);
    }
    if (hasPrimPath) {
        database.getFlagArgument("-pp", 0, primPath);
        if (!SdfPath(primPath.asChar()).IsPrimPath()) {
            MGlobal::displayError(MString("Invalid primPath: ") + primPath);
            return MS::kFailure;
        }
    }
    if (hasExclPrimPath) {
        database.getFlagArgument("-epp", 0, excludePrimPath);
        if (!SdfPath(excludePrimPath.asChar()).IsPrimPath()) {
            MGlobal::displayError(MString("Invalid excludePrimPath: ") + excludePrimPath);
            return MS::kFailure;
        }
    }
    if (database.isFlagSet("-ctt")) {
        database.getFlagArgument("-ctt", 0, connectToTime);
    }
    if (hasSession) {
        database.getFlagArgument("-s", 0, sessionLayerSerialized);
    }
    if (hasStagePopulationMaskInclude) {
        database.getFlagArgument("-pmi", 0, populationMaskIncludePath);
    }

    // what are we parenting this node to?
    MObject firstParent;
    if (m_parentTransforms.length()) {
        firstParent = m_parentTransforms[0];
        m_parentTransforms.remove(0);
    } else {
        // if no TM specified, create one
        firstParent = m_modifier.createNode("transform");
    }

    // create the shape node
    m_shape = m_modifier.createNode(nodes::ProxyShape::kTypeId, firstParent);

    // initialize the session layer, if given
    if (hasSession) {
        auto sessionLayer = SdfLayer::CreateAnonymous();

        sessionLayer->ImportFromString(AL::maya::utils::convert(sessionLayerSerialized));
        auto layerManager = nodes::LayerManager::findOrCreateManager(&m_modifier);
        if (!layerManager) {
            MGlobal::displayError(MString("Unknown error getting/creating LayerManager node"));
            return MS::kFailure;
        }
        layerManager->addLayer(sessionLayer);
        m_modifier.newPlugValueString(
            MPlug(m_shape, nodes::ProxyShape::sessionLayerName()),
            AL::maya::utils::convert(sessionLayer->GetIdentifier()));
    }

    // intialise the params
    if (hasPrimPath)
        m_modifier.newPlugValueString(MPlug(m_shape, nodes::ProxyShape::primPath()), primPath);
    if (hasExclPrimPath)
        m_modifier.newPlugValueString(
            MPlug(m_shape, nodes::ProxyShape::excludePrimPaths()), excludePrimPath);
    if (database.isFlagSet("-ul")) {
        bool unloaded;
        database.getFlagArgument("-ul", 0, unloaded);
        m_modifier.newPlugValueBool(MPlug(m_shape, nodes::ProxyShape::unloaded()), unloaded);
    }

    if (hasStagePopulationMaskInclude)
        m_modifier.newPlugValueString(
            MPlug(m_shape, nodes::ProxyShape::populationMaskIncludePaths()),
            populationMaskIncludePath);

    // Prefer stage cache Id over file path.
    if (hasStageCacheId) {
        m_modifier2.newPlugValueInt(MPlug(m_shape, nodes::ProxyShape::stageCacheId()), stageId);
    } else {
        m_modifier2.newPlugValueString(MPlug(m_shape, nodes::ProxyShape::filePath()), filePath);
    }

    if (connectToTime) {
        MSelectionList sl;
        sl.add("time1");
        MObject time1;
        sl.getDependNode(0, time1);
        MFnDependencyNode fnTime(time1);
        MPlug             outTime = fnTime.findPlug("outTime");
        m_modifier2.connect(outTime, MPlug(m_shape, nodes::ProxyShape::time()));
    }
    status = redoIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MFnDagNode dagNode(m_shape, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    MDagPathArray dagPaths;
    MStringArray  stringNames;
    CHECK_MSTATUS_AND_RETURN_IT(dagNode.getAllPaths(dagPaths));
    std::function<MString(const MDagPath&)> getFullPath
        = [&](const MDagPath& dagPath) { return dagPath.fullPathName(); };
    std::function<MString(const MDagPath&)> getPartialPath
        = [&](const MDagPath& dagPath) { return dagPath.partialPathName(); };
    auto getPath = database.isFlagSet("-fp") ? getFullPath : getPartialPath;
    for (size_t i = 0; i < dagPaths.length(); i++) {
        stringNames.append(getPath(dagPaths[i]));
    }
    clearResult();
    setResult(stringNames);
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeFindLoadable, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeFindLoadable::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-l", "-loaded");
    syntax.addFlag("-ul", "-unloaded");
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeFindLoadable::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeFindLoadable::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeFindLoadable::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        bool loaded = db.isFlagSet("-l");
        bool unloaded = db.isFlagSet("-ul");
        if (unloaded && loaded) {
            MGlobal::displayError("-loaded or -unloaded, there can be only one.");
            return MS::kFailure;
        }

        SdfPath path;
        if (db.isFlagSet("-pp")) {
            MString pathString;
            db.getFlagArgument("-pp", 0, pathString);
            path = SdfPath(AL::maya::utils::convert(pathString));
            if (!path.IsPrimPath()) {
                MGlobal::displayError(MString("Invalid primPath: ") + path.GetText());
                return MS::kFailure;
            }
        } else {
            path = SdfPath::AbsoluteRootPath();
        }

        MStringArray   result;
        UsdStageRefPtr stage = getShapeNodeStage(db);
        if (!unloaded && !loaded) {
            SdfPathSet all = stage->FindLoadable(path);
            result.setLength(all.size());
            uint32_t i = 0;
            for (auto it = all.begin(); it != all.end(); ++it, ++i) {
                result[i] = AL::maya::utils::convert(it->GetString());
            }
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("all %zu\n", all.size());
        } else if (loaded && db.isFlagSet("-pp")) {
            SdfPathSet loadableSet = stage->FindLoadable(path);
            SdfPathSet loadedSet = stage->GetLoadSet();
            SdfPathSet intersected;

            std::set_intersection(
                loadedSet.begin(),
                loadedSet.end(),
                loadableSet.begin(),
                loadableSet.end(),
                std::inserter(intersected, intersected.end()));

            result.setLength(intersected.size());
            uint32_t i = 0;
            for (auto it = intersected.begin(); it != intersected.end(); ++it, ++i) {
                result[i] = AL::maya::utils::convert(it->GetString());
            }
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadableSet %zu\n", loadableSet.size());
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadedSet %zu\n", loadedSet.size());
        } else if (loaded) {
            SdfPathSet all = stage->GetLoadSet();
            result.setLength(all.size());
            uint32_t i = 0;
            for (auto it = all.begin(); it != all.end(); ++it, ++i) {
                result[i] = AL::maya::utils::convert(it->GetString());
            }
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loaded %zu\n", all.size());
        } else if (unloaded && db.isFlagSet("-pp")) {
            SdfPathSet all = stage->FindLoadable();
            SdfPathSet loadableSet = stage->FindLoadable(path);
            SdfPathSet loadedSet = stage->GetLoadSet();
            SdfPathSet diffed;
            SdfPathSet intersected;

            std::set_difference(
                loadedSet.begin(),
                loadedSet.end(),
                all.begin(),
                all.end(),
                std::inserter(diffed, diffed.end()));

            std::set_intersection(
                diffed.begin(),
                diffed.end(),
                loadableSet.begin(),
                loadableSet.end(),
                std::inserter(intersected, intersected.end()));

            result.setLength(intersected.size());
            uint32_t i = 0;
            for (auto it = intersected.begin(); it != intersected.end(); ++it, ++i) {
                result[i] = AL::maya::utils::convert(it->GetString());
            }
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("all %zu\n", all.size());
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadableSet %zu\n", loadableSet.size());
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadedSet %zu\n", loadedSet.size());
        } else if (unloaded) {
            SdfPathSet loadableSet = stage->FindLoadable(path);
            SdfPathSet loadedSet = stage->GetLoadSet();
            SdfPathSet diffed;

            std::set_difference(
                loadedSet.begin(),
                loadedSet.end(),
                loadableSet.begin(),
                loadableSet.end(),
                std::inserter(diffed, diffed.end()));

            result.setLength(diffed.size());
            uint32_t i = 0;
            for (auto it = diffed.begin(); it != diffed.end(); ++it, ++i) {
                result[i] = AL::maya::utils::convert(it->GetString());
            }
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadedSet %zu\n", loadedSet.size());
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("loadableSet %zu\n", loadableSet.size());
            TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("diffed %zu\n", diffed.size());
        }
        setResult(result);
    } catch (const MStatus& status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeImportAllTransforms, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeImportAllTransforms::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-p2p", "-pushToPrim", MSyntax::kBoolean);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-s", "-selected", MSyntax::kNoArg);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeImportAllTransforms::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportAllTransforms::undoIt()
{
    if (m_modifier2.doIt()) {
        return m_modifier.doIt();
    }
    return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportAllTransforms::redoIt()
{
    if (m_modifier.doIt()) {
        return m_modifier2.doIt();
    }
    return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportAllTransforms::doIt(const MArgList& args)
{
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        bool pushToPrim = false;
        if (db.isFlagSet("-p2p")) {
            db.getFlagArgument("-p2p", 0, pushToPrim);
        }

        MString primPath;
        if (db.isFlagSet("-pp")) {
            db.getFlagArgument("-pp", 0, primPath);
        }

        // This command should pretty much always just
        nodes::ProxyShape::TransformReason reason = nodes::ProxyShape::kRequested;
        if (db.isFlagSet("-s")) {
            reason = nodes::ProxyShape::kSelection;
        }

        MDagPath shapePath = getShapePath(db);
        MObject  shapeObject = shapePath.node();
        MDagPath transformPath = shapePath;
        transformPath.pop();

        nodes::ProxyShape* shapeNode = getShapeNode(db);
        if (!shapeNode) {
            throw MS::kFailure;
        }

        UsdStageRefPtr stage = shapeNode->usdStage();
        if (!stage) {
            throw MS::kFailure;
        }

        MDGModifier* modifier = 0;
        if (pushToPrim) {
            modifier = &m_modifier2;
        }

        if (primPath.length()) {
            SdfPath usdPath(AL::maya::utils::convert(primPath));
            UsdPrim prim = stage->GetPrimAtPath(usdPath);
            if (!prim) {
                MGlobal::displayError(
                    MString("The prim path specified could not be found in the USD stage: ")
                    + primPath);
                throw MS::kFailure;
            } else {
                shapeNode->makeUsdTransforms(prim, m_modifier, reason, modifier);
            }
        } else {
            UsdPrim root = stage->GetPseudoRoot();
            for (auto it = root.GetChildren().begin(), end = root.GetChildren().end(); it != end;
                 ++it) {
                SdfPath usdPath = it->GetPath();
                UsdPrim prim = stage->GetPrimAtPath(usdPath);
                shapeNode->makeUsdTransforms(prim, m_modifier, reason, modifier);
            }
        }
    } catch (const MStatus&) {
        return MS::kFailure;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeRemoveAllTransforms, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeRemoveAllTransforms::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);

    // This flag is not to be used. I'm just using it for testing purposes.
    syntax.addFlag("-s", "-selection", MSyntax::kNoArg);
    syntax.addFlag("-f", "-force", MSyntax::kNoArg);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeRemoveAllTransforms::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeRemoveAllTransforms::undoIt() { return m_modifier.undoIt(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeRemoveAllTransforms::redoIt() { return m_modifier.doIt(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeRemoveAllTransforms::doIt(const MArgList& args)
{
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        nodes::ProxyShape* shapeNode = getShapeNode(db);
        MDagPath           shapePath = getShapePath(db);
        MObject            shapeObject = shapePath.node();
        MDagPath           transformPath = shapePath;
        transformPath.pop();

        // This command should pretty much always just
        nodes::ProxyShape::TransformReason reason = nodes::ProxyShape::kRequested;
        if (db.isFlagSet("-s")) {
            reason = nodes::ProxyShape::kSelection;
        }

        MString primPath;
        if (db.isFlagSet("-pp")) {
            db.getFlagArgument("-pp", 0, primPath);
        }

        UsdStageRefPtr stage = shapeNode->usdStage();
        if (!stage) {
            throw MS::kFailure;
        }

        if (primPath.length()) {
            SdfPath usdPath(AL::maya::utils::convert(primPath));
            UsdPrim prim = stage->GetPrimAtPath(usdPath);
            if (!prim) {
                MGlobal::displayError(
                    MString("The prim path specified could not be found in the USD stage: ")
                    + primPath);
                throw MS::kFailure;
            } else {
                shapeNode->removeUsdTransforms(prim, m_modifier, reason);
            }
        } else {
            UsdPrim root = stage->GetPseudoRoot();
            for (auto it = root.GetChildren().begin(), end = root.GetChildren().end(); it != end;
                 ++it) {
                shapeNode->removeUsdTransforms(*it, m_modifier, reason);
            }
        }
    } catch (const MStatus&) {
        return MS::kFailure;
    }
    return redoIt();
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeResync, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------

MSyntax ProxyShapeResync::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    return syntax;
}

bool ProxyShapeResync::isUndoable() const { return false; }

MStatus ProxyShapeResync::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeResync::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        m_shapeNode = getShapeNode(db);

        if (db.isFlagSet("-pp")) {
            MString pathString;
            db.getFlagArgument("-pp", 0, pathString);
            SdfPath primPath = SdfPath(AL::maya::utils::convert(pathString));
            if (!primPath.IsPrimPath()) {
                MGlobal::displayError(MString("Invalid primPath: ") + pathString);
                return MS::kFailure;
            }

            UsdStageRefPtr stage = m_shapeNode->getUsdStage();
            UsdPrim        prim = stage->GetPrimAtPath(primPath);

            if (prim.IsValid()) {
                m_resyncPrimPath = primPath;
            }
        }
    } catch (const MStatus& status) {
        return status;
    }

    return redoIt();
}

MStatus ProxyShapeResync::redoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeResync::redoIt\n");
    if (m_resyncPrimPath == SdfPath::EmptyPath()) {
        MGlobal::displayError("ProxyShapeResync: PrimPath is empty. ");
        return MStatus::kFailure;
    }

    m_shapeNode->primChangedAtPath(m_resyncPrimPath);

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(InternalProxyShapeSelect, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax InternalProxyShapeSelect::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.useSelectionAsDefault(false);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    syntax.addFlag("-cl", "-clear", MSyntax::kNoArg);
    syntax.addFlag("-a", "-append", MSyntax::kNoArg);
    syntax.addFlag("-tgl", "-toggle", MSyntax::kNoArg);
    syntax.addFlag("-r", "-replace", MSyntax::kNoArg);
    syntax.addFlag("-d", "-deselect", MSyntax::kNoArg);
    syntax.makeFlagMultiUse("-pp");
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool InternalProxyShapeSelect::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus InternalProxyShapeSelect::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("InternalProxyShapeSelect::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        m_proxy = getShapeNode(db);
        if (!m_proxy) {
            throw MS::kFailure;
        }
        m_previous = m_proxy->selectionList();
        if (db.isFlagSet("-cl")) {
        } else {
            SelectionListModifierFunc selListModiferFunc;
            if (db.isFlagSet("-d")) {
                m_new = m_previous;
                selListModiferFunc = &nodes::SelectionList::remove;
            } else if (db.isFlagSet("-tgl")) {
                m_new = m_previous;
                selListModiferFunc = &nodes::SelectionList::toggle;
            } else {
                if (!db.isFlagSet("-r")) {
                    m_new = m_previous;
                }

                selListModiferFunc = &nodes::SelectionList::add;
            }

            for (uint32_t i = 0, n = db.numberOfFlagUses("-pp"); i < n; ++i) {
                MArgList args;
                db.getFlagArgumentList("-pp", i, args);
                MString pathString = args.asString(0);
                SdfPath path(AL::maya::utils::convert(pathString));
                if (!path.IsPrimPath()) {
                    MGlobal::displayError(MString("Invalid primPath: ") + pathString);
                    return MS::kFailure;
                }
                (m_new.*selListModiferFunc)(path);
            }
        }
    } catch (const MStatus& status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus InternalProxyShapeSelect::undoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("InternalProxyShapeSelect::undoIt\n");
    m_proxy->selectionList() = m_previous;
    if (MGlobal::kInteractive == MGlobal::mayaState())
        MGlobal::executeCommand("refresh", false, false);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus InternalProxyShapeSelect::redoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("InternalProxyShapeSelect::redoIt\n");
    m_proxy->selectionList() = m_new;
    if (MGlobal::kInteractive == MGlobal::mayaState())
        MGlobal::executeCommand("refresh", false, false);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeSelect, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeSelect::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    syntax.addFlag("-cl", "-clear", MSyntax::kNoArg);
    syntax.addFlag("-a", "-append", MSyntax::kNoArg);
    syntax.addFlag("-tgl", "-toggle", MSyntax::kNoArg);
    syntax.addFlag("-r", "-replace", MSyntax::kNoArg);
    syntax.addFlag("-d", "-deselect", MSyntax::kNoArg);
    syntax.addFlag("-i", "-internal", MSyntax::kNoArg);
    syntax.addFlag("-ls", "-list", MSyntax::kNoArg);
    syntax.makeFlagMultiUse("-pp");
    syntax.enableQuery(true);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeSelect::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeSelect::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeSelect::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        MDagPath           proxyDagPath;
        nodes::ProxyShape* proxy = getShapeNode(db);
        if (!proxy) {
            throw MS::kFailure;
        }

        if (db.isFlagSet("-ls") && db.isQuery()) {
            m_helper = nullptr;
            auto         sl = Ufe::GlobalSelection::get();
            MStringArray strings;
            for (auto& item : *sl) {
                auto pathSegment = item->path().getSegments()[1];
                auto pathStr = pathSegment.string().c_str();
                if (proxyDagPath.fullPathName() == pathStr) {
                    strings.append(pathStr);
                }
            }
            setResult(strings);
            return MS::kSuccess;
        } else if (!db.isQuery()) {

            SdfPathVector                              orderedPaths;
            nodes::SelectionUndoHelper::SdfPathHashSet unorderedPaths;

            MGlobal::ListAdjustment mode = MGlobal::kAddToList;
            if (db.isFlagSet("-cl")) {
                mode = MGlobal::kReplaceList;
            } else {
                for (uint32_t i = 0, n = db.numberOfFlagUses("-pp"); i < n; ++i) {
                    MArgList args;
                    db.getFlagArgumentList("-pp", i, args);
                    MString pathString = args.asString(0);

                    SdfPath path(AL::maya::utils::convert(pathString));

                    if (!proxy->selectabilityDB().isPathUnselectable(path)
                        && path.IsAbsolutePath()) {
                        auto insertResult = unorderedPaths.insert(path);
                        if (insertResult.second) {
                            orderedPaths.push_back(path);
                        }
                    }
                }

                if (db.isFlagSet("-tgl")) {
                    mode = MGlobal::kXORWithList;
                } else if (db.isFlagSet("-a")) {
                    mode = MGlobal::kAddToList;
                } else if (db.isFlagSet("-r")) {
                    mode = MGlobal::kReplaceList;
                } else if (db.isFlagSet("-d")) {
                    mode = MGlobal::kRemoveFromList;
                }
            }
            const bool isInternal = db.isFlagSet("-i");

            m_helper = new nodes::SelectionUndoHelper(proxy, unorderedPaths, mode, isInternal);
            if (!proxy->doSelect(*m_helper, orderedPaths)) {
                delete m_helper;
                m_helper = 0;
            }
            return _redoIt(isInternal);
        }
    } catch (const MStatus& status) {
        return status;
    } catch (std::exception& e) {
        MStatus status = MS::kFailure;
        status.perror(MString("(ProxyShapeSelect::doIt) : ") + e.what());
        return status;
    } catch (...) {
        MStatus status = MS::kFailure;
        status.perror("(ProxyShapeSelect::doIt) Unknown internal failure!");
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeSelect::undoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeSelect::undoIt\n");
    if (m_helper)
        m_helper->undoIt();
    if (MGlobal::kInteractive == MGlobal::mayaState())
        MGlobal::executeCommand("refresh", false, false);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeSelect::redoIt() { return _redoIt(false); }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeSelect::_redoIt(bool isInternal)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeSelect::redoIt\n");
    if (m_helper)
        m_helper->doIt();
    if (MGlobal::kInteractive == MGlobal::mayaState() && !isInternal)
        MGlobal::executeCommandOnIdle("refresh", false);

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapePostSelect, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapePostSelect::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePostSelect::redoIt()
{
    m_proxy->setChangedSelectionState(false);
    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl);
    MString    command;
    MFnDagNode fn(m_proxy->thisMObject());
    for (const auto& path : m_proxy->selectedPaths()) {
        auto obj = m_proxy->findRequiredPath(path);
        if (obj != MObject::kNullObj) {
            MFnDagNode dagNode(obj);
            MDagPath   dg;
            dagNode.getPath(dg);
            if (!sl.hasItem(dg)) {
                command += "AL_usdmaya_ProxyShapeSelect -i -d -pp \"";
                command += path.GetText();
                command += "\" \"";
                command += fn.fullPathName();
                command += "\";";
            }
        }
    }
    if (command.length() > 0) {
        MGlobal::executeCommand(command, false, false);
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePostSelect::undoIt()
{
    m_proxy->setChangedSelectionState(true);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapePostSelect::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePostSelect::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapePostSelect::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        m_proxy = getShapeNode(db);
        if (!m_proxy) {
            throw MS::kFailure;
        }
    } catch (const MStatus& status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapeImportPrimPathAsMaya, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MObject
ProxyShapeImportPrimPathAsMaya::makePrimTansforms(nodes::ProxyShape* shapeNode, UsdPrim usdPrim)
{
    return shapeNode->makeUsdTransformChain(usdPrim, m_modifier, nodes::ProxyShape::kRequired);
}

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapeImportPrimPathAsMaya::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-ap", "-asProxy", MSyntax::kNoArg);
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeImportPrimPathAsMaya::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportPrimPathAsMaya::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeImportPrimPathAsMaya::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        MDagPath shapePath = getShapePath(db);
        m_transformPath = shapePath;
        m_transformPath.pop();

        if (db.isFlagSet("-pp")) {
            MString pathString;
            db.getFlagArgument("-pp", 0, pathString);
            m_path = SdfPath(AL::maya::utils::convert(pathString));
            if (!m_path.IsPrimPath()) {
                MGlobal::displayError(MString("Invalid primPath: ") + pathString);
                return MS::kFailure;
            }
        }

        m_asProxyShape = false;
        if (db.isFlagSet("-ap")) {
            m_asProxyShape = true;
        }

        nodes::ProxyShape* shapeNode = getShapeNode(db);
        if (!shapeNode) {
            throw MS::kFailure;
        }

        UsdPrim usdPrim = shapeNode->usdStage()->GetPrimAtPath(m_path);
        if (!usdPrim) {
            throw MS::kFailure;
        }

        MObject parentTransform = makePrimTansforms(shapeNode, usdPrim);
        if (m_asProxyShape) {
            MFnDagNode fn;
            MObject    node = m_modifier.createNode(nodes::ProxyShape::kTypeId, parentTransform);
            fn.setObject(node);
            fn.setName(usdPrim.GetName().GetText());
        } else {
        }
    } catch (const MStatus& status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportPrimPathAsMaya::redoIt() { return m_modifier.doIt(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeImportPrimPathAsMaya::undoIt() { return m_modifier.undoIt(); }

//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(TranslatePrim, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------

MSyntax TranslatePrim::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-ip", "-importPaths", MSyntax::kString);
    syntax.addFlag("-tp", "-teardownPaths", MSyntax::kString);
    syntax.addFlag("-up", "-updatePaths", MSyntax::kString);
    syntax.addFlag("-fi", "-forceImport", MSyntax::kNoArg);
    syntax.addFlag("-fd", "-forceDefault", MSyntax::kNoArg);
    syntax.addFlag("-ptp", "-pushToPrim", MSyntax::kBoolean);
    syntax.addFlag("-rav", "-readAnimatedValues", MSyntax::kBoolean);
    syntax.addFlag("-r", "-recursive");
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatePrim::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("TranslatePrim::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        m_proxy = getShapeNode(db);

        m_recursive = db.isFlagSet("-r");

        if (db.isFlagSet("-ip")) {
            MString pathsCsv;
            db.getFlagArgument("-ip", 0, pathsCsv);
            m_importPaths = m_proxy->getPrimPathsFromCommaJoinedString(pathsCsv);
        }

        if (db.isFlagSet("-tp")) {
            MString pathsCsv;
            db.getFlagArgument("-tp", 0, pathsCsv);
            m_teardownPaths = m_proxy->getPrimPathsFromCommaJoinedString(pathsCsv);
        }

        if (db.isFlagSet("-up")) {
            MString pathsCsv;
            db.getFlagArgument("-up", 0, pathsCsv);
            m_updatePaths = m_proxy->getPrimPathsFromCommaJoinedString(pathsCsv);
        }

        // change the translator context to force import
        if (db.isFlagSet("-fi")) {
            tp.setForcePrimImport(true);
        }

        // should pushToPrim be enabled on transforms when translating?
        if (db.isFlagSet("-ptp")) {
            bool value = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim");
            db.getFlagArgument("-ptp", 0, value);
            tp.setPushToPrim(value);
        }

        // should pushToPrim be enabled on transforms when translating?
        if (db.isFlagSet("-rav")) {
            bool value = true;
            db.getFlagArgument("-rav", 0, value);
            tp.setReadAnimatedValues(value);
        }

        // change the translator context to read default value
        if (db.isFlagSet("-fd")) {
            m_proxy->context()->setForceDefaultRead(true);
        }

        if (m_recursive) {
            const bool forceImport = db.isFlagSet("-fi");
            MFnDagNode fn(m_proxy->thisMObject());
            MDagPath   proxyTransformPath;
            fn.getPath(proxyTransformPath);
            proxyTransformPath.pop();

            auto& manufacture = m_proxy->translatorManufacture();

            auto          stage = m_proxy->usdStage();
            SdfPathVector newImportPaths;
            for (auto importPath : m_importPaths) {
                {
                    auto prim = stage->GetPrimAtPath(importPath);
                    if (prim) {
                        if (manufacture.get(prim)) {
                            newImportPaths.push_back(importPath);
                        }
                    }
                }
                auto newPrimSet = m_proxy->huntForNativeNodesUnderPrim(
                    proxyTransformPath, importPath, manufacture, forceImport);
                for (auto it : newPrimSet) {
                    newImportPaths.push_back(it.GetPath());
                }
            }
            std::swap(m_importPaths, newImportPaths);

            SdfPathVector newTeardownPaths;
            for (auto teardownPath : m_teardownPaths) {
                {
                    auto prim = stage->GetPrimAtPath(teardownPath);
                    if (prim) {
                        if (manufacture.get(prim)) {
                            newTeardownPaths.push_back(teardownPath);
                        }
                    }
                }
                auto newPrimSet = m_proxy->huntForNativeNodesUnderPrim(
                    proxyTransformPath, teardownPath, manufacture, true);
                for (auto it : newPrimSet) {
                    newTeardownPaths.push_back(it.GetPath());
                }
            }
            std::swap(m_teardownPaths, newTeardownPaths);

            SdfPathVector newUpdatePaths;
            for (auto updatePath : m_updatePaths) {
                {
                    auto prim = stage->GetPrimAtPath(updatePath);
                    if (prim) {
                        if (manufacture.get(prim)) {
                            newUpdatePaths.push_back(updatePath);
                        }
                    }
                }
                auto newPrimSet = m_proxy->huntForNativeNodesUnderPrim(
                    proxyTransformPath, updatePath, manufacture, true);
                for (auto it : newPrimSet) {
                    // We only want to list the prims that are actually updateable.
                    auto translator = manufacture.get(it);
                    if (translator && translator->supportsUpdate()) {
                        newUpdatePaths.push_back(it.GetPath());
                    }
                }
            }
            std::swap(m_updatePaths, newUpdatePaths);
        }
    } catch (const MStatus& status) {
        return status;
    }

    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatePrim::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatePrim::redoIt()
{
    MDagPath parentTransform = m_proxy->parentTransform();

    // This makes sure we cannot run import twice.
    SdfPathVector newImportPaths;
    for (auto importPath : m_importPaths) {
        uint32_t selected = 0;
        uint32_t required = 0;
        uint32_t refCount = 0;
        m_proxy->getCounts(importPath, selected, required, refCount);
        if (!required && !refCount) {
            newImportPaths.push_back(importPath);
        }
    }

    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("TranslatePrim::redoIt\n");
    m_proxy->translatePrimPathsIntoMaya(newImportPaths, m_teardownPaths, tp);

    // construct locks and selectability for imported prims
    if (m_proxy->isLockPrimFeatureActive()) {
        m_proxy->removeMetaData(m_teardownPaths);
        m_proxy->processChangedMetaData(SdfPathVector(), newImportPaths);
    }

    if (!m_updatePaths.empty()) {

        // check paths refer to valid prims for this stage
        auto                        stage = m_proxy->usdStage();
        MayaUsdUtils::UsdPrimVector updatePrims;

        AL::usdmaya::fileio::translators::TranslatorManufacture manufacture(nullptr);
        for (const SdfPath& path : m_updatePaths) {
            UsdPrim prim = stage->GetPrimAtPath(path);
            if (prim.IsValid()) {
                auto translator = manufacture.get(prim);
                if (translator && translator->supportsUpdate()) {
                    translator->update(prim);
                    updatePrims.push_back(prim);
                }
            } else {
                TF_DEBUG(ALUSDMAYA_COMMANDS)
                    .Msg(
                        "TranslatePrim::redoIt '%s' resolves to an invalid prim\n", path.GetText());
            }
        }
        cmds::ProxyShapePostLoadProcess::updateSchemaPrims(m_proxy, updatePrims);
        cmds::ProxyShapePostLoadProcess::connectSchemaPrims(m_proxy, updatePrims);
    }

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(ProxyShapeTestIntersection, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------

MSyntax ProxyShapeTestIntersection::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-sx", "-screenX", MSyntax::kDouble);
    syntax.addFlag("-sy", "-screenY", MSyntax::kDouble);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeTestIntersection::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapeTestIntersection::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);
        MDagPath proxyDagPath;
        m_proxy = getShapeNode(db);
        if (!m_proxy || !m_proxy->usdStage()) {
            throw MS::kFailure;
        }

        if (!db.isFlagSet("-sx") || !db.isFlagSet("-sy")) {
            MGlobal::displayError("-sx and -sy are required");
            return MStatus::kFailure;
        }

        db.getFlagArgument("-sx", 0, m_sx);
        db.getFlagArgument("-sy", 0, m_sy);
    } catch (const MStatus& status) {
        return status;
    }

    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeTestIntersection::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapeTestIntersection::redoIt()
{
    MStatus status;
    auto    currentView = M3dView::active3dView(&status);

    MMatrix worldViewMatrix, projectionMatrix;
    status = currentView.modelViewMatrix(worldViewMatrix);
    status = currentView.projectionMatrix(projectionMatrix);

    // Compute a pick matrix that, when it is post-multiplied with the projection
    // matrix, will cause the picking region to fill the entire viewport for
    // OpenGL selection.
    {
        unsigned int view_x, view_y, view_w, view_h;
        currentView.viewport(view_x, view_y, view_w, view_h);

        MMatrix pickMatrix;
        pickMatrix[0][0] = view_w;
        pickMatrix[1][1] = view_h;
        pickMatrix[3][0] = view_w - 2.0 * (m_sx - view_x);
        pickMatrix[3][1] = view_h - 2.0 * (m_sy - view_y);

        projectionMatrix *= pickMatrix;
    }

    auto engine = m_proxy->engine();
    if (!engine)
        return MStatus::kFailure;

    auto          stage = m_proxy->getUsdStage();
    UsdPrim       root = stage->GetPseudoRoot();
    SdfPathVector rootPath;
    rootPath.push_back(root.GetPath());
    UsdImagingGLRenderParams params;
    unsigned int             resolution = 10;
    nodes::Engine::HitBatch  hitBatch;

    bool hit = engine->TestIntersectionBatch(
        GfMatrix4d(worldViewMatrix.matrix),
        GfMatrix4d(projectionMatrix.matrix),
        GfMatrix4d(1.f),
        rootPath,
        params,
        HdxPickTokens->resolveNearestToCamera,
        resolution,
        &hitBatch);

    if (hit) {
        clearResult();
        for (const auto& it : hitBatch) {
            const double* p = it.second.GetArray();
            appendToResult(p[0]);
            appendToResult(p[1]);
            appendToResult(p[2]);
        }
    } else {
        MGlobal::displayInfo("[AL_usdmaya_ProxyShapeTestIntersection]: No hit points found\n");
    }

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void constructProxyShapeCommandGuis()
{
    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_ProxyShapeImport",
            "Proxy Shape Import",
            "Import",
            "USD/Proxy Shape/Import",
            false);
        commandGui.addFilePathOption(
            "file",
            "File Path",
            AL::maya::utils::CommandGuiHelper::kLoad,
            "USD all (*.usdc *.usda *.usd);;USD crate (*.usdc) (*.usdc);;USD Ascii (*.usda) "
            "(*.usda);;USD (*.usd) (*.usd)",
            AL::maya::utils::CommandGuiHelper::kStringOptional);
        commandGui.addIntOption("stageId", "Stage cache Id", -1, false);
        commandGui.addStringOption(
            "primPath",
            "USD Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringOptional);
        commandGui.addStringOption(
            "excludePrimPath",
            "Exclude Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringOptional);
        commandGui.addStringOption(
            "name",
            "Proxy Shape Node Name",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringOptional);
        commandGui.addBoolOption("connectToTime", "Connect to Time", true, true);
        commandGui.addBoolOption(
            "unloaded", "Opens the layer with payloads unloaded.", false, true);
    }

    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_ProxyShapeImportPrimPathAsMaya",
            "Import Prim Path as Maya xforms",
            "Import",
            "USD/Proxy Shape/Import Prim Path as Maya",
            true);
        commandGui.addStringOption(
            "primPath",
            "USD Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
        commandGui.addFlagOption("asProxy", "Import Subsection as a Proxy Node", false, true);
    }

    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_ProxyShapeImportAllTransforms",
            "Import All Transforms",
            "Import",
            "USD/Proxy Shape/Import Transforms as Transforms",
            true);
        commandGui.addBoolOption("pushToPrim", "Push to Prim", false, true);
    }

    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_ProxyShapeRemoveAllTransforms",
            "Remove All Transforms",
            "Remove",
            "USD/Proxy Shape/Remove all Transforms",
            true);
    }

    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_ProxyShapeResync",
            "Resync at Prim path",
            "",
            "Resync and reload prim at passed in primpath",
            false);
        commandGui.addStringOption(
            "primPath",
            "USD Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    }

    {
        AL::maya::utils::CommandGuiHelper commandGui(
            "AL_usdmaya_TranslatePrim",
            "Translate a Prim at path",
            "",
            "Run the translator to either import or teardown the Prims at the paths",
            false);
        commandGui.addStringOption(
            "importPath",
            "USD Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringOptional);
        commandGui.addStringOption(
            "teardownPath",
            "USD Prim Path",
            "",
            false,
            AL::maya::utils::CommandGuiHelper::kStringOptional);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void constructPickModeCommandGuis()
{
    const nodes::ProxyShape::PickMode pickMode = MGlobal::optionVarExists("AL_usdmaya_pickMode")
        ? nodes::ProxyShape::PickMode(
            MGlobal::optionVarIntValue("AL_usdmaya_pickMode")) // Restore from prefs
        : nodes::ProxyShape::PickMode::kPrims;                 // Fall back to default
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Pick Mode/Prims",
        "optionVar -iv \\\"AL_usdmaya_pickMode\\\" 0",
        false,                                            // Checkbox
        false,                                            // Checkbox state
        true,                                             // Radio button
        pickMode == nodes::ProxyShape::PickMode::kPrims); // Radio button state
    AL::maya::utils::MenuBuilder::addEntry(
        "USD/Pick Mode/Models",
        "optionVar -iv \\\"AL_usdmaya_pickMode\\\" 1",
        false,                                             // Checkbox
        false,                                             // Checkbox state
        true,                                              // Radio button
        pickMode == nodes::ProxyShape::PickMode::kModels); // Radio button state
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ProxyShapePrintRefCountState, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ProxyShapePrintRefCountState::createSyntax()
{
    MSyntax syntax = setUpCommonSyntax();
    syntax.addFlag("-h", "-help", MSyntax::kNoArg);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapePrintRefCountState::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePrintRefCountState::doIt(const MArgList& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ProxyShapePrintRefCountState::doIt\n");
    try {
        MArgDatabase db = makeDatabase(args);
        AL_MAYA_COMMAND_HELP(db, g_helpText);

        /// find the proxy shape node
        nodes::ProxyShape* shapeNode = getShapeNode(db);
        if (shapeNode) {
            shapeNode->printRefCounts();
        }
    } catch (const MStatus& status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// Documentation strings.
//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeImport::g_helpText = R"(
AL_usdmaya_ProxyShapeImport Overview:

  This command allows you to import a USD file as a proxy shape node. In the simplest case, you can do this:

    AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene";

  which will load the usda file specified, and create an ProxyShape of the specified name.

  If you wish to instance that scene into maya a bunch of times, you can do this:

    AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene" "transform1" "transform2";

  This will load the file, create the proxy shape, and then add them as instances underneath transform1 and transform2.

  Some other flags and stuff:

    To load only a subset of the USD file, you can specify a root prim path with the -pp/-primPath flag:

       -primPath "/myScene/foo/bar"

    This will ignore everything in the USD file apart from the UsdPrim's underneath /myScene/foo/bar.

    By default the imported proxy node will be connected to the time1.outTime attribute.
    The -ctt/-connectToTime flag controls this behaviour, so adding this flag will mean the usd proxy
    is not driven by time at all:

       -connectToTime false

    If you wish to prevent certain prims from being displayed in the proxy, you can specify the -excludePrimPath/-epp
    flag, e.g.

       -excludePrimPath "/do/not/show/this/prim"

    If you want to exclude some prims from being read when stage is opened, use the -pmi/-populationMaskInclude flag, e.g.

       -populationMaskInclude "/only/show/this/prim1,/only/show/this/prim2"

   It is possible to import the USD stage with an overloaded session layer _(which can be useful if you wish to import
   the scene with a specific set of variants set)_.  To specify the session layer contents, use the -session flag:

       -session "#usda 1.0"

   It is also possible to prevent all loadable prims from being loaded when importing the USD stage by specifying the unloaded
   flag _(the default is false)_

       -unloaded true   //< don't load any loadable prims
       -unloaded false  //< load all loadable prims

    The command will return a string array containing the names of all instances of the created node. (There will be
    more than one instance if more than one transform was selected or passed into the command.)  By default, the will
    be the shortest-unique names; if -fp/-fullpaths is given, then they will be full path names.

    This command is undoable.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeFindLoadable::g_helpText = R"(
AL_usdmaya_ProxyShapeFindLoadable Overview:

  This command doesn't do what I thought it would, so therefore I have no idea whey it's here.
  I had assumed this would produce a list of all asset references that can be loaded, however it
  seems to do nothing.

  If you have an ProxyShape node selected, then:

    AL_usdmaya_ProxyShapeFindLoadable              //< produce a list of all assets? payloads?

    AL_usdmaya_ProxyShapeFindLoadable -unloaded    //< produce a list of all unloaded assets? unloaded payloads?

    AL_usdmaya_ProxyShapeFindLoadable -loaded      //< produce a list of all loaded assets? loaded payloads?

  You can also specify a prim path root, which in theory should end up restricting the returned
  results to just those under the specified path.

    AL_usdmaya_ProxyShapeFindLoadable -pp "/only/assets/under/here";
    AL_usdmaya_ProxyShapeFindLoadable -pp -loaded "/only/assets/under/here";
    AL_usdmaya_ProxyShapeFindLoadable -pp -unloaded "/only/assets/under/here";

  I think the code to this command is correct, however I have no idea what it's supposed to do. One
  day it might return a result, so I'll leave it here for now.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeImportAllTransforms::g_helpText = R"(
AL_usdmaya_ProxyShapeImportAllTransforms Overview:

  Assuming you have selected an ProxyShape node, this command will traverse the prim hierarchy,
  and for each prim found, an Transform node will be created. The -pushToPrim/-p2p option controls
  whether the generated Transforms have their pushToPrim attribute set to true. If it's enabled,
  then the generated transforms will drive the USD prims. If however it is disabled, then the transform
  nodes will only be observing the UsdPrims

    AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p true;  // drive the USD prims
    AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p false ; // observe the USD prims

  This command is undoable.

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeRemoveAllTransforms::g_helpText = R"(
AL_usdmaya_ProxyShapeRemoveAllTransforms Overview:

  If you have previously generated a tonne of Transforms to drive the prims in a usd proxy shape,
  via a call to 'ProxyShapeImportAllTransforms', then this command will go and delete all of those
  transform nodes again.

    AL_usdmaya_ProxyShapeRemoveAllTransforms "ProxyShape1";  // drive the USD prims

  This command is undoable.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeImportPrimPathAsMaya::g_helpText = R"(
AL_usdmaya_ProxyShapeImportPrimPathAsMaya Overview:

  The following call will import the path "/some/prim/path", and all of it's parent transforms as AL_usdmaya_Transform
  nodes into maya.

    AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -pp "/some/prim/path";

  The custom Maya transforms generated will now act as thin wrapper over the transforms within USD. Any modifications
  you make within Maya will be directly translated into USD _(and stored within the currect edit target)_.

  Adding in the -ap/-asProxy flag will build a transform hierarchy of Transform nodes to the
  specified prim, and then create a new ProxyShape to represent all of that geometry underneath
  that prim path.

    AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -ap -pp "/some/prim/path";

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapePrintRefCountState::g_helpText = R"(
AL_usdmaya_ProxyShapePrintRefCountState Overview:

  The AL_usdmaya_ProxyShape node maintains an internal set of reference counts that determine the life span of an
  AL_usdmaya_Transform node (i.e. a transform may have been created because it has been selected, or because the
  transform is required for a particular plugin translator node). For the average user of AL_USDMaya, these ref
  counts are nothing more than an implementation detail that can be ignored. For some developers working on the
  core of AL_USDMaya, being able to inspect these ref counts may be of use, which you can do so like this:

    AL_usdmaya_ProxyShapePrintRefCountState -p "ProxyShapeName";
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeSelect::g_helpText = R"(
AL_usdmaya_ProxyShapeSelect Overview:

  This command is designed to mimic the maya select command, but instead of acting on
  maya node names or dag paths, it acts upon SdfPaths within a USD stage. So in the very simplest case,
  to select a USD prim with path "/root/hips/thigh_left" contained within the proxy shape
  "AL_usdmaya_ProxyShape1", you would execute the command in the following way:

      AL_usdmaya_ProxyShapeSelect -r -pp "/root/hips/thigh_left" "AL_usdmaya_ProxyShape1";

  To select more than one path, re-use the -pp flag, e.g.

      AL_usdmaya_ProxyShapeSelect -r -pp "/root/hips/thigh_left" -pp "/root/hips/thigh_right" "AL_usdmaya_ProxyShape1";

  The -pp flag specifies a prim path to select, and it can be re-used as many times as needed.
  When selecting prims on a proxy shape, you can specify a series of modifiers that change the behaviour
  of the AL_usdmaya_ProxyShapeSelect command. These modifiers roughly map to the flags in the standard
  maya 'select' command:

    -a   / -append      : Add to the current selection list
    -r   / -replace     : Replace the current selection list
    -d   / -deselect    : Remove the prims from the current selection list
    -tgl / -toggle      : If the prim is selected, deselect. If the prim is unselected, select.


  If you wish to deselect all prims on a proxy shape node, use the -cl/-clear flag, e.g.

    AL_usdmaya_ProxyShapeSelect -cl "AL_usdmaya_ProxyShape1";


  There is one final flag: -i/-internal. Please do not use (It will probably cause a crash!)

  [The -i/-internal flag prevents changes to Mayas global selection list. This is occasionally needed
   internally within the USD Maya plugin, when the proxy shape is listening to state changes caused by the
   MEL command select, or via the API call MGlobal::setActiveSelectionList. The behaviour of this flag
   is driven by internal requirements, so no guarantee will be given about its behaviour in future]
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapePostSelect::g_helpText = R"(
AL_usdmaya_ProxyShapePostSelect Overview:

  This is an internal command to ensure that maya selection is instep with the usd selection.
)";
//----------------------------------------------------------------------------------------------------------------------
const char* const InternalProxyShapeSelect::g_helpText = R"(
AL_usdmaya_InternalProxyShapeSelect Overview:

  This command is a simpler version of the AL_usdmaya_ProxyShapeSelect command. Unlike that command,
  AL_usdmaya_InternalProxyShapeSelect only highlights the geometry in UsdImaging (it does not generate
  the modifyable transform nodes in the scene).

    AL_usdmaya_InternalProxyShapeSelect -r -pp "/root/hips/thigh_left" "AL_usdmaya_ProxyShape1";

  To select more than one path, re-use the -pp flag, e.g.

    AL_usdmaya_InternalProxyShapeSelect -r -pp "/root/hips/thigh_left" -pp "/root/hips/thigh_right" "AL_usdmaya_ProxyShape1";

  The -pp flag specifies a prim path to select, and it can be re-used as many times as needed.
  When selecting prims on a proxy shape, you can specify a series of modifiers that change the behaviour
  of the AL_usdmaya_ProxyShapeSelect command. These modifiers roughly map to the flags in the standard
  maya 'select' command:

    -a   / -append      : Add to the current selection list
    -r   / -replace     : Replace the current selection list
    -d   / -deselect    : Remove the prims from the current selection list
    -tgl / -toggle      : If the prim is selected, deselect. If the prim is unselected, select.


  If you wish to deselect all prims on a proxy shape node, use the -cl/-clear flag, e.g.

    AL_usdmaya_InternalProxyShapeSelect -cl "AL_usdmaya_ProxyShape1";

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeResync::g_helpText = R"(
AL_usdmaya_ProxyShapeResync Overview:
    used to inform AL_USDMaya that at the provided prim path and it's descendants, that the Maya scene at that point may be affected by some upcoming changes.

    After calling this command, clients are expected to make modifications to the stage and as a side effect will trigger a USDNotice call in AL_USDMaya
    which will update corresponding Maya nodes that live at or under the specified primpath; any other maintenance such as updating of internal caches will also be done.

    The provided prim path and it's descendants of  known schema type will have the AL::usdmaya::fileio::translators::TranslatorAbstract::preTearDown method called on each schema's translator
    It's then up to the user to perform updates to the USD scene at or below that point in the hierarchy
    On calling stage.Reload(),the relevant USDNotice will be triggered and and apply any changes and updates to the Maya scene.

    AL_usdmaya_ProxyShapeResync -p "ProxyShape1" -pp "/some/prim/path"

)";
//----------------------------------------------------------------------------------------------------------------------
const char* const TranslatePrim::g_helpText = R"(
TranslatePrim Overview:

  Used to manually execute a translator for a prim at the specified path typically so you can force an import or a tearDown of a prim:

    AL_usdmaya_TranslatePrim -ip "/MyPrim";  //< Run the Prim's translator's import
    AL_usdmaya_TranslatePrim -tp "/MyPrim";  //< Run the Prim's translator's tearDown

    AL_usdmaya_TranslatePrim -ip "/MyPrim,/YourPrim";  //< Run the Prim's translator's import on multiple Prims
    AL_usdmaya_TranslatePrim -tp "/MyPrim,/YourPrim";  //< Run the Prim's translator's tearDown on multiple Prims

  Some prims such as the Mesh typed prims are not imported by default, so you will need to pass in a flag that forces the import:

    AL_usdmaya_TranslatePrim -fi -ip "/MyMesh";  //< Run the Prim's translator's import

  The ForceImport(-fi) flag will forces the import of the available translator. Used for translators who don't import automatically when
  their corresponding prim type is brought into the scene.
)";
//----------------------------------------------------------------------------------------------------------------------
const char* const ProxyShapeTestIntersection::g_helpText = R"(
ProxyShapeTestIntersection Overview:

  Used to retrieve the world space location of the point of the closest USD prim hit by ray cast from a selected
  position.

    AL_usdmaya_ProxyShapeTestIntersection -sx 30 -sy 80;
)";
//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
