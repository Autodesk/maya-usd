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
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <maya/MFnDagNode.h>
#include <maya/MSelectionList.h>

#include <string>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
TranslatorContext::~TranslatorContext() { }

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr TranslatorContext::getUsdStage() const
{
    if (m_proxyShape)
        return m_proxyShape->usdStage();
    return UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::validatePrims()
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::validatePrims ** VALIDATE PRIMS **\n");
    for (auto it : m_primMapping) {
        if (it.objectHandle().isValid() && it.objectHandle().isAlive()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::validatePrims ** VALID HANDLE DETECTED %s **\n",
                    it.path().GetText());
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getTransform(const SdfPath& path, MObjectHandle& object)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getTransform %s\n", path.GetText());
    auto it = find(path);
    if (it != m_primMapping.end()) {
        if (!it->objectHandle().isValid()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("TranslatorContext::getTransform - invalid handle\n");
            return false;
        }
        object = it->object();
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::updatePrimTypes()
{
    auto stage = m_proxyShape->usdStage();
    for (auto it = m_primMapping.begin(); it != m_primMapping.end();) {
        SdfPath path(it->path());
        UsdPrim prim = stage->GetPrimAtPath(path);
        bool    modifiedIt = false;
        if (!prim) {
            it = m_primMapping.erase(it);
            modifiedIt = true;
        } else {
            std::string translatorId
                = m_proxyShape->translatorManufacture().generateTranslatorId(prim);
            if (it->translatorId() != translatorId) {
                it->translatorId() = translatorId;
                ++it;
                modifiedIt = true;
            }
        }
        if (!modifiedIt) {
            ++it;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MTypeId typeId)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject '%s' \n", path.GetText());

    auto it = find(path);
    if (it != m_primMapping.end()) {
        const MTypeId zero(0);
        if (zero != typeId) {
            for (auto temp : it->createdNodes()) {
                MFnDependencyNode fn(temp.object());
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg("TranslatorContext::getMObject getting %s\n", fn.typeName().asChar());
                if (fn.typeId() == typeId) {
                    object = temp;
                    if (!object.isAlive())
                        MGlobal::displayError(
                            MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
                    if (!object.isValid())
                        MGlobal::displayError(
                            MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
                    return true;
                }
            }
        } else {
            if (!it->createdNodes().empty()) {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "TranslatorContext::getMObject getting anything %s\n",
                        path.GetString().c_str());
                object = it->createdNodes()[0];

                if (!object.isAlive())
                    MGlobal::displayError(
                        MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
                if (!object.isValid())
                    MGlobal::displayError(
                        MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MFn::Type type)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject '%s' \n", path.GetText());

    auto it = find(path);
    if (it != m_primMapping.end()) {
        const MTypeId zero(0);
        if (MFn::kInvalid != type) {
            for (auto temp : it->createdNodes()) {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg("TranslatorContext::getMObject getting: %s\n", temp.object().apiTypeStr());
                if (temp.object().apiType() == type) {
                    object = temp;
                    if (!object.isAlive())
                        MGlobal::displayError(
                            MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
                    if (!object.isValid())
                        MGlobal::displayError(
                            MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
                    return true;
                }
            }
        } else {
            if (!it->createdNodes().empty()) {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "TranslatorContext::getMObject getting anything: %s\n",
                        path.GetString().c_str());
                object = it->createdNodes()[0];

                if (!object.isAlive())
                    MGlobal::displayError(
                        MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
                if (!object.isValid())
                    MGlobal::displayError(
                        MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getMObjects(const SdfPath& path, MObjectHandleArray& returned)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObjects: %s\n", path.GetText());
    auto it = find(path);
    if (it != m_primMapping.end()) {
        returned = it->createdNodes();
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::registerItem(const UsdPrim& prim, MObjectHandle object)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorContext::registerItem adding entry %s[%s]\n",
            prim.GetPath().GetText(),
            object.object().apiTypeStr());
    auto iter = findLocation(prim.GetPath());
    if (iter == m_primMapping.end() || iter->path() != prim.GetPath()) {
        // We keep around this legacy plugin identification by type only to allow tests which don't
        // create a proxy shape to run..
        std::string translatorId = m_proxyShape
            ? m_proxyShape->translatorManufacture().generateTranslatorId(prim)
            : "schematype:" + prim.GetTypeName().GetString();

        iter
            = m_primMapping.insert(iter, PrimLookup(prim.GetPath(), translatorId, object.object()));
    } else {
        iter->setNode(object.object());
    }

    if (object.object() == MObject::kNullObj) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorContext::registerItem primPath=%s translatorId=%s to null MObject\n",
                prim.GetPath().GetText(),
                iter->translatorId().c_str());
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorContext::registerItem primPath=%s translatorId=%s to MObject type %s\n",
                prim.GetPath().GetText(),
                iter->translatorId().c_str(),
                object.object().apiTypeStr());
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::insertItem(const UsdPrim& prim, MObjectHandle object)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorContext::insertItem adding entry %s[%s]\n",
            prim.GetPath().GetText(),
            object.object().apiTypeStr());

    auto iter = findLocation(prim.GetPath());
    if (iter == m_primMapping.end() || iter->path() != prim.GetPath()) {
        // We keep around this legacy plugin identification by type only to allow tests which don't
        // create a proxy shape to run..
        std::string translatorId = m_proxyShape
            ? m_proxyShape->translatorManufacture().generateTranslatorId(prim)
            : "schematype:" + prim.GetTypeName().GetString();

        iter = m_primMapping.insert(iter, PrimLookup(prim.GetPath(), translatorId, MObject()));
    }

    if (object.object() == MObject::kNullObj) {
        return;
    }

    iter->createdNodes().push_back(object);

    if (object.object() == MObject::kNullObj) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorContext::insertItem primPath=%s translatorId=%s to null MObject\n",
                prim.GetPath().GetText(),
                iter->translatorId().c_str());
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorContext::insertItem primPath=%s translatorId=%s to MObject type %s\n",
                prim.GetPath().GetText(),
                iter->translatorId().c_str(),
                object.object().apiTypeStr());
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::removeItems(const SdfPath& path)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorContext::removeItems remove under primPath=%s\n", path.GetText());
    auto it = find(path);
    if (it != m_primMapping.end() && it->path() == path) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg("TranslatorContext::removeItems removing path=%s\n", it->path().GetText());
        MDGModifier        modifier1;
        MDagModifier       modifier2;
        MObjectHandleArray tempXforms;
        MStatus            status;

        // Store the DAG nodes to delete in a vector which we will sort via their path length
        std::vector<std::pair<int, MObject>> dagNodesToDelete;

        auto& nodes = it->createdNodes();
        for (std::size_t j = 0, n = nodes.size(); j < n; ++j) {
            if (nodes[j].isAlive() && nodes[j].isValid()) {
                // Need to reparent nodes first to avoid transform getting deleted and triggering
                // ancestor deletion rather than just deleting directly.
                MObject obj = nodes[j].object();

                if (obj.hasFn(MFn::kDagNode)) {
                    MFnDagNode fn(obj);
                    MDagPath   path;
                    fn.getPath(path);
                    dagNodesToDelete.emplace_back(path.fullPathName().length(), obj);
                    AL_MAYA_CHECK_ERROR2(status, "failed to delete dag node");
                } else {
                    status = modifier1.deleteNode(obj);
                    status = modifier1.doIt();
                    AL_MAYA_CHECK_ERROR2(status, MString("failed to delete node"));
                }
            } else {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "TranslatorContext::removeItems Invalid MObject was registered with the "
                        "primPath \"%s\"\n",
                        path.GetText());
            }
        }
        nodes.clear();

        if (!dagNodesToDelete.empty()) {
            std::sort(
                dagNodesToDelete.begin(),
                dagNodesToDelete.end(),
                [](const std::pair<int, MObject>& a, const std::pair<int, MObject>& b) {
                    return a.first > b.first;
                });

            for (auto it : dagNodesToDelete) {
                MObjectHandle handle(it.second);
                if (handle.isAlive() && handle.isValid()) {
                    if (it.second.hasFn(MFn::kPluginEmitterNode)
                        || it.second.hasFn(MFn::kPluginTransformNode)
                        || it.second.hasFn(MFn::kPluginConstraintNode)
                        || it.second.hasFn(MFn::kTransform)) {
                        // reparent custom transform nodes under the world prior to deletion.
                        // This avoids the problem where deleting a custom transform node
                        // automatically deletes the parent transform (which cascades up the
                        // hierarchy until the entire scene has been deleted)
                        status = modifier2.reparentNode(it.second);
                        modifier2.deleteNode(it.second);
                    } else if (
                        it.second.hasFn(MFn::kPluginShape)
                        || it.second.hasFn(MFn::kPluginImagePlaneNode)
                        || it.second.hasFn(MFn::kShape)) {
                        // same issue exists with custom shape nodes, except that we need to create
                        // a temp transform node to parent the shape under (otherwise the reparent
                        // operation will fail!)
                        MObject node = modifier2.createNode("transform");
                        status = modifier2.reparentNode(it.second, node);
                        status = modifier2.doIt();
                        modifier2.deleteNode(node);
                    } else {
                        modifier2.deleteNode(it.second);
                    }
                    status = modifier2.doIt();
                }
            }
            AL_MAYA_CHECK_ERROR2(status, "failed to delete dag nodes");
        }
        m_primMapping.erase(it);
    }
    validatePrims();
}

//----------------------------------------------------------------------------------------------------------------------
MString getNodeName(MObject obj)
{
    if (obj.hasFn(MFn::kDagNode)) {
        MFnDagNode fn(obj);
        MDagPath   path;
        fn.getPath(path);
        return path.fullPathName();
    }
    MFnDependencyNode fn(obj);
    return fn.name();
}

//----------------------------------------------------------------------------------------------------------------------
MString TranslatorContext::serialise() const
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext:serialise\n");
    std::ostringstream oss;
    for (auto& path : m_excludedGeometry) {
        oss << path.first.GetString() << ",";
    }

    m_proxyShape->excludedTranslatedGeometryPlug().setString(MString(oss.str().c_str()));

    oss.str("");
    oss.clear();

    for (auto it : m_primMapping) {
        oss << it.path() << "=" << it.translatorId() << ",";
        oss << getNodeName(it.object());
        for (uint32_t i = 0; i < it.createdNodes().size(); ++i) {
            oss << "," << getNodeName(it.createdNodes()[i].object());
        }
        if (it.uniqueKey()) {
            oss << ",uniquekey:" << it.uniqueKey();
        }
        oss << ";";
    }
    return MString(oss.str().c_str());
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::deserialise(const MString& string)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext:deserialise\n");
    MStringArray strings;
    string.split(';', strings);

    for (uint32_t i = 0; i < strings.length(); ++i) {
        MStringArray strings2;
        strings[i].split('=', strings2);

        MStringArray strings3;
        strings2[1].split(',', strings3);

        MObject obj;
        {
            MSelectionList sl;
            sl.add(strings3[1].asChar());
            sl.getDependNode(0, obj);
        }

        PrimLookup lookup(SdfPath(strings2[0].asChar()), strings3[0].asChar(), obj);

        static const MString uniqueKeyPrefix("uniquekey:");

        for (uint32_t j = 2; j < strings3.length(); ++j) {
            if (strings3[j].substring(0, 10) == uniqueKeyPrefix) {
                auto keyStr(strings3[j].substring(10, strings3[j].length()));
                if (keyStr.length()) {
                    try {
                        lookup.setUniqueKey(std::stoul(keyStr.asChar()));
                    } catch (std::logic_error&) {
                        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                            .Msg(
                                "TranslatorContext:deserialise ignored invalid hash value for "
                                "prim='%s' [hash='%s']\n",
                                lookup.path().GetText(),
                                keyStr.asChar());
                    }
                }
                continue;
            }

            MSelectionList sl;
            sl.add(strings3[j].asChar());
            MObject obj;
            sl.getDependNode(0, obj);
            lookup.createdNodes().push_back(obj);
        }

        // Check for any prim lookup duplicates.
        // This assumes lookups have 1:1 mapping of prim to translator, and that
        // multiple translators can not be registered against the same prim type.
        auto iter
            = std::find_if(m_primMapping.begin(), m_primMapping.end(), [&](const PrimLookup& p) {
                  return p.path() == lookup.path();
              });
        if (iter == m_primMapping.end())
            m_primMapping.push_back(lookup);
    }

    SdfPathVector vec = m_proxyShape->getPrimPathsFromCommaJoinedString(
        m_proxyShape->excludedTranslatedGeometryPlug().asString());
    for (auto& it : vec) {
        m_excludedGeometry.emplace(it, it);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::preRemoveEntry(
    const SdfPath& primPath,
    SdfPathVector& itemsToRemove,
    bool           callPreUnload)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorContext::preRemoveEntry primPath=%s\n", primPath.GetText());

    PrimLookups::iterator end = m_primMapping.end();
    PrimLookups::iterator range_begin
        = std::lower_bound(m_primMapping.begin(), end, primPath, value_compare());
    PrimLookups::iterator range_end = range_begin;
    for (; range_end != end; ++range_end) {
        // due to the joys of sorting, any child prims of this prim being destroyed should appear
        // next to each other (one would assume); So if compare does not find a match (the value is
        // something other than zero), we are no longer in the same prim root
        const SdfPath& childPath = range_end->path();

        if (!childPath.HasPrefix(primPath)) {
            break;
        }
    }

    auto stage = m_proxyShape->usdStage();

    // run the preTearDown stage on each prim. We will walk over the prims in the reverse order here
    // (which will guarentee the the itemsToRemove will be ordered such that the child prims will be
    // destroyed before their parents).
    auto iter = range_end;
    itemsToRemove.reserve(itemsToRemove.size() + (range_end - range_begin));
    while (iter != range_begin) {
        --iter;
        PrimLookup& node = *iter;

        if (std::find(itemsToRemove.begin(), itemsToRemove.end(), node.path())
            != itemsToRemove.end()) {
            // Same exact path has already been processed and added to the list of itemsToRemove.
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::preRemoveEntry skipping path thats already in "
                    "itemsToRemove. primPath=%s\n",
                    primPath.GetText());
        } else {
            itemsToRemove.push_back(node.path());
            auto prim = stage->GetPrimAtPath(node.path());
            if (prim && callPreUnload) {
                preUnloadPrim(prim, node.object());
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::removeEntries(const SdfPathVector& itemsToRemove)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries\n");
    auto stage = m_proxyShape->usdStage();

    MDagModifier modifier;
    MStatus      status;

    // so now we need to unload the prims (itemsToRemove is reverse sorted so we won't nuke parents
    // before children)
    auto iter = itemsToRemove.begin();
    while (iter != itemsToRemove.end()) {
        auto path = *iter;
        auto node
            = std::lower_bound(m_primMapping.begin(), m_primMapping.end(), path, value_compare());
        if (node == m_primMapping.end()) {
            ++iter;
            continue;
        }
        bool isInTransformChain = isPrimInTransformChain(path);
        auto primMappingSize = m_primMapping.size();

        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg("TranslatorContext::removeEntries removing: %s\n", iter->GetText());
        if (node->objectHandle().isValid() && node->objectHandle().isAlive()) {
            unloadPrim(path, node->object());
        }

        // The item might already have been removed by a translator...
        if (primMappingSize == m_primMapping.size()) {
            // remove nodes from map
            m_primMapping.erase(node);
        }

        if (isInTransformChain) {
            m_proxyShape->removeUsdTransformChain(path, modifier, nodes::ProxyShape::kRequired);
        }

        ++iter;
    }
    status = modifier.doIt();
    AL_MAYA_CHECK_ERROR2(status, "failed to remove translator prims.");
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::updateUniqueKeys()
{
    auto stage = getUsdStage();
    for (auto& lookup : m_primMapping) {
        const auto& prim = stage->GetPrimAtPath(lookup.path());
        if (prim) {
            std::string translatorId = getTranslatorIdForPath(lookup.path());
            auto        translator
                = m_proxyShape->translatorManufacture().getTranslatorFromId(translatorId);
            if (translator) {
                auto key(translator->generateUniqueKey(prim));
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "TranslatorContext::updateUniqueKeys [generateUniqueKey] prim='%s', "
                        "uniqueKey='%lu'\n",
                        lookup.path().GetText(),
                        key);
                lookup.setUniqueKey(key);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::updateUniqueKey(const UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::updateUniqueKey\n");

    const auto path(prim.GetPath());
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg(
            "TranslatorContext::updateUniqueKey [generateUniqueKey] updating unique key for "
            "prim='%s'\n",
            path.GetText());

    std::string translatorId = getTranslatorIdForPath(path);
    auto translator = m_proxyShape->translatorManufacture().getTranslatorFromId(translatorId);
    if (translator) {
        auto it = find(path);
        if (it != m_primMapping.end() && it->path() == path) {
            auto key(translator->generateUniqueKey(prim));
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::updateUniqueKey [generateUniqueKey] prim='%s', "
                    "uniqueKey='%lu', previousUniqueKey='%lu'\n",
                    path.GetText(),
                    key,
                    it->uniqueKey());
            it->setUniqueKey(key);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::preUnloadPrim(UsdPrim& prim, const MObject& primObj)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorContext::preUnloadPrim %s", prim.GetPath().GetText());
    assert(m_proxyShape);
    auto stage = m_proxyShape->getUsdStage();
    if (stage) {
        std::string translatorId = m_proxyShape->context()->getTranslatorIdForPath(prim.GetPath());

        fileio::translators::TranslatorRefPtr translator
            = m_proxyShape->translatorManufacture().getTranslatorFromId(translatorId);
        if (translator) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::preUnloadPrim [preTearDown] prim=%s\n",
                    prim.GetPath().GetText());
            translator->preTearDown(prim);
        } else {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "TranslatorContext::preUnloadPrim [preTearDown] prim=%s\n. Could not find usd "
                    "translator plugin instance for prim!",
                    prim.GetPath().GetText());
            MGlobal::displayError(
                MString("TranslatorContext::preUnloadPrim could not find usd translator plugin "
                        "instance for prim: ")
                + prim.GetPath().GetText() + " type: " + translatorId.c_str());
        }
    } else {
        MGlobal::displayError(
            MString("Could not unload prim: \"") + prim.GetPath().GetText()
            + MString("\", the stage is invalid"));
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::unloadPrim(const SdfPath& path, const MObject& primObj)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::unloadPrim\n");
    assert(m_proxyShape);
    auto stage = m_proxyShape->usdStage();
    if (stage) {
        std::string translatorId = m_proxyShape->context()->getTranslatorIdForPath(path);

        fileio::translators::TranslatorRefPtr translator
            = m_proxyShape->translatorManufacture().getTranslatorFromId(translatorId);
        if (translator) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("TranslatorContext::unloadPrim [tearDown] prim=%s\n", path.GetText());

            UsdPrim prim = stage->GetPrimAtPath(path);
            if (prim) {
                // run through the extra data plugins to apply to this prim
                auto dataPlugins
                    = m_proxyShape->translatorManufacture().getExtraDataPlugins(primObj);
                for (auto dataPlugin : dataPlugins) {
                    dataPlugin->preTearDown(prim);
                }

                translator->preTearDown(prim);
            } else {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "TranslatorContext::preTearDown was skipped because the path '%s' was "
                        "invalid\n",
                        path.GetText());
            }

            MStatus status = translator->tearDown(path);
            switch (status.statusCode()) {
            case MStatus::kSuccess: break;

            case MStatus::kNotImplemented:
                MGlobal::displayError(
                    MString("A variant switch has occurred on a NON-CONFORMING prim, of type: ")
                    + translatorId.c_str() + MString(" located at prim path \"") + path.GetText()
                    + MString("\""));
                break;

            default:
                MGlobal::displayError(
                    MString("A variant switch has caused an error on tear down on prim, of type: ")
                    + translatorId.c_str() + MString(" located at prim path \"") + path.GetText()
                    + MString("\""));
                break;
            }
        } else {
            MGlobal::displayError(
                MString("could not find usd translator plugin instance for prim: ") + path.GetText()
                + " translatorId: " + translatorId.c_str());
        }
    } else {
        MGlobal::displayError(
            MString("Could not unload prim: \"") + path.GetText()
            + MString("\", the stage is invalid"));
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::isNodeAncestorOf(
    MObjectHandle ancestorHandle,
    MObjectHandle objectHandleToTest)
{
    if (!ancestorHandle.isValid() || !ancestorHandle.isAlive()) {
        return false;
    }

    if (!objectHandleToTest.isValid() || !objectHandleToTest.isAlive()) {
        return false;
    }

    MObject ancestorNode = ancestorHandle.object();
    MObject nodeToTest = objectHandleToTest.object();
    if (ancestorNode == nodeToTest) {
        return false;
    }

    MStatus    parentStatus = MS::kSuccess;
    MFnDagNode dagFn(nodeToTest, &parentStatus);

    // If it is not a DAG node:
    if (!parentStatus) {
        return false;
    }

    bool isParent = dagFn.isChildOf(ancestorNode, &parentStatus);
    if (isParent) {
        return true;
    }

    unsigned int parentC = dagFn.parentCount(&parentStatus);
    if (!parentStatus) {
        return false;
    }

    for (unsigned int i = 0; i < parentC; ++i) {
        MObject parentNode = dagFn.parent(i, &parentStatus);
        if (!parentStatus) {
            continue;
        }

        MObjectHandle parentHandle(parentNode);
        if (isNodeAncestorOf(ancestorHandle, parentHandle)) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::isPrimInTransformChain(const SdfPath& path)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("TranslatorContext::isPrimInTransformChain %s\n", path.GetText());

    MDagPath      proxyShapeTransformDagPath = m_proxyShape->parentTransform();
    MObject       proxyTransformNode = proxyShapeTransformDagPath.node();
    MObjectHandle proxyTransformNodeHandle(proxyTransformNode);

    // First test the Maya node that prim is for, this is for MayaReference..
    MObjectHandle parentHandle;
    if (getTransform(path, parentHandle)) {
        if (isNodeAncestorOf(proxyTransformNodeHandle, parentHandle)) {
            return true;
        }
    }

    // Now test the Maya node that translator created, this is for DAG hierarchy transform|shape..
    MObjectHandleArray mayaNodes;
    bool               everCreatedNodes = getMObjects(path, mayaNodes);
    if (!everCreatedNodes) {
        return false;
    }

    size_t nodeCount = mayaNodes.size();
    for (size_t i = 0; i < nodeCount; ++i) {
        MObjectHandle node = mayaNodes[i];
        if (isNodeAncestorOf(proxyTransformNodeHandle, node)) {
            return true;
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::addExcludedGeometry(const SdfPath& newPath)
{
    if (m_proxyShape) {
        auto foundPath = m_excludedGeometry.find(newPath);
        if (foundPath != m_excludedGeometry.end()) {
            return false;
        }

        UsdStageRefPtr stage = getUsdStage();
        SdfPath        pathToAdd = newPath;
        bool           hasInstanceParent = false;
        {
            UsdPrim parentPrim;
            do {
                pathToAdd = pathToAdd.GetParentPath();
                parentPrim = stage->GetPrimAtPath(pathToAdd);
                if (!parentPrim)
                    break;

                if (parentPrim.IsInstance()) {
                    hasInstanceParent = true;
                    break;
                }
            } while (!pathToAdd.IsEmpty());
        }
        if (hasInstanceParent) {
            m_excludedGeometry.emplace(newPath, pathToAdd);
        } else {
            m_excludedGeometry.emplace(newPath, newPath);
        }
        m_isExcludedGeometryDirty = true;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
