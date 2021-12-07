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
#include "AL/usdmaya/nodes/LayerManager.h"

#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"
#include "pxr/usd/ar/resolver.h"

#include <mayaUsd/listeners/notice.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlugArray.h>
#include <maya/MProfiler.h>

#include <mutex>

namespace {
const int _layerManagerProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "LayerManager",
    "LayerManager"
#else
    "LayerManager"
#endif
);

MObjectHandle theLayerManagerHandle;

struct _OnSceneResetListener : public TfWeakBase
{
    _OnSceneResetListener()
    {
        TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Created _OnSceneResetListener\n");
        TfWeakPtr<_OnSceneResetListener> me(this);
        TfNotice::Register(me, &_OnSceneResetListener::OnSceneReset);
    }

    ~_OnSceneResetListener()
    {
        TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Destroyed _OnSceneResetListener\n");
    }

    void OnSceneReset(const UsdMayaSceneResetNotice& notice)
    {
        TF_DEBUG(ALUSDMAYA_LAYERS).Msg("_OnSceneResetListener: Clearing LayerManager Cache\n");
        theLayerManagerHandle = MObject::kNullObj;
    }
};

} // namespace

namespace {
// Global mutex protecting _findNode / findOrCreateNode.
// Recursive because we need to get the mutex inside of conditionalCreator,
// but that may be triggered by the node creation inside of findOrCreateNode.

// Note on layerManager / multithreading:
// I don't know that layerManager will be used in a multithreaded manner... but I also don't know it
// COULDN'T be. (I haven't really looked into the way maya's new multithreaded node evaluation
// works, for instance.) This is essentially a globally shared resource, so I figured better be
// safe...
static std::recursive_mutex _findNodeMutex;

// Utility func to disconnect an array plug, and all it's element plugs, and all
// their child plugs.
// Not in Utils, because it's not generic - ie, doesn't handle general case
// where compound/array plugs may be nested arbitrarily deep...
MStatus disconnectCompoundArrayPlug(MPlug arrayPlug)
{
    const char* errorString = "disconnectCompoundArrayPlug";
    MStatus     status;
    MPlug       elemPlug;
    MPlug       srcPlug;
    MPlugArray  destPlugs;
    MDGModifier dgmod;

    auto disconnectPlug = [&](MPlug plug) -> MStatus {
        MStatus status;
        srcPlug = plug.source(&status);
        AL_MAYA_CHECK_ERROR(status, errorString);
        if (!srcPlug.isNull()) {
            dgmod.disconnect(srcPlug, plug);
        }
        destPlugs.clear();
        plug.destinations(destPlugs, &status);
        AL_MAYA_CHECK_ERROR(status, errorString);
        for (size_t i = 0; i < destPlugs.length(); ++i) {
            dgmod.disconnect(plug, destPlugs[i]);
        }
        return status;
    };

    // Considered using numConnectedElements, but for arrays-of-compound attributes, not sure if
    // this will also detect connections to a child-of-an-element... so just iterating through all
    // plugs. Shouldn't be too many...
    const size_t numElements = arrayPlug.evaluateNumElements();
    // Iterate over all elements...
    for (size_t elemI = 0; elemI < numElements; ++elemI) {
        elemPlug = arrayPlug.elementByPhysicalIndex(elemI, &status);

        // Disconnect the element compound attribute
        AL_MAYA_CHECK_ERROR(status, errorString);
        AL_MAYA_CHECK_ERROR(disconnectPlug(elemPlug), errorString);

        // ...then disconnect any children
        if (elemPlug.numConnectedChildren() > 0) {
            for (size_t childI = 0; childI < elemPlug.numChildren(); ++childI) {
                AL_MAYA_CHECK_ERROR(disconnectPlug(elemPlug.child(childI)), errorString);
            }
        }
    }
    return dgmod.doIt();
}
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
bool LayerDatabase::addLayer(SdfLayerRefPtr layer, const std::string& identifier)
{
    auto insertLayerResult = m_layerToIds.emplace(
        std::piecewise_construct, std::forward_as_tuple(layer), std::forward_as_tuple());
    auto& idsForLayer = insertLayerResult.first->second;

    _addLayer(layer, layer->GetIdentifier(), idsForLayer);
    if (identifier != layer->GetIdentifier() && !identifier.empty()) {
        _addLayer(layer, identifier, idsForLayer);
    }
    return insertLayerResult.second;
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerDatabase::removeLayer(SdfLayerRefPtr layer)
{
    auto foundLayerAndIds = m_layerToIds.find(layer);
    if (foundLayerAndIds == m_layerToIds.end())
        return false;

    for (std::string& oldId : foundLayerAndIds->second) {
        auto oldIdPosition = m_idToLayer.find(oldId);
#ifdef DEBUG
        assert(oldIdPosition != m_idToLayer.end());
#else
        if (oldIdPosition == m_idToLayer.end()) {
            MGlobal::displayError(
                MString("Error - layer '") + AL::maya::utils::convert(layer->GetIdentifier())
                + "' could be found indexed by layer, but not by identifier '"
                + AL::maya::utils::convert(oldId) + "'");
        } else
#endif // DEBUG
        {
            m_idToLayer.erase(oldIdPosition);
        }
    }
    m_layerToIds.erase(foundLayerAndIds);
    return true;
}

SdfLayerHandle LayerDatabase::findLayer(std::string identifier) const
{
    auto foundIdAndLayer = m_idToLayer.find(identifier);
    if (foundIdAndLayer != m_idToLayer.end()) {
        // Non-dirty layers may be placed in the database "temporarily" -
        // ie, current edit targets for proxyShape stages, that have not
        // yet been edited. Filter those out.
        if (foundIdAndLayer->second->IsDirty()) {
            return foundIdAndLayer->second;
        }
    }

    return SdfLayerHandle();
}

//----------------------------------------------------------------------------------------------------------------------
void LayerDatabase::_addLayer(
    SdfLayerRefPtr            layer,
    const std::string&        identifier,
    std::vector<std::string>& idsForLayer)
{
    // Try to insert into m_idToLayer...
    auto insertIdResult = m_idToLayer.emplace(identifier, layer);
    if (!insertIdResult.second) {
        // We've seen this identifier before...
        if (insertIdResult.first->second == layer) {
            // ...and it was referring to the same layer. Nothing to do!
            return;
        }

        // If it was pointing to a DIFFERENT layer, we need to first remove
        // this id from the set of ids for the OLD layer...
        SdfLayerRefPtr oldLayer = insertIdResult.first->second;
        auto           oldLayerAndIds = m_layerToIds.find(oldLayer);
#ifdef DEBUG
        assert(oldLayerAndIds != m_layerToIds.end());
#else
        if (oldLayerAndIds == m_layerToIds.end()) {
            // The layer didn't exist in the opposite direction - this should
            // never happen, but don't want to crash if it does
            MGlobal::displayError(
                MString("Error - layer '") + AL::maya::utils::convert(identifier)
                + "' could be found indexed by identifier, but not by layer");
        } else
#endif // DEBUG
        {
            auto& oldLayerIds = oldLayerAndIds->second;
            if (oldLayerIds.size() <= 1) {
                // This was the ONLY identifier for the layer - so delete
                // the layer entirely!
                m_layerToIds.erase(oldLayerAndIds);
            } else {
                auto idLocation = std::find(oldLayerIds.begin(), oldLayerIds.end(), identifier);
#ifdef DEBUG
                assert(idLocation != oldLayerIds.end());
#else
                if (idLocation == oldLayerIds.end()) {
                    MGlobal::displayError(
                        MString("Error - layer '") + AL::maya::utils::convert(identifier)
                        + "' could be found indexed by identifier, but was not in layer's list of "
                          "identifiers");
                } else
#endif
                {
                    oldLayerIds.erase(idLocation);
                }
            }
        }

        // Ok, we've cleaned up the OLD layer - now make the id point to our
        // NEW layer
        insertIdResult.first->second = layer;
    }

    // Ok, we've now added the layer to m_idToLayer, and cleaned up
    // any potential old entries from m_layerToIds. Now we just need
    // to add the identifier to idsForLayer (which should be == m_layerToIds[layer])
    idsForLayer.push_back(identifier);
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(LayerManager, AL_USDMAYA_LAYERMANAGER, AL_usdmaya);

// serialization
MObject LayerManager::m_layers = MObject::kNullObj;
MObject LayerManager::m_identifier = MObject::kNullObj;
MObject LayerManager::m_fileFormatId = MObject::kNullObj;
MObject LayerManager::m_serialized = MObject::kNullObj;
MObject LayerManager::m_anonymous = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
void* LayerManager::conditionalCreator()
{
    // If we were called from findOrCreate, we don't need to call findNode, we already did
    MObject theManager = findNode();
    if (!theManager.isNull()) {
        MFnDependencyNode fn(theManager);
        MGlobal::displayError(
            MString("cannot create a new '") + kTypeName
            + "' node, an unreferenced"
              " one already exists: "
            + fn.name());
        return nullptr;
    }
    return creator();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerManager::initialise()
{
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::initialize\n");
    try {
        setNodeType(kTypeName);
        addFrame("USD Layer Manager Node");

        addFrame("Serialization infos");

        // add attributes to store the serialization info
        m_identifier = addStringAttr("identifier", "id", kCached | kReadable | kStorable | kHidden);
        m_fileFormatId
            = addStringAttr("fileFormatId", "fid", kCached | kReadable | kStorable | kHidden);
        m_serialized
            = addStringAttr("serialized", "szd", kCached | kReadable | kStorable | kHidden);
        m_anonymous
            = addBoolAttr("anonymous", "ann", false, kCached | kReadable | kStorable | kHidden);
        m_layers = addCompoundAttr(
            "layers",
            "lyr",
            kCached | kReadable | kWritable | kStorable | kConnectable | kHidden | kArray
                | kUsesArrayDataBuilder,
            { m_identifier, m_fileFormatId, m_serialized, m_anonymous });
    } catch (const MStatus& status) {
        return status;
    }
    generateAETemplate();
    static _OnSceneResetListener onSceneResetListener;
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MObject LayerManager::findNode()
{
    std::lock_guard<std::recursive_mutex> lock(_findNodeMutex);
    return _findNode();
}

//----------------------------------------------------------------------------------------------------------------------
MObject LayerManager::_findNode()
{
    if (theLayerManagerHandle.isValid() && theLayerManagerHandle.isAlive()) {
        MObject theManager { theLayerManagerHandle.object() };
        if (!theManager.isNull()) {
            return theManager;
        } else {
            TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::_findNode cache got null mobject\n");
        }
    } else {
        TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::_findNode cache got invalid mobjecthandle\n");
    }

    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        fn.setObject(mobj);
        if (fn.typeId() == kTypeId && !fn.isFromReferencedFile()) {
            theLayerManagerHandle = mobj;
            return mobj;
        }
    }
    return MObject::kNullObj;
}

//----------------------------------------------------------------------------------------------------------------------
// TODO: make it take an optional MDGModifier
MObject LayerManager::findOrCreateNode(MDGModifier* dgmod, bool* wasCreated)
{
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::findOrCreateNode\n");
    std::lock_guard<std::recursive_mutex> lock(_findNodeMutex);
    MObject                               theManager = _findNode();

    if (!theManager.isNull()) {
        if (wasCreated)
            *wasCreated = false;
        return theManager;
    }

    if (wasCreated)
        *wasCreated = true;

    if (dgmod) {
        return dgmod->createNode(kTypeId);
    } else {
        MDGModifier modifier;
        MObject     node = modifier.createNode(kTypeId);
        modifier.doIt();
        return node;
    }
}

//----------------------------------------------------------------------------------------------------------------------
LayerManager* LayerManager::findManager()
{
    MObject manager = findNode();
    if (manager.isNull()) {
        return nullptr;
    }
    return static_cast<LayerManager*>(MFnDependencyNode(manager).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
LayerManager* LayerManager::findOrCreateManager(MDGModifier* dgmod, bool* wasCreated)
{
    return static_cast<LayerManager*>(
        MFnDependencyNode(findOrCreateNode(dgmod, wasCreated)).userNode());
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::addLayer(SdfLayerHandle layer, const std::string& identifier)
{
    SdfLayerRefPtr layerRef(layer);
    if (!layerRef) {
        MGlobal::displayError("LayerManager::addLayer - given layer is no longer valid");
        return false;
    }
    std::unique_lock<std::shared_timed_mutex> lock(m_layersMutex);
    return m_layerDatabase.addLayer(layerRef, identifier);
}

//----------------------------------------------------------------------------------------------------------------------
bool LayerManager::removeLayer(SdfLayerHandle layer)
{
    SdfLayerRefPtr layerRef(layer);
    if (!layerRef) {
        MGlobal::displayError("LayerManager::removeLayer - given layer is no longer valid");
        return false;
    }
    std::unique_lock<std::shared_timed_mutex> lock(m_layersMutex);
    return m_layerDatabase.removeLayer(layerRef);
}

//----------------------------------------------------------------------------------------------------------------------
SdfLayerHandle LayerManager::findLayer(std::string identifier)
{
    std::shared_lock<std::shared_timed_mutex> lock(m_layersMutex);
    return m_layerDatabase.findLayer(identifier);
}

//----------------------------------------------------------------------------------------------------------------------
void LayerManager::getLayerIdentifiers(MStringArray& outputNames)
{
    outputNames.clear();
    for (const auto& layerAndIds : m_layerDatabase) {
        outputNames.append(layerAndIds.first->GetIdentifier().c_str());
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus LayerManager::populateSerialisationAttributes()
{
    MProfilingScope profilerScope(
        _layerManagerProfilerCategory, MProfiler::kColorE_L3, "Populate serialisation attributes");

    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::populateSerialisationAttributes\n");
    const char* errorString = "LayerManager::populateSerialisationAttributes";

    MStatus status;
    MPlug   arrayPlug = layersPlug();

    // First, disconnect any connected attributes
    AL_MAYA_CHECK_ERROR(disconnectCompoundArrayPlug(arrayPlug), errorString);

    // Then fill out the array attribute
    MDataBlock dataBlock = forceCache();

    MArrayDataHandle layersArrayHandle = dataBlock.outputArrayValue(m_layers, &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    {
        std::shared_lock<std::shared_timed_mutex> lock(m_layersMutex);
        MArrayDataBuilder builder(&dataBlock, layers(), m_layerDatabase.max_size(), &status);
        AL_MAYA_CHECK_ERROR(status, errorString);
        std::string temp;
        for (const auto& layerAndIds : m_layerDatabase) {
            auto&       layer = layerAndIds.first;
            MDataHandle layersElemHandle = builder.addLast(&status);
            AL_MAYA_CHECK_ERROR(status, errorString);
            MDataHandle idHandle = layersElemHandle.child(m_identifier);
            idHandle.setString(AL::maya::utils::convert(layer->GetIdentifier()));
            MDataHandle fileFormatIdHandle = layersElemHandle.child(m_fileFormatId);
            auto        fileFormatIdToken = layer->GetFileFormat()->GetFormatId();
            fileFormatIdHandle.setString(AL::maya::utils::convert(fileFormatIdToken.GetString()));
            MDataHandle serializedHandle = layersElemHandle.child(m_serialized);
            layer->ExportToString(&temp);
            serializedHandle.setString(AL::maya::utils::convert(temp));
            MDataHandle anonHandle = layersElemHandle.child(m_anonymous);
            anonHandle.setBool(layer->IsAnonymous());
        }
        AL_MAYA_CHECK_ERROR(layersArrayHandle.set(builder), errorString);
    }
    AL_MAYA_CHECK_ERROR(layersArrayHandle.setAllClean(), errorString);
    AL_MAYA_CHECK_ERROR(dataBlock.setClean(layers()), errorString);
    return status;
}

MStatus LayerManager::clearSerialisationAttributes()
{
    MProfilingScope profilerScope(
        _layerManagerProfilerCategory, MProfiler::kColorE_L3, "Clear serialisation attributes");

    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::clearSerialisationAttributes\n");
    const char* errorString = "LayerManager::clearSerialisationAttributes";

    MStatus status;
    MPlug   arrayPlug = layersPlug();

    // First, disconnect any connected attributes
    AL_MAYA_CHECK_ERROR(disconnectCompoundArrayPlug(arrayPlug), errorString);

    // Then wipe the array attribute
    MDataBlock       dataBlock = forceCache();
    MArrayDataHandle layersArrayHandle = dataBlock.outputArrayValue(m_layers, &status);
    AL_MAYA_CHECK_ERROR(status, errorString);

    MArrayDataBuilder builder(&dataBlock, layers(), 0, &status);
    AL_MAYA_CHECK_ERROR(status, errorString);
    AL_MAYA_CHECK_ERROR(layersArrayHandle.set(builder), errorString);
    AL_MAYA_CHECK_ERROR(layersArrayHandle.setAllClean(), errorString);
    AL_MAYA_CHECK_ERROR(dataBlock.setClean(layers()), errorString);
    return status;
}

void LayerManager::loadAllLayers()
{
    MProfilingScope profilerScope(
        _layerManagerProfilerCategory, MProfiler::kColorE_L3, "Load all layers");

    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("LayerManager::loadAllLayers\n");
    const char*                        errorString = "LayerManager::loadAllLayers";
    const char*                        identifierTempSuffix = "_tmp";
    MStatus                            status;
    MPlug                              allLayersPlug = layersPlug();
    MPlug                              singleLayerPlug;
    MPlug                              idPlug;
    MPlug                              fileFormatIdPlug;
    MPlug                              anonymousPlug;
    MPlug                              serializedPlug;
    std::string                        identifierVal;
    std::string                        fileFormatIdVal;
    std::string                        serializedVal;
    std::string                        sessionLayerIdentifier;
    std::map<std::string, std::string> sessionSublayerNames;
    SdfLayerRefPtr                     layer;
    // We DON'T want to use evaluate num elements, because we don't want to trigger
    // a compute - we want the value(s) as read from the file!
    const unsigned int numElements = allLayersPlug.numElements();
    for (unsigned int i = 0; i < numElements; ++i) {
        singleLayerPlug = allLayersPlug.elementByPhysicalIndex(i, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        idPlug = singleLayerPlug.child(m_identifier, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        fileFormatIdPlug = singleLayerPlug.child(m_fileFormatId, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        anonymousPlug = singleLayerPlug.child(m_anonymous, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        serializedPlug = singleLayerPlug.child(m_serialized, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);

        identifierVal = idPlug.asString(MDGContext::fsNormal, &status).asChar();
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        if (identifierVal.empty()) {
            MGlobal::displayError(
                MString("Error - plug ") + idPlug.partialName(true) + "had empty identifier");
            continue;
        }
        fileFormatIdVal = fileFormatIdPlug.asString(MDGContext::fsNormal, &status).asChar();
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        if (fileFormatIdVal.empty()) {
            MGlobal::displayInfo(
                MString("No file format in ") + fileFormatIdPlug.partialName(true)
                + " plug. Will use identifier to work it out.");
        }
        serializedVal = serializedPlug.asString(MDGContext::fsNormal, &status).asChar();
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        if (serializedVal.empty()) {
            MGlobal::displayError(
                MString("Error - plug ") + serializedPlug.partialName(true)
                + "had empty serialization");
            continue;
        }

        bool isAnon = anonymousPlug.asBool(MDGContext::fsNormal, &status);
        AL_MAYA_CHECK_ERROR_CONTINUE(status, errorString);
        if (isAnon) {
            // Note that the new identifier will not match the old identifier - only the "tag" will
            // be retained
            // if this layer is an anonymous sublayer of the session layer, these will be replaced
            // with the new identifier
            layer
                = SdfLayer::CreateAnonymous(SdfLayer::GetDisplayNameFromIdentifier(identifierVal));

            // store old:new name so we can replace the session layers anonymous subLayers
            sessionSublayerNames[identifierVal] = layer->GetIdentifier().c_str();

            // Check if this is the session layer
            // Used later to update the session layers anonymous subLayer naming
            if (sessionLayerIdentifier.empty() && TfStringEndsWith(identifierVal, "session.usda")) {
                sessionLayerIdentifier = layer->GetIdentifier().c_str();
            }

        } else {
            SdfLayerHandle layerHandle = SdfLayer::Find(identifierVal);
            if (layerHandle) {
                layer = layerHandle;
            } else {
                // TODO: currently, there is a small window here, after the find, and before the
                // New, where another process might sneak in and create a layer with the same
                // identifier, which could cause an error. This seems unlikely, but we have a
                // discussion with Pixar to find a way to avoid this.

                SdfFileFormatConstPtr fileFormat;
                if (!fileFormatIdVal.empty()) {
                    fileFormat = SdfFileFormat::FindById(TfToken(fileFormatIdVal));
                } else {
                    fileFormat = SdfFileFormat::FindByExtension(
                        ArGetResolver().GetExtension(identifierVal));
                    if (!fileFormat) {
                        MGlobal::displayError(
                            MString("Cannot determine file format for identifier '")
                            + identifierVal.c_str() + "' for plug " + idPlug.partialName(true));
                        continue;
                    }
                }

                // In order to make the layer reloadable by SdfLayer::Reload(), we hack the
                // identifier with temp one on creation and call layer->SetIdentifier() again to set
                // the timestamp:
                layer = SdfLayer::New(fileFormat, identifierVal + identifierTempSuffix);
                if (!layer) {
                    MGlobal::displayError(
                        MString("Error - failed to create new layer for identifier '")
                        + identifierVal.c_str() + "' for plug " + idPlug.partialName(true));
                    continue;
                }
                layer->SetIdentifier(identifierVal); // Make it reloadable by SdfLayer::Reload(true)
                layer->Clear(); // Mark it dirty to make it reloadable by SdfLayer::Reload() without
                                // force=true
            }
        }

        TF_DEBUG(ALUSDMAYA_LAYERS)
            .Msg(
                "################################################\n"
                "Importing layer from serialised data:\n"
                "old identifier: %s\n"
                "new identifier: %s\n"
                "format: %s\n",
                identifierVal.c_str(),
                layer->GetIdentifier().c_str(),
                layer->GetFileFormat()->GetFormatId().GetText());

        if (!layer->ImportFromString(serializedVal)) {
            TF_DEBUG(ALUSDMAYA_LAYERS)
                .Msg("Import result: failed!\n"
                     "################################################\n");
            MGlobal::displayError(
                MString("Failed to import serialized layer: ") + serializedVal.c_str());
            continue;
        }
        TF_DEBUG(ALUSDMAYA_LAYERS)
            .Msg("Import result: success!\n"
                 "################################################\n");
        addLayer(layer, identifierVal);
    }

    // Update the name of any anonymous sublayers in the session layer
    SdfLayerHandle sessionLayer = SdfLayer::Find(sessionLayerIdentifier);

    if (sessionLayer) {
        // TODO drill down and apply through session sublayers to enable recursive anonymous
        // sublayers
        auto subLayerPaths = sessionLayer->GetSubLayerPaths();

        typedef std::map<std::string, std::string>::const_iterator MapIterator;
        for (MapIterator iter = sessionSublayerNames.begin(); iter != sessionSublayerNames.end();
             iter++) {
            std::replace(subLayerPaths.begin(), subLayerPaths.end(), iter->first, iter->second);
        }

        sessionLayer->SetSubLayerPaths(subLayerPaths);
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
