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
#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"

#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include <maya/MFnDagNode.h>

namespace AL {
namespace usdmaya {
namespace cmds {
namespace {

struct CompareLayerHandle
{
    bool operator()(const SdfLayerHandle& a, const SdfLayerHandle& b) const
    {
        return a->GetDisplayName() < b->GetDisplayName();
    }
};

typedef std::set<SdfLayerHandle, CompareLayerHandle>           LayerSet;
typedef std::map<SdfLayerHandle, LayerSet, CompareLayerHandle> LayerMap;
typedef std::map<SdfLayerHandle, MObject, CompareLayerHandle>  LayerToObjectMap;

//----------------------------------------------------------------------------------------------------------------------
struct ImportCallback
{
    enum ScriptType : uint32_t
    {
        kMel,
        kPython
    };

    void setCallbackType(TfToken scriptType)
    {
        if (scriptType == "mel") {
            type = kMel;
        } else if (scriptType == "py") {
            type = kPython;
        }
    }
    std::string  name;
    VtDictionary params;
    ScriptType   type;
};

//----------------------------------------------------------------------------------------------------------------------
void huntForNativeNodes(
    const MDagPath&              proxyTransformPath,
    std::vector<UsdPrim>&        schemaPrims,
    std::vector<ImportCallback>& postCallBacks,
    nodes::ProxyShape&           proxy)
{

    UsdStageRefPtr                              stage = proxy.getUsdStage();
    fileio::translators::TranslatorManufacture& manufacture = proxy.translatorManufacture();

    fileio::SchemaPrimsUtils utils(manufacture);
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("huntForNativeNodes::huntForNativeNodes\n");
    fileio::TransformIterator it(stage, proxyTransformPath);
    for (; !it.done(); it.next()) {
        UsdPrim prim = it.prim();
        TF_DEBUG(ALUSDMAYA_COMMANDS)
            .Msg("huntForNativeNodes: PrimName %s\n", prim.GetName().GetText());

        // If the prim isn't importable by default then don't add it to the list
        fileio::translators::TranslatorRefPtr t = utils.isSchemaPrim(prim);
        if (t && t->importableByDefault()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "ProxyShapePostLoadProcess::huntForNativeNodes found matching schema %s\n",
                    prim.GetPath().GetText());
            schemaPrims.push_back(prim);
        }

        VtDictionary                 customData = prim.GetCustomData();
        VtDictionary::const_iterator postCallBacksEntry = customData.find("callbacks");
        if (postCallBacksEntry != customData.end()) {
            // Get the list of post callbacks
            VtDictionary melCallbacks = postCallBacksEntry->second.Get<VtDictionary>();

            for (VtDictionary::const_iterator melCommand = melCallbacks.begin(),
                                              end = melCallbacks.end();
                 melCommand != end;
                 ++melCommand) {
                ImportCallback importCallback;
                importCallback.name = melCommand->first;
                importCallback.type = ImportCallback::kMel;
                importCallback.params = melCommand->second.Get<VtDictionary>();

                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShapePostLoadProcess::huntForNativeNodes adding post callback from "
                        "%s\n",
                        prim.GetPath().GetText());
                postCallBacks.push_back(importCallback);
            }
        }
    }
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
static bool parentNodeIsUnmerged(const UsdPrim& prim)
{
    bool    parentUnmerged = false;
    TfToken val;
    if (prim.GetParent().IsValid()
        && prim.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val)) {
        parentUnmerged = (val == AL::usdmaya::Metadata::unmerged);
    }
    return parentUnmerged;
}

//----------------------------------------------------------------------------------------------------------------------
fileio::ImporterParams ProxyShapePostLoadProcess::m_params;

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(
    nodes::ProxyShape*                        ptrNode,
    const std::vector<UsdPrim>&               schemaPrims,
    const MDagPath&                           proxyTransformPath,
    ProxyShapePostLoadProcess::MObjectToPrim& objsToCreate,
    bool                                      pushToPrim,
    bool                                      readAnimatedValues)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims\n");
    {
        objsToCreate.reserve(schemaPrims.size());
        MDagModifier modifier;
        MDGModifier  modifier2;

        MPlug                    outStage = ptrNode->outStageDataPlug();
        MPlug                    outTime = ptrNode->outTimePlug();
        MFnTransform             fnx(proxyTransformPath);
        fileio::SchemaPrimsUtils schemaPrimUtils(ptrNode->translatorManufacture());
        for (auto it = schemaPrims.begin(); it != schemaPrims.end(); ++it) {
            const UsdPrim& usdPrim = *it;
            if (usdPrim.IsValid()) {
                SdfPath path = usdPrim.GetPath();
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims checking "
                        "%s\n",
                        path.GetText());
                MObject newpath = MObject::kNullObj;
                bool    parentUnmerged = parentNodeIsUnmerged(usdPrim);

                if (schemaPrimUtils.needsTransformParent(usdPrim)) {
                    if (!parentUnmerged) {
                        newpath = ptrNode->makeUsdTransformChain(
                            usdPrim,
                            modifier,
                            nodes::ProxyShape::kRequired,
                            &modifier2,
                            0,
                            pushToPrim,
                            readAnimatedValues);
                    } else {
                        newpath = ptrNode->makeUsdTransformChain(
                            usdPrim.GetParent(),
                            modifier,
                            nodes::ProxyShape::kRequired,
                            &modifier2,
                            0,
                            pushToPrim,
                            readAnimatedValues);
                    }
                }
                objsToCreate.emplace_back(newpath, usdPrim);
            } else {
                std::cout << "prim is invalid" << std::endl;
            }
        }

        if (!modifier.doIt()) {
            std::cerr << "Failed to connect up attributes" << std::endl;
        } else if (!modifier2.doIt()) {
            std::cerr << "Failed to enable pushToPrim attributes" << std::endl;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::createSchemaPrims(
    nodes::ProxyShape*                               proxy,
    const std::vector<UsdPrim>&                      objsToCreate,
    const fileio::translators::TranslatorParameters& param)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::createSchemaPrims\n");
    {
        fileio::translators::TranslatorContextPtr   context = proxy->context();
        fileio::translators::TranslatorManufacture& translatorManufacture
            = proxy->translatorManufacture();

        if (context->getForceDefaultRead()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("ProxyShapePostLoadProcess::createSchemaPrims,"
                     " will read default values\n");
        }

        auto       it = objsToCreate.begin();
        const auto end = objsToCreate.end();
        for (; it != end; ++it) {
            UsdPrim prim = *it;
            bool    parentUnmerged = parentNodeIsUnmerged(prim);
            MObject object;
            if (parentUnmerged) {
                object = proxy->findRequiredPath(prim.GetParent().GetPath());
            } else {
                object = proxy->findRequiredPath(prim.GetPath());
            }

            fileio::translators::TranslatorRefPtr translator = translatorManufacture.get(prim);

            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "ProxyShapePostLoadProcess::createSchemaPrims prim=%s\n",
                    prim.GetPath().GetText());

            // if(!context->hasEntry(prim.GetPath(), prim.GetTypeName()))
            {
                MObject created;
                if (!fileio::importSchemaPrim(prim, object, created, context, translator, param)) {
                    std::cerr << "Error: unable to load schema prim node: '"
                              << prim.GetName().GetString() << "' that has type: '"
                              << prim.GetTypeName() << "'" << std::endl;
                }

                auto dataPlugins = translatorManufacture.getExtraDataPlugins(created);
                for (auto dataPlugin : dataPlugins) {
                    dataPlugin->import(prim, created);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::updateSchemaPrims(
    nodes::ProxyShape*          proxy,
    const std::vector<UsdPrim>& objsToCreate)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::updateSchemaPrims\n");
    {
        fileio::translators::TranslatorContextPtr   context = proxy->context();
        fileio::translators::TranslatorManufacture& translatorManufacture
            = proxy->translatorManufacture();

        auto       it = objsToCreate.begin();
        const auto end = objsToCreate.end();
        for (; it != end; ++it) {
            UsdPrim prim = *it;

            fileio::translators::TranslatorRefPtr translator = translatorManufacture.get(prim);
            std::string translatorId = translatorManufacture.generateTranslatorId(prim);
            bool        hasMatchingEntry = context->hasEntry(prim.GetPath(), translatorId);
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "ProxyShapePostLoadProcess::updateSchemaPrims: hasEntry(%s, %s)=%d\n",
                    prim.GetPath().GetText(),
                    translatorId.c_str(),
                    hasMatchingEntry);

            if (!hasMatchingEntry) {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShapePostLoadProcess::updateSchemaPrims prim=%s hasEntry=false\n",
                        prim.GetPath().GetText());
                MObject created;
                MObject object = proxy->findRequiredPath(prim.GetPath());
                fileio::importSchemaPrim(prim, object, created, context, translator);
            } else {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShapePostLoadProcess::updateSchemaPrims [update] prim=%s\n",
                        prim.GetPath().GetText());
                if (translator) {
                    if (!translator->supportsUpdate()) {
                        MString err = "Update requested on prim that does not support update: ";
                        err += prim.GetPath().GetText();
                        MGlobal::displayWarning(err);
                    } else {
                        if (translator->update(prim).statusCode() == MStatus::kNotImplemented) {
                            MGlobal::displayError(
                                MString("Prim type has claimed that it supports update, but it "
                                        "does not! ")
                                + prim.GetPath().GetText());
                        } else {
                            std::vector<MObjectHandle> returned;
                            if (context->getMObjects(prim, returned) && !returned.empty()) {
                                auto dataPlugins = translatorManufacture.getExtraDataPlugins(
                                    returned[0].object());
                                for (auto dataPlugin : dataPlugins) {
                                    dataPlugin->update(prim);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapePostLoadProcess::connectSchemaPrims(
    nodes::ProxyShape*          proxy,
    const std::vector<UsdPrim>& objsToCreate)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::connectSchemaPrims\n");

    fileio::translators::TranslatorContextPtr   context = proxy->context();
    fileio::translators::TranslatorManufacture& translatorManufacture
        = proxy->translatorManufacture();

    // iterate over the prims we created, and call any post-import logic to make any attribute
    // connections etc
    auto       it = objsToCreate.begin();
    const auto end = objsToCreate.end();
    for (; it != end; ++it) {
        UsdPrim                               prim = *it;
        fileio::translators::TranslatorRefPtr torBase = translatorManufacture.get(prim);
        if (torBase) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "ProxyShapePostLoadProcess::connectSchemaPrims [postImport] prim=%s\n",
                    prim.GetPath().GetText());
            torBase->postImport(prim);
            std::vector<MObjectHandle> returned;
            if (context->getMObjects(prim, returned) && !returned.empty()) {
                auto dataPlugins = translatorManufacture.getExtraDataPlugins(returned[0].object());
                for (auto dataPlugin : dataPlugins) {
                    dataPlugin->postImport(prim);
                }
            }

            context->updateUniqueKey(prim);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShapePostLoadProcess::initialise(nodes::ProxyShape* ptrNode)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShapePostLoadProcess::initialise called\n");

    if (!ptrNode) {
        std::cerr << "ProxyShapePostLoadProcess::initialise Unable to initialise the "
                     "PostLoadProcess since the ProxyShape ptr is null"
                  << std::endl;
        return MStatus::kFailure;
    }

    MFnDagNode fn(ptrNode->thisMObject());
    MDagPath   proxyTransformPath;
    fn.getPath(proxyTransformPath);

    // make sure we unload all references prior to reloading them again
    ptrNode->unloadMayaReferences();
    ptrNode->destroyTransformReferences();
    fileio::translators::TranslatorManufacture::preparePythonTranslators(ptrNode->context());

    // Now go and delete any child Transforms found directly underneath the shapes parent.
    // These nodes are likely to be driven by the output stage data of the shape.
    {
        MDagModifier modifier;
        MFnDagNode   fnDag(fn.parent(0));
        for (uint32_t i = 0; i < fnDag.childCount(); ++i) {
            MObject obj = fnDag.child(i);
            if (obj.hasFn(MFn::kPluginTransformNode)) {
                MFnDagNode fnChild(obj);
                if (fnChild.typeId() == nodes::Transform::kTypeId
                    || fnChild.typeId() == nodes::Scope::kTypeId) {
                    modifier.deleteNode(obj);
                }
            }
        }

        if (!modifier.doIt()) { }
    }

    proxyTransformPath.pop();

    // iterate over the stage and find all custom schema nodes that have registered translator
    // plugins
    std::vector<UsdPrim>        schemaPrims;
    std::vector<ImportCallback> callBacks;
    UsdStageRefPtr              stage = ptrNode->usdStage();
    if (stage) {
        huntForNativeNodes(
            proxyTransformPath,
            schemaPrims,
            callBacks,
            *ptrNode /* stage, ptrNode->translatorManufacture()*/);
    } else {
        return MS::kFailure;
    }

    // generate the transform chains
    MObjectToPrim objsToCreate;
    createTranformChainsForSchemaPrims(ptrNode, schemaPrims, proxyTransformPath, objsToCreate);

    // create prims that need to be imported
    createSchemaPrims(ptrNode, schemaPrims);

    // now perform any post-creation fix up
    connectSchemaPrims(ptrNode, schemaPrims);

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
