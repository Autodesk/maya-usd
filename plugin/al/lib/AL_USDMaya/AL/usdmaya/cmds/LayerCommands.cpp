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
#include "AL/usdmaya/cmds/LayerCommands.h"

#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/sdf/listOp.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MArgDatabase.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

namespace {
AL::usdmaya::nodes::ProxyShape* getProxyShapeFromSel(const MSelectionList& sl)
{
    auto getShapePtr
        = [](const MObject& mobj, MFnDagNode& fnDag) -> AL::usdmaya::nodes::ProxyShape* {
        if (mobj.hasFn(MFn::kPluginShape)) {
            fnDag.setObject(mobj);
            if (fnDag.typeId() == AL::usdmaya::nodes::ProxyShape::kTypeId) {
                return (AL::usdmaya::nodes::ProxyShape*)fnDag.userNode();
            }
        }
        return nullptr;
    };

    AL::usdmaya::nodes::ProxyShape* foundShape = nullptr;

    MDagPath       path;
    const uint32_t selLength = sl.length();
    for (uint32_t i = 0; i < selLength; ++i) {
        MStatus status = sl.getDagPath(i, path);
        if (status != MS::kSuccess)
            continue;

        MFnDagNode fn(path);

        if (path.node().hasFn(MFn::kTransform)) {
            if (fn.typeId() == AL::usdmaya::nodes::Transform::kTypeId
                || fn.typeId() == AL::usdmaya::nodes::Scope::kTypeId) {
                auto transform = (AL::usdmaya::nodes::Scope*)fn.userNode();
                if (transform) {
                    MPlug sourcePlug = transform->inStageDataPlug().source();
                    foundShape = getShapePtr(sourcePlug.node(), fn);
                    if (foundShape)
                        return foundShape;
                } else {
                    MGlobal::displayError(
                        MString("Error getting Transform pointer for ") + fn.partialPathName());
                    return nullptr;
                }
                // If we have an AL_usdmaya_ProxyShapeTransform, but it wasn't hooked
                // up to a ProxyShape, just continue to the next selection item
                continue;
            } else {
                // If it's a "normal" xform, search all shapes directly below
                unsigned int numShapes;
                AL_MAYA_CHECK_ERROR_RETURN_VAL(
                    path.numberOfShapesDirectlyBelow(numShapes),
                    nullptr,
                    "Error getting number of shapes beneath " + path.partialPathName());
                for (unsigned int i = 0; i < numShapes; path.pop(), ++i) {
                    path.extendToShapeDirectlyBelow(i);
                    foundShape = getShapePtr(path.node(), fn);
                    if (foundShape)
                        return foundShape;
                }
            }
        } else {
            foundShape = getShapePtr(path.node(), fn);
            if (foundShape)
                return foundShape;
        }
    }
    return nullptr;
}
} // namespace

namespace AL {
namespace usdmaya {
namespace cmds {
//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerCommandBase::setUpCommonSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-p", "-proxy", MSyntax::kString);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
nodes::ProxyShape* LayerCommandBase::getShapeNode(const MArgDatabase& args)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCommandBase::getShapeNode\n");
    MDagPath       path;
    MSelectionList sl;
    args.getObjects(sl);

    const uint32_t selLength = sl.length();
    for (uint32_t i = 0; i < selLength; ++i) {
        MStatus status = sl.getDagPath(i, path);
        if (status != MS::kSuccess)
            continue;

        if (path.node().hasFn(MFn::kTransform)) {
            path.extendToShape();
        }

        if (path.node().hasFn(MFn::kPluginShape)) {
            MFnDagNode fn(path);
            if (fn.typeId() == nodes::ProxyShape::kTypeId) {
                return (nodes::ProxyShape*)fn.userNode();
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
                    if (status == MS::kSuccess) {
                        if (path.node().hasFn(MFn::kTransform)) {
                            path.extendToShape();
                        }

                        if (path.node().hasFn(MFn::kPluginShape)) {
                            MFnDagNode fn(path);
                            if (fn.typeId() == nodes::ProxyShape::kTypeId) {
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
UsdStageRefPtr LayerCommandBase::getShapeNodeStage(const MArgDatabase& args)
{
    nodes::ProxyShape* node = getShapeNode(args);
    return node ? node->getUsdStage() : UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
// LayerGetLayers
//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(LayerGetLayers, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerGetLayers::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.useSelectionAsDefault(true);
    syn.setObjectType(MSyntax::kSelectionList, 0, 1);
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addFlag("-u", "-used", MSyntax::kNoArg);
    syn.addFlag("-m", "-muted", MSyntax::kNoArg);
    syn.addFlag("-s", "-stack", MSyntax::kNoArg);
    syn.addFlag("-sl", "-sessionLayer", MSyntax::kNoArg);
    syn.addFlag("-rl", "-rootLayer", MSyntax::kNoArg);
    syn.addFlag("-id", "-identifiers", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerGetLayers::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerGetLayers::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerGetLayers::doIt\n");
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;
        AL_MAYA_COMMAND_HELP(args, g_helpText);

        nodes::ProxyShape* proxyShape = getShapeNode(args);
        UsdStageRefPtr     stage = proxyShape->usdStage();
        MStringArray       results;

        const bool useIdentifiers = args.isFlagSet("-id");
        if (args.isFlagSet("-rl")) {

            SdfLayerHandle rootLayer = stage->GetRootLayer();
            if (useIdentifiers) {
                setResult(AL::maya::utils::convert(rootLayer->GetIdentifier()));
            } else {
                setResult(AL::maya::utils::convert(rootLayer->GetDisplayName()));
            }
            return MS::kSuccess;
        } else if (args.isFlagSet("-m")) {
            const std::vector<std::string>& layers = stage->GetMutedLayers();
            for (auto it = layers.begin(); it != layers.end(); ++it) {
                if (useIdentifiers) {
                    results.append(AL::maya::utils::convert(*it));
                } else {
                    // Need to convert from identifier to display name...
                    auto layer = SdfLayer::Find(*it);
                    if (!layer) {
                        // If we failed to grab the layer from it's identifier, something went
                        // wrong...
                        MGlobal::displayError(
                            MString("Could not find muted layer from identifier: ")
                            + AL::maya::utils::convert(*it));
                        return MS::kFailure;
                    }
                    results.append(AL::maya::utils::convert(layer->GetDisplayName()));
                }
            }
        } else if (args.isFlagSet("-s")) {
            const bool           includeSessionLayer = args.isFlagSet("-sl");
            SdfLayerHandleVector layerStack = stage->GetLayerStack(includeSessionLayer);
            for (auto it = layerStack.begin(); it != layerStack.end(); ++it) {
                if (useIdentifiers) {
                    results.append(AL::maya::utils::convert((*it)->GetIdentifier()));
                } else {
                    results.append(AL::maya::utils::convert((*it)->GetDisplayName()));
                }
            }
        } else if (args.isFlagSet("-u")) {
            const bool           includeSessionLayer = args.isFlagSet("-sl");
            SdfLayerHandle       sessionLayer = stage->GetSessionLayer();
            SdfLayerHandleVector layerStack = stage->GetUsedLayers();
            for (auto it = layerStack.begin(); it != layerStack.end(); ++it) {
                if (!includeSessionLayer) {
                    if (sessionLayer == *it)
                        continue;
                }
                if (useIdentifiers) {
                    results.append(AL::maya::utils::convert((*it)->GetIdentifier()));
                } else {
                    results.append(AL::maya::utils::convert((*it)->GetDisplayName()));
                }
            }
        } else if (args.isFlagSet("-sl")) {
            SdfLayerHandle sessionLayer = stage->GetSessionLayer();
            if (useIdentifiers) {
                setResult(AL::maya::utils::convert(sessionLayer->GetIdentifier()));
            } else {
                setResult(AL::maya::utils::convert(sessionLayer->GetDisplayName()));
            }
            return MS::kSuccess;
        }
        setResult(results);
    } catch (const MStatus& status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// LayerCreateLayer
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerCreateLayer, AL_usdmaya);

MSyntax LayerCreateLayer::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.useSelectionAsDefault(true);
    syn.setObjectType(MSyntax::kSelectionList, 0, 1);
    syn.addFlag("-o", "-open", MSyntax::kString);
    syn.addFlag("-s", "-sublayer", MSyntax::kNoArg);
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerCreateLayer::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::doIt\n");
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        if (args.isFlagSet("-o")) {
            // extract remaining args
            args.getFlagArgument("-o", 0, m_filePath);
        }

        if (args.isFlagSet("-s")) {
            m_addSublayer = true;
        }

        m_shape = getShapeNode(args);

        if (!m_shape) {
            throw MS::kFailure;
        }

        m_stage = m_shape->getUsdStage();
        if (!m_stage) {
            MGlobal::displayError("no valid stage found on the proxy shape");
            throw MS::kFailure;
        }
        m_rootLayer = m_stage->GetRootLayer();
    } catch (const MStatus& status) {
        return status;
    }

    if (!m_shape) {
        MGlobal::displayError(MString("LayerCreateLayer: Invalid shape during Layer creation"));
        return MS::kFailure;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::undoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::undoIt\n");

    // first let's go remove the newly created layer handle from the root layer we added it to
    LAYER_HANDLE_CHECK(m_newLayer);

    if (m_layerWasInserted) {
        auto manager = AL::usdmaya::nodes::LayerManager::findOrCreateManager();
        manager->removeLayer(m_newLayer);
    }

    if (m_addSublayer) {
        TF_DEBUG(ALUSDMAYA_COMMANDS)
            .Msg(
                "LayerCreateLayer::Undo'ing '%s', removing from sublayers \n",
                m_newLayer->GetIdentifier().c_str());
        SdfSubLayerProxy paths = m_stage->GetRootLayer()->GetSubLayerPaths();
        paths.Remove(m_newLayer->GetIdentifier());
    }

    // lots more to do here!
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCreateLayer::redoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCreateLayer::redoIt\n");
    UsdStageRefPtr childStage;

    if (m_filePath.length() > 0) {
        std::string filePath = AL::maya::utils::convert(m_filePath);
        m_newLayer = SdfLayer::FindOrOpen(filePath);
        if (!m_newLayer) {
            MGlobal::displayError(
                MString("LayerCreateLayer:unable to open layer \"") + m_filePath + "\"");
            return MS::kFailure;
        }

    } else {
        m_newLayer = SdfLayer::CreateAnonymous();
        if (!m_newLayer) {
            MGlobal::displayError("LayerCreateLayer:unable to create anonymous layer");
            return MS::kFailure;
        }
    }

    auto manager = AL::usdmaya::nodes::LayerManager::findOrCreateManager();
    m_layerWasInserted = manager->addLayer(m_newLayer);

    if (m_addSublayer) {
        TF_DEBUG(ALUSDMAYA_COMMANDS)
            .Msg(
                "LayerCreateLayer::Adding '%s' to sublayers\n",
                m_newLayer->GetIdentifier().c_str());
        SdfSubLayerProxy paths = m_stage->GetRootLayer()->GetSubLayerPaths();
        paths.push_back(m_newLayer->GetIdentifier());
    }

    std::stringstream ss("LayerCreateLayer:", std::ios_base::app | std::ios_base::out);
    MGlobal::displayInfo(ss.str().c_str());
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// LayerCurrentEditTarget
//----------------------------------------------------------------------------------------------------------------------

AL_MAYA_DEFINE_COMMAND(LayerCurrentEditTarget, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerCurrentEditTarget::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.useSelectionAsDefault(true);
    syn.setObjectType(MSyntax::kSelectionList, 0, 1);
    syn.enableQuery(true);
    syn.addFlag("-l", "-layer", MSyntax::kString);
    syn.addFlag("-sp", "-sourcePath", MSyntax::kString);
    syn.addFlag("-tp", "-targetPath", MSyntax::kString);
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addFlag("-fdn", "-findByDisplayName", MSyntax::kNoArg);
    syn.addFlag("-fid", "-findByIdentifier", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerCurrentEditTarget::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
PcpNodeRef LayerCurrentEditTarget::determineEditTargetMapping(
    UsdStageRefPtr      stage,
    const MArgDatabase& args,
    SdfLayerHandle      editTargetLayer) const
{
    if (editTargetLayer.IsInvalid()) {
        return PcpNodeRef();
    }

    if (args.isFlagSet("-sp") && args.isFlagSet("-tp")) {
        MString targetPath;
        MString sourcePath;
        args.getFlagArgument("-tp", 0, targetPath);
        args.getFlagArgument("-sp", 0, sourcePath);

        UsdPrim parentPrim = stage->GetPrimAtPath(SdfPath(targetPath.asChar()));
        if (!parentPrim.IsValid()) {
            std::stringstream ss(
                "LayerCurrentEditTarget:", std::ios_base::app | std::ios_base::out);
            ss << "Couldn't find the parent prim at path '" << targetPath.asChar() << "'"
               << std::endl;
            MGlobal::displayWarning(ss.str().c_str());
            return PcpNodeRef();
        }

        SdfReferenceListOp referenceListOp;
        if (parentPrim.GetMetadata<SdfReferenceListOp>(TfToken("references"), &referenceListOp)) {
            // TODO: I doubt this is the correct way to get current references. The API for
            // UsdPrim.GetReferences() isn't sufficient!
            // TODO: Spiff recommends getting the references a different way,
            // TODO: as mentioned here
            // https://groups.google.com/forum/#!topic/usd-interest/o6jK0tVw2eU
            const SdfListOp<SdfReference>::ItemVector& addedItems = referenceListOp.GetAddedItems();

            size_t referenceListSize = addedItems.size();
            for (size_t i = 0; i < referenceListSize; ++i) {
                SdfLayerHandle referencedLayer = SdfLayer::Find(addedItems[i].GetAssetPath());

                // Is the referenced layer referring to the layer we selected?
                if (referencedLayer == editTargetLayer) {
                    PcpNodeRef::child_const_range childRange
                        = parentPrim.GetPrimIndex().GetRootNode().GetChildrenRange();
                    PcpNodeRef::child_const_iterator currentChildSpec = childRange.first;
                    PcpNodeRef::child_const_iterator end = childRange.second;
                    while (currentChildSpec != end) {
                        if (currentChildSpec->GetParentNode().GetPath()
                                == SdfPath(targetPath.asChar())
                            && currentChildSpec->GetPath() == SdfPath(sourcePath.asChar())) {
                            return *currentChildSpec;
                        }
                        ++currentChildSpec;
                    }
                }
            }

            MGlobal::displayWarning("LayerCurrentEditTarget: Couldn't find the PcpNodeRef to "
                                    "initialise the MappingFunction for the EditTarget");
        }
    } else {
        MGlobal::displayInfo(
            "LayerCurrentEditTarget: Default MappingFunction for EditTarget will be used since "
            "sp(Source Prim) and tp(Target Prim) flags were not set");
    }

    return PcpNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);
        if (args.isQuery()) {
            isQuery = true;
            stage = getShapeNodeStage(args);
            if (stage) {
                const UsdEditTarget& editTarget = stage->GetEditTarget();
                previous = editTarget;
            } else {
                MGlobal::displayError(
                    "LayerCurrentEditTarget: no loaded stage found on proxy node");
                return MS::kFailure;
            }
        } else {
            // Setup the function pointers which will be used to find the wanted layer
            if (args.isFlagSet("-fid")) {
                // Use the Identifier when looking for the correct layer. Used for anonymous layers
                getLayerId = [](SdfLayerHandle layer) { return layer->GetIdentifier(); };
            } else if (args.isFlagSet("-fdn")) {
                // Use the DisplayName when looking for the correct layer
                getLayerId = [](SdfLayerHandle layer) { return layer->GetDisplayName(); };
            } else {
                // Default to DisplayName if not specified for backwards compatibility
                getLayerId = [](SdfLayerHandle layer) { return layer->GetDisplayName(); };
            }

            isQuery = false;

            stage = getShapeNodeStage(args);
            if (stage) {
                // if the layer has been manually specified
                MString        layerName;
                SdfLayerHandle foundLayer = nullptr;
                if (args.isFlagSet("-l") && args.getFlagArgument("-l", 0, layerName)) {
                    nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
                    if (layerManager) {
                        foundLayer = layerManager->findLayer(layerName.asChar());
                    }
                }

                previous = stage->GetEditTarget();
                isQuery = false;
                std::string layerName2;
                if (foundLayer) {

                    PcpNodeRef mappingNode = determineEditTargetMapping(stage, args, foundLayer);
                    if (mappingNode) {
                        next = UsdEditTarget(foundLayer, mappingNode);
                    } else {
                        next = UsdEditTarget(foundLayer);
                    }
                    layerName2 = getLayerId(next.GetLayer());
                } else if (layerName.length() > 0) {
                    layerName2 = AL::maya::utils::convert(layerName);
                    SdfLayerHandleVector layers = stage->GetUsedLayers();
                    for (auto it = layers.begin(); it != layers.end(); ++it) {

                        SdfLayerHandle handle = *it;

                        if (layerName2 == getLayerId(handle)) {
                            PcpNodeRef mappingNode
                                = determineEditTargetMapping(stage, args, handle);
                            if (mappingNode) {
                                next = UsdEditTarget(handle, mappingNode);
                            } else {
                                next = UsdEditTarget(handle);
                            }
                            break;
                        }
                    }
                } else {
                    MGlobal::displayError("No layer specified");
                    return MS::kFailure;
                }

                if (!next.IsValid()) {
                    // if we failed to find the layer in the list of used layers, just check to see
                    // whether we are actually able to edit said layer.
                    SdfLayerHandleVector layers = stage->GetUsedLayers();
                    for (auto it = layers.begin(); it != layers.end(); ++it) {
                        SdfLayerHandle handle = *it;
                        if (layerName2 == getLayerId(handle)) {
                            MGlobal::displayError("LayerCurrentEditTarget: Unable to set the edit "
                                                  "target, the specified layer cannot be edited");
                            return MS::kFailure;
                        }
                    }
                    MGlobal::displayError(
                        MString("LayerCurrentEditTarget: no layer found on proxy node that matches "
                                "the name \"")
                        + AL::maya::utils::convert(layerName2) + "\"");
                    return MS::kFailure;
                }
            } else {
                MGlobal::displayError(
                    "LayerCurrentEditTarget: no loaded stage found on proxy node");
                return MS::kFailure;
            }
        }
    } catch (const MStatus& status) {
        MGlobal::displayError("LayerCurrentEditTarget: no proxy node found");
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::redoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::redoIt\n");
    if (!isQuery) {
        TF_DEBUG(ALUSDMAYA_COMMANDS)
            .Msg(
                "LayerCurrentEditTarget::redoIt setting target: %s\n",
                next.GetLayer()->GetDisplayName().c_str());
        stage->SetEditTarget(next);
    } else {
        setResult(AL::maya::utils::convert(previous.GetLayer()->GetDisplayName()));
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerCurrentEditTarget::undoIt()
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("LayerCurrentEditTarget::undoIt\n");
    if (!isQuery) {
        TF_DEBUG(ALUSDMAYA_COMMANDS)
            .Msg(
                "LayerCurrentEditTarget::undoIt setting target: %s\n",
                previous.GetLayer()->GetDisplayName().c_str());
        stage->SetEditTarget(previous);
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// LayerSave
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerSave, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerSave::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.addArg(MSyntax::kString); // Layer name
    syn.addFlag("-f", "-filename", MSyntax::kString);
    syn.addFlag("-s", "-string", MSyntax::kNoArg);
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerSave::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSave::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        MString layerName = args.commandArgumentString(0);
        if (layerName.length() == 0) {
            MGlobal::displayError(
                "LayerSave: you need to specify a layer name that you wish to save");
            throw MS::kFailure;
        }

        SdfLayerHandle       handle;
        nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
        if (layerManager) {
            handle = layerManager->findLayer(layerName.asChar());
        } else {
            MGlobal::displayError("LayerSave: no layer manager in scene (so no layers)");
            throw MS::kFailure;
        }

        LAYER_HANDLE_CHECK(handle);
        if (handle) {
            bool flatten = args.isFlagSet("-fl");
            if (flatten) {
                if (!args.isFlagSet("-f") && !args.isFlagSet("-s")) {
                    MGlobal::displayError(
                        "LayerSave: when using -flatten/-fl, you must specify the filename");
                    throw MS::kFailure;
                }

                // grab the path to the layer.
                const std::string filename = handle->GetRealPath();
                std::string       outfilepath;
                if (args.isFlagSet("-f")) {
                    MString temp;
                    args.getFlagArgument("-f", 0, temp);
                    outfilepath = AL::maya::utils::convert(temp);
                }

                // make sure the user is not going to annihilate their own work.
                // I should probably put more checks in here? Or just remove this check and
                // assume user error is not a thing?
                if (outfilepath == filename) {
                    MGlobal::displayError("LayerSave: nice try, but no, I'm not going to let you "
                                          "overwrite the layer with a flattened version."
                                          "\nthat would seem like a very bad idea to me.");
                    throw MS::kFailure;
                }
            } else {
                bool exportingToString = args.isFlagSet("-s");
                if (exportingToString) {
                    // just set the text string as the result of the command
                    std::string temp;
                    handle->ExportToString(&temp);
                    setResult(AL::maya::utils::convert(temp));
                } else {
                    // if exporting to a file
                    if (args.isFlagSet("-f")) {
                        MString temp;
                        args.getFlagArgument("-f", 0, temp);
                        const std::string filename = AL::maya::utils::convert(temp);
                        bool              result = handle->Export(filename);
                        setResult(result);
                        if (!result)
                            MGlobal::displayError("LayerSave: could not export layer");
                    } else {
                        bool result = handle->Save();
                        setResult(result);
                        if (!result)
                            MGlobal::displayError("LayerSave: could not save layer");
                    }
                }
            }
        } else {
            MGlobal::displayError(
                MString("LayerSave: Could not find layer named '") + layerName + "'");
            throw MS::kFailure;
        }
    } catch (const MStatus& status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set whether the layer is currently muted
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerSetMuted, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerSetMuted::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.addArg(MSyntax::kString); // Layer name
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addFlag("-m", "-muted", MSyntax::kBoolean);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerSetMuted::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        MString layerName = args.commandArgumentString(0);
        if (layerName.length() == 0) {
            MGlobal::displayError(
                "LayerSetMuted: you need to specify a layer name that you wish to set muted");
            throw MS::kFailure;
        }

        if (!args.isFlagSet("-m")) {
            MGlobal::displayError("LayerSetMuted: please tell me whether you want to mute or "
                                  "unmute via the -m <bool> flag");
            throw MS::kFailure;
        }

        nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
        if (layerManager) {
            m_layer = layerManager->findLayer(layerName.asChar());
        } else {
            MGlobal::displayError("LayerSetMuted: no layer manager in scene (so no layers)");
            throw MS::kFailure;
        }
        LAYER_HANDLE_CHECK(m_layer);
        if (!m_layer) {
            MGlobal::displayError("LayerSetMuted: no valid USD layer found on the node");
            throw MS::kFailure;
        }

        args.getFlagArgument("-m", 0, m_muted);
    } catch (const MStatus& status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::undoIt()
{
    if (m_layer)
        m_layer->SetMuted(m_muted);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerSetMuted::redoIt()
{
    if (m_layer)
        m_layer->SetMuted(m_muted);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief Retrieves all the layers that have been set as the EditTarget from any Stage during this
/// session.
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(LayerManager, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax LayerManager::createSyntax()
{
    MSyntax syn = setUpCommonSyntax();
    syn.useSelectionAsDefault(true);
    syn.setObjectType(MSyntax::kSelectionList, 0);
    syn.addFlag("-dal", "-dirtyallLayers", MSyntax::kNoArg);
    syn.addFlag("-dso", "-dirtysessiononly", MSyntax::kNoArg);
    syn.addFlag("-dlo", "-dirtyedittargetlayersonly", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerManager::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        nodes::LayerManager* layerManager = nodes::LayerManager::findManager();

        auto shouldRecord = [&](const MString& currId) {
            if (args.isFlagSet("-dso")) {
                UsdStageRefPtr stage = getShapeNodeStage(args);
                std::string    shapesSessionId = stage->GetSessionLayer()->GetIdentifier();
                if (std::strcmp(currId.asChar(), shapesSessionId.c_str()) != 0) {
                    // only return the dirty session layer for the selected stage
                    return false;
                }
            } else if (args.isFlagSet("-dlo")) {
                std::string displayName = SdfLayer::GetDisplayNameFromIdentifier(currId.asChar());
                std::size_t found = displayName.find("session");
                if (found != std::string::npos) {
                    return false;
                }
            }
            return true;
        };

        MStringArray results;
        if (layerManager) {
            MStringArray identifiers;
            layerManager->getLayerIdentifiers(identifiers);

            for (uint32_t x = 0, n = identifiers.length(); x < n; ++x) {
                MString currId = identifiers[x];

                if (!shouldRecord(currId)) {
                    continue;
                }

                SdfLayerHandle l = layerManager->findLayer(currId.asChar());
                if (l) {
                    std::string str;
                    l->ExportToString(&str);

                    // Write the results in adjacent pairs(id,contents, id,contents)
                    results.append(currId);
                    results.append(AL::maya::utils::convert(str));
                }
            }
        }
        setResult(results);
    } catch (const MStatus& status) {
        return status;
    }

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray buildEditedLayersList(const MString&)
{
    MStringArray         result;
    nodes::LayerManager* layerManager = nodes::LayerManager::findManager();
    if (layerManager) {
        layerManager->getLayerIdentifiers(result);
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray buildProxyLayersList(const MString&)
{
    MStringArray   result;
    MSelectionList sl;
    AL_MAYA_CHECK_ERROR_RETURN_VAL(
        MGlobal::getActiveSelectionList(sl), result, "Error building layer list");

    nodes::ProxyShape* foundShape = getProxyShapeFromSel(sl);
    if (!foundShape) {
        MGlobal::displayError("No proxy shape selected");
        return result;
    }

    auto stage = foundShape->getUsdStage();
    if (!stage) {
        MGlobal::displayError(MString("Proxy shape '") + foundShape->name() + "' had no usd stage");
        return result;
    }

    auto usedLayers = stage->GetUsedLayers();

    for (auto& layer : usedLayers) {
        result.append(layer->GetIdentifier().c_str());
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
void constructLayerCommandGuis()
{
    {
        AL::maya::utils::CommandGuiHelper saveLayer(
            "AL_usdmaya_LayerSave", "Save Layer", "Save Layer", "USD/Layers/Save Layer", false);
        saveLayer.addListOption(
            "l",
            "Layer to Save",
            (AL::maya::utils::GenerateListFn)buildEditedLayersList,
            /*isMandatory=*/true);
        saveLayer.addFilePathOption(
            "f",
            "USD File Path",
            AL::maya::utils::CommandGuiHelper::kSave,
            "USDA files (*.usda) (*.usda);;USDC files (*.usdc) (*.usdc);;Alembic Files (*.abc) "
            "(*.abc);;All Files (*) (*)",
            AL::maya::utils::CommandGuiHelper::kStringMustHaveValue);
    }

    {
        AL::maya::utils::CommandGuiHelper createLayer(
            "AL_usdmaya_LayerCreateLayer",
            "Create Layer on current layer",
            "Create",
            "USD/Layers/Create Sub Layer",
            false);
        createLayer.addExecuteText(" -s ");
        createLayer.addFilePathOption(
            "open",
            "Find or Open Existing Layer",
            AL::maya::utils::CommandGuiHelper::kLoad,
            "USD files (*.usd*) (*.usd*);; Alembic Files (*.abc) (*.abc);;All Files (*) (*)",
            AL::maya::utils::CommandGuiHelper::kStringOptional);
    }

    {
        AL::maya::utils::CommandGuiHelper setEditTarget(
            "AL_usdmaya_LayerCurrentEditTarget",
            "Set Current Edit Target",
            "Set",
            "USD/Layers/Set Current Edit Target",
            false);
        // we build our layer list using identifiers, so make sure the command is told to expect
        // identifiers
        setEditTarget.addExecuteText(" -fid ");
        setEditTarget.addListOption(
            "l", "USD Layer", (AL::maya::utils::GenerateListFn)buildProxyLayersList);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Documentation strings.
//----------------------------------------------------------------------------------------------------------------------
const char* const LayerCreateLayer::g_helpText = R"(
LayerCreateLayer Overview:

  This command provides a way to create new layers in Maya. The Layer identifier passed into the -o will attempt to find the layer,
  and if it doesn't exist then it is created. If a layer is created, it will create a AL::usdmaya::nodes::Layer which will contain a SdfLayerRefPtr
  to the layer opened with -o.

  This command is currently used in our pipeline to create layers on the fly. These layers may then be targeted by an EditTarget for edits
  and these edits are saved into the maya scene file.

  If no identifier is passed, the stage's root layer is used as the parent.

  Examples:
    To create a layer in maya and implicitly parent it to Maya's root layer representation
      AL_usdmaya_LayerCreateLayer -o "path/to/layer.usda" -p "ProxyShape1"

    To create a layer and parent it to a layer existing
      AL_usdmaya_LayerCreateLayer -o "path/to/layer.usda" -p "ProxyShape1"
)";

const char* const LayerGetLayers::g_helpText = R"(
LayerGetLayers Overview:

  This command provides a way to query the various layers on an ProxyShape.
  There are 4 main types of layer that you can query:

    1. Muted layers: These layers are effectively disabled (muted in USD speak).
    2. Used layers: These are the current layers in use by the proxy shape node, as a
       flattened list.
    3. Session Layers: This is the highest level layer, used to store changes made for
       your session, e.g. visibility changes, wireframe display mode, etc.
    4. Layer Stack: This is a stack of layers that can be set as edit targets. This implicitly
       includes the session layer for this current session, but you can choose to filter that
       out.

  An ProxyShape node must either be selected when running this command, or it must be
  specified as the final argument to this command.

  By default, the command will return the USD layer display names (e.g. "myLayer.udsa",
  "myAnonymousTag", "resolve-result.usda"). If you wish to return the raw identifier names
  of the layers ("/full/path/to/myLayer.usda", "anon:0x1655cc0:myAnonymousTag",
  "asset://unresolved/asset/path.usda"), add the flag "-identifiers" to any of the following examples:

Examples:

  To query the muted layers:

      LayerGetLayers -muted "ProxyShape1";

  To query the used layers as a flattened list:

      LayerGetLayers -used "ProxyShape1";

  To query the usd layer stack (without the session layer):

      LayerGetLayers -stack "ProxyShape1";

  To query the usd layer stack (with the session layer):

      LayerGetLayers -stack -sessionLayer "ProxyShape1";

  To query the usd session layer on its own:

      LayerGetLayers -sessionLayer "ProxyShape1";

  To query the usd root layer on its own:

      LayerGetLayers -rootLayer "ProxyShape1";
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerCurrentEditTarget::g_helpText = R"(
LayerCurrentEditTarget Overview:

  Within the USD stage contained within an ProxyShape node, a single layer may be set as the
  edit target at any given time. Any changes made to the contents of a USD proxy node, will end up
  being stored within that layer.

  To determine the current edit target for the currently selected ProxyShape, you can simply
  execute this command:

    LayerCurrentEditTarget -q;

  To determine the edit target on a specific proxy shape node, you can append the name of the shape
  to the end of the command:

    LayerCurrentEditTarget -q "ProxyShape1";

  To set the edit target on a proxy node, there are a few approaches:

  1. Select a ProxyShape, and specify the name of the layer to set via the "-layer" flag:

     LayerCurrentEditTarget -l "identifier/for/layer.usda";

  2. Specify the name of the layer via the "-layer" flag, and specify the ProxyShape name:

     LayerCurrentEditTarget -l "identifier/for/layer.usda" "ProxyShape1";

  3. Specify name of the layer as well as specifying parameters to the EditTargets mapping function
     LayerCurrentEditTarget -tp "/shot_zda01_020/environment" -sp "/ShotSetRoot" -l "identifier/for/layer.usda" "ProxyShape1"

  4. Specify the layer name as an identifier:
     LayerCurrentEditTarget -l "anon:0x136d9050" -fid -proxy "ProxyShape1"


  There are some caveats here though. If no TargetPath and SourcePath prim paths are specified,
  USD will only allow you to set an edit target into what is known as the current layer stack.
  These layers can be determined using the following command:

     LayerGetLayers -stack "ProxyShape1";

  These usually include the current root layer (LayerGetLayers -rootLayer "ProxyShape1"),
  the current session layer ((LayerGetLayers -sessionLayer "ProxyShape1"), and any sub
  layers of those two layers. Attempting to set an edit target on a layer that is not in the layer
  stack and without providing the TargetPath or SourcePath is an error.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerSave::g_helpText = R"(
LayerSave Overview:

  This command allows you to export/save a single layer to a file. In the simplest case, you
  specify the layer name to save, e.g.

     LayerSave "identifier/for/myscene_root.usda";

  If you wish to export that layer and return it as a text string, use the -string/-s flag. The following
  command will return the usd file contents as a string.

     LayerSave -s "identifier/for/myscene_root.usda";

  If you wish to export that layer as a new file, you can also specify the filepath with the -f/-filename
  flag, e.g.

     LayerSave -f "/scratch/stuff/newlayer.usda" "identifier/for/myscene_root.usda";

  In addition, you are also able to flatten a given layer using the -flatten option. When using this
  option, the specified layer will be written out as a new file, and that file will contain ALL of the
  data from that layers child layers and sublayers. This can result in some fairly large files!
  Note: when using the -flatten option, you must specify the -s or -f flags (to write to a string,
  or export as a file)

     LayerSave -flatten -f "/scratch/stuff/phatlayer.usda" "identifier/for/myscene_root.usda";

  or to return a string

     LayerSave -flatten -s "identifier/for/myscene_root.usda";

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerSetMuted::g_helpText = R"(
LayerSetMuted Overview:

  This command allows you to mute or unmute a specified layer:

     LayerSetMuted -m true "identifier/for/layer.usda";  //< mutes the layer 'layer.usda'
     LayerSetMuted -m false "identifier/for/layer.usda";  //< unmutes the layer 'layer.usda'

  This command is undoable, but it will probably crash right now.
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const LayerManager::g_helpText = R"(
LayerManager Command Overview:
  This command retrieves all the layers that have been set as the EditTarget from any Stage during this session.

  Returns a StringArray in the format of [LayerIdentifier_A,LayerContents_A, LayerIdentifier_B,LayerContents_B]

  Retrieves all Layers that have been set as the EditTarget and have been modified:
  LayerManager -dall "ProxyShape1"

  Retrieves the SessionLayer that has been modified for the passed in ProxyShape's stage:
  LayerManager -dso "ProxyShape1"

  Retrieves all Layers, except SessionLayers, that have been set as the EditTarget and have been modified:
  LayerManager -dlo "ProxyShape1"
)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
