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
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDGModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

using AL::maya::test::buildTempPath;

// Utilities
// -----------------------------------------------------------------------------------------------------------

MObject createLayerManager()
{
    MDGModifier dgMod;
    MStatus     status;
    MObject     result = dgMod.createNode(AL::usdmaya::nodes::LayerManager::kTypeId, &status);
    if (!status)
        return MObject();
    if (!dgMod.doIt())
        return MObject();
    return result;
}

void getLayerManagers(MObjectArray& result)
{
    result.clear();
    MFnDependencyNode  fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for (; !iter.isDone(); iter.next()) {
        MObject mobj = iter.item();
        EXPECT_EQ(fn.setObject(mobj), MStatus::kSuccess);
        if (fn.typeId() == AL::usdmaya::nodes::LayerManager::kTypeId) {
            EXPECT_EQ(result.append(mobj), MStatus::kSuccess);
        }
    }
}

void deleteLayerManager(MObject& mobj)
{
    MDGModifier dgMod;
    EXPECT_EQ(dgMod.deleteNode(mobj), MStatus::kSuccess);
    EXPECT_EQ(dgMod.doIt(), MStatus::kSuccess);
}

// Tests
// ---------------------------------------------------------------------------------------------------------------

//#define TEST(X, Y) void X##Y()

//  static void* conditionalCreator();
TEST(LayerManager, conditionalCreator)
{
    MFileIO::newFile(true);

    MObjectArray managers;

    // Before we start, should no LayerManagers
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 0u);

    // After we make one, should be one
    createLayerManager();
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);

    // Trying to make another should fail
    createLayerManager();
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);

    // Delete the layer manager
    deleteLayerManager(managers[0]);
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 0u);

    // Should be able to make another one again
    createLayerManager();
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);

    // Trying to make another should fail
    createLayerManager();
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);
}

//  static MObject findNode();
//  static MObject findOrCreateNode();
//  static MObject findManager();
//  static MObject findOrCreateManager();
TEST(LayerManager, findNode)
{
    MFileIO::newFile(true);

    MObjectArray managers;
    MObject      result;

    // Before we start, should no LayerManagers
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 0u);
    result = AL::usdmaya::nodes::LayerManager::findNode();
    EXPECT_TRUE(result.isNull());
    EXPECT_FALSE(AL::usdmaya::nodes::LayerManager::findManager());

    // Make a layer manager
    MObject manager = AL::usdmaya::nodes::LayerManager::findOrCreateNode();
    EXPECT_FALSE(manager.isNull());
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);
    EXPECT_EQ(managers[0], manager);
    result = AL::usdmaya::nodes::LayerManager::findNode();
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result, manager);
    result = AL::usdmaya::nodes::LayerManager::findOrCreateNode();
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result, manager);
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);

    auto managerPtr
        = static_cast<AL::usdmaya::nodes::LayerManager*>(MFnDependencyNode(manager).userNode());
    ASSERT_TRUE(managerPtr);
    ASSERT_EQ(AL::usdmaya::nodes::LayerManager::findManager(), managerPtr);
    ASSERT_EQ(AL::usdmaya::nodes::LayerManager::findOrCreateManager(), managerPtr);

    // Trying to make another should fail
    createLayerManager();
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);
    EXPECT_EQ(managers[0], manager);

    // Delete the layer manager
    deleteLayerManager(managers[0]);
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 0u);
    result = AL::usdmaya::nodes::LayerManager::findNode();
    EXPECT_TRUE(result.isNull());
    EXPECT_FALSE(AL::usdmaya::nodes::LayerManager::findManager());

    // Should be able to make another one again
    manager = AL::usdmaya::nodes::LayerManager::findOrCreateNode();
    EXPECT_FALSE(manager.isNull());
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);
    EXPECT_EQ(managers[0], manager);
    result = AL::usdmaya::nodes::LayerManager::findNode();
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result, manager);
    result = AL::usdmaya::nodes::LayerManager::findOrCreateNode();
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result, manager);
    getLayerManagers(managers);
    EXPECT_EQ(managers.length(), 1u);

    managerPtr
        = static_cast<AL::usdmaya::nodes::LayerManager*>(MFnDependencyNode(manager).userNode());
    ASSERT_TRUE(managerPtr);
    ASSERT_EQ(AL::usdmaya::nodes::LayerManager::findManager(), managerPtr);
    ASSERT_EQ(AL::usdmaya::nodes::LayerManager::findOrCreateManager(), managerPtr);
}

// std::pair<size_t, bool> addLayer(SdfLayerRefPtr layer);
// bool removeLayer(SdfLayerRefPtr layer);
// SdfLayerHandle findLayer(size_t index);
// SdfLayerHandle findLayer(std::string identifier);
// void getLayerIdentifiers(MStringArray& outputNames);
TEST(LayerManager, addRemoveLayer)
{
    MFileIO::newFile(true);

    auto* manager = AL::usdmaya::nodes::LayerManager::findOrCreateManager();
    ASSERT_TRUE(manager);

    auto anonLayer = SdfLayer::CreateAnonymous("myAnonLayer");
    auto realLayer = SdfLayer::New(
        SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id), "/my/silly/layer.usda");
    MStringArray layerIds;

    ASSERT_FALSE(manager->findLayer(anonLayer->GetIdentifier()));
    ASSERT_FALSE(manager->findLayer(realLayer->GetIdentifier()));
    manager->getLayerIdentifiers(layerIds);
    ASSERT_EQ(layerIds.length(), 0u);

    // try adding an anonymous layer
    {
        ASSERT_TRUE(manager->addLayer(anonLayer));
        ASSERT_FALSE(manager->addLayer(anonLayer));
        // Dirty the anonymous layer so it can be found
        anonLayer->SetComment("DirtyThisAnonLayer");
        ASSERT_EQ(manager->findLayer(anonLayer->GetIdentifier()), anonLayer);
        ASSERT_FALSE(manager->findLayer(realLayer->GetIdentifier()));
        manager->getLayerIdentifiers(layerIds);
        ASSERT_EQ(layerIds.length(), 1u);
        ASSERT_EQ(MString(anonLayer->GetIdentifier().c_str()), layerIds[0]);
    }

    // try adding a "real" layer
    {
        ASSERT_TRUE(manager->addLayer(realLayer));
        ASSERT_FALSE(manager->addLayer(realLayer));
        realLayer->SetComment("DirtyThisAnonLayer");
        ASSERT_EQ(manager->findLayer(anonLayer->GetIdentifier()), anonLayer);
        ASSERT_EQ(manager->findLayer(realLayer->GetIdentifier()), realLayer);
        manager->getLayerIdentifiers(layerIds);
        ASSERT_EQ(layerIds.length(), 2u);
        // since there's ony two items, and they may be returned in arbitrary
        // order, just check both orderings
        if (MString(anonLayer->GetIdentifier().c_str()) == layerIds[0]) {
            ASSERT_EQ(MString(realLayer->GetIdentifier().c_str()), layerIds[1]);
        } else {
            ASSERT_EQ(MString(realLayer->GetIdentifier().c_str()), layerIds[0]);
            ASSERT_EQ(MString(anonLayer->GetIdentifier().c_str()), layerIds[1]);
        }
    }

    // try removing an anonymous layer
    {
        ASSERT_TRUE(manager->removeLayer(anonLayer));
        ASSERT_FALSE(manager->removeLayer(anonLayer));

        ASSERT_FALSE(manager->findLayer(anonLayer->GetIdentifier()));
        ASSERT_EQ(manager->findLayer(realLayer->GetIdentifier()), realLayer);
        manager->getLayerIdentifiers(layerIds);
        ASSERT_EQ(layerIds.length(), 1u);
        ASSERT_EQ(MString(realLayer->GetIdentifier().c_str()), layerIds[0]);
    }

    // try removing a "real" layer
    {
        ASSERT_TRUE(manager->removeLayer(realLayer));
        ASSERT_FALSE(manager->removeLayer(realLayer));

        ASSERT_FALSE(manager->findLayer(anonLayer->GetIdentifier()));
        ASSERT_FALSE(manager->findLayer(realLayer->GetIdentifier()));
        manager->getLayerIdentifiers(layerIds);
        ASSERT_EQ(layerIds.length(), 0u);
    }
}

// MStatus populateSerialisationAttributes();
// clearSerialisationAttributes();
// disconnectCompoundArrayPlug(MPlug arrayPlug)
TEST(LayerManager, populateClearSerializationAttributes)
{
    constexpr auto LAYER_CONTENTS = R"ESC(#usda 1.0

def Scope "blabla"
{
    def Xform "wassup"
    {
    }
}

)ESC";

    MStatus status;

    MFileIO::newFile(true);

    // Make a manager, and add a layer to be managed by it
    auto* manager = AL::usdmaya::nodes::LayerManager::findOrCreateManager();
    ASSERT_TRUE(manager);

    ASSERT_EQ(0u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(0u, manager->layersPlug().evaluateNumElements());

    auto realLayer = SdfLayer::New(
        SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id), "/my/silly/layer.usda");
    realLayer->ImportFromString(LAYER_CONTENTS);

    ASSERT_TRUE(manager->addLayer(realLayer));

    // Make a dummy message attribute on the persp node that we can use
    // to make connections to
    MObject perspNode;
    MObject destMsgAttr1;
    MObject destMsgAttr2;
    MPlug   destMsgPlug1;
    MPlug   destMsgPlug2;
    {
        MSelectionList sel;
        sel.add("persp");
        ASSERT_TRUE(sel.getDependNode(0, perspNode));

        MFnMessageAttribute mfnMsgAttr;
        destMsgAttr1 = mfnMsgAttr.create("myMessageAttr1", "myMsgAttr1", &status);
        ASSERT_TRUE(status);
        destMsgAttr2 = mfnMsgAttr.create("myMessageAttr2", "myMsgAttr2", &status);
        ASSERT_TRUE(status);

        MDGModifier dgMod;
        ASSERT_TRUE(dgMod.addAttribute(perspNode, destMsgAttr1));
        ASSERT_TRUE(dgMod.addAttribute(perspNode, destMsgAttr2));
        ASSERT_TRUE(dgMod.doIt());

        MFnDependencyNode perspMFn(perspNode, &status);
        ASSERT_TRUE(status);
        destMsgPlug1 = perspMFn.findPlug(destMsgAttr1, false, &status);
        ASSERT_TRUE(status);
        destMsgPlug2 = perspMFn.findPlug(destMsgAttr2, false, &status);
        ASSERT_TRUE(status);
    }

    auto makeConnections = [&]() {
        MDGModifier dgMod;
        MPlug       layerPlug0 = manager->layersPlug().elementByLogicalIndex(0, &status);
        ASSERT_TRUE(status);
        MPlug layerPlug1 = manager->layersPlug().elementByLogicalIndex(1, &status);
        ASSERT_TRUE(status);
        ASSERT_TRUE(dgMod.connect(layerPlug0, destMsgPlug1));
        ASSERT_TRUE(dgMod.connect(layerPlug1, destMsgPlug2));
        ASSERT_TRUE(dgMod.doIt());
    };

    auto assertLayersPopulated = [&]() {
        ASSERT_EQ(0u, manager->layersPlug().numConnectedElements());
        ASSERT_EQ(1u, manager->layersPlug().evaluateNumElements());
        MPlug layersPlug0 = manager->layersPlug().elementByPhysicalIndex(0, &status);
        ASSERT_TRUE(status);
        ASSERT_EQ(0u, layersPlug0.logicalIndex(&status));
        ASSERT_TRUE(status);
        // lame that MPlug.child(MObject&) won't accept const MObject&... so make a temp non-const
        // one
        MObject tempNonConst;
        tempNonConst = manager->identifier();
        MPlug idPlug = layersPlug0.child(tempNonConst, &status);
        ASSERT_TRUE(status);
        tempNonConst = manager->fileFormatId();
        MPlug fileFormatIdPlug = layersPlug0.child(tempNonConst, &status);
        ASSERT_TRUE(status);
        tempNonConst = manager->serialized();
        MPlug serializedPlug = layersPlug0.child(tempNonConst, &status);
        ASSERT_TRUE(status);
        tempNonConst = manager->anonymous();
        MPlug anonymousPlug = layersPlug0.child(tempNonConst, &status);
        ASSERT_TRUE(status);

        ASSERT_EQ(
            MString(realLayer->GetIdentifier().c_str()),
            idPlug.asString(MDGContext::fsNormal, &status));
        ASSERT_TRUE(status);
        ASSERT_EQ(
            MString(realLayer->GetFileFormat()->GetFormatId().GetText()),
            fileFormatIdPlug.asString(MDGContext::fsNormal, &status));
        ASSERT_TRUE(status);
        ASSERT_EQ(MString(LAYER_CONTENTS), serializedPlug.asString(MDGContext::fsNormal, &status));
        ASSERT_TRUE(status);
        ASSERT_FALSE(anonymousPlug.asBool(MDGContext::fsNormal, &status));
        ASSERT_TRUE(status);
    };

    // Now try making dummy connections to the layers attribute
    ASSERT_EQ(0u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(0u, manager->layersPlug().evaluateNumElements());
    {
        SCOPED_TRACE("");
        makeConnections();
    }
    ASSERT_EQ(2u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(2u, manager->layersPlug().evaluateNumElements());

    // Then make sure clearSerialisationAttributes wipes them out
    manager->clearSerialisationAttributes();
    ASSERT_EQ(0u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(0u, manager->layersPlug().evaluateNumElements());

    // Now, populate, we should have one layer plug
    manager->populateSerialisationAttributes();
    {
        SCOPED_TRACE("");
        assertLayersPopulated();
    }

    // Try clearing, then making connections, then re-populating
    manager->clearSerialisationAttributes();
    ASSERT_EQ(0u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(0u, manager->layersPlug().evaluateNumElements());
    {
        SCOPED_TRACE("");
        makeConnections();
    }
    ASSERT_EQ(2u, manager->layersPlug().numConnectedElements());
    ASSERT_EQ(2u, manager->layersPlug().evaluateNumElements());
    manager->populateSerialisationAttributes();
    {
        SCOPED_TRACE("");
        assertLayersPopulated();
    }
}

TEST(LayerManager, simpleSaveRestore)
{
    MFileIO::newFile(true);

    const SdfPath     rootPath("/root");                               // ie, /root
    const SdfPath     hipPath = rootPath.AppendChild(TfToken("hip1")); // ie, /root/hip1
    const TfToken     fooToken("foo");
    const SdfPath     fooPath = hipPath.AppendProperty(fooToken); // ie, /root/hip1.foo
    const float       fooValue = 5.86;
    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_LayerManager_simpleSaveRestore.usda");
    const MString temp_ma_path = buildTempPath("AL_USDMayaTests_LayerManager_simpleSaveRestore.ma");

    MString shapeName;

    auto constructTransformChain = [&]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, rootPath);
        UsdGeomXform   leg1 = UsdGeomXform::Define(stage, hipPath);
        return stage;
    };

    auto newFileAndClearCache = [&]() {
        // nuke everything
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::newFile(true));

        // inspect the sdf layer cache to make sure that has been cleared!
        {
            UsdStageCache& layerCache = AL::usdmaya::StageCache::Get();
            EXPECT_EQ(size_t(0), layerCache.Size());
            auto layer = SdfLayer::Find(temp_path);
            EXPECT_FALSE(layer);
        }
        {
            UsdStageCache& layerCache = AL::usdmaya::StageCache::Get();
            EXPECT_EQ(size_t(0), layerCache.Size());
        }
    };

    auto confirmLayerEditsPresent = [&]() {
        // there SHOULD be a layer manager...
        MObject layerManagerNode = AL::usdmaya::nodes::LayerManager::findNode();
        ASSERT_FALSE(layerManagerNode.isNull());
        MStringArray result;
        MGlobal::executeCommand(
            MString("ls -type ") + AL::usdmaya::nodes::LayerManager::kTypeName, result);
        EXPECT_EQ(result.length(), 1u);

        // ...however, it's layers attribute should be empty (only used during serialization /
        // deserialization!)
        MFnDependencyNode mfnLayerMan(layerManagerNode);
        MPlug             layerPlug = mfnLayerMan.findPlug("layers");
        ASSERT_FALSE(layerPlug.isNull());
        ASSERT_TRUE(layerPlug.isArray());
        EXPECT_EQ(layerPlug.evaluateNumElements(), 0u);

        // Make sure that we still have the edits we made...
        MSelectionList list;
        list.add(shapeName);
        MObject shapeObj;
        ASSERT_EQ(list.length(), 1u);
        list.getDependNode(0, shapeObj);
        ASSERT_FALSE(shapeObj.isNull());
        MFnDagNode fn(shapeObj);
        ASSERT_EQ(fn.typeId(), AL::usdmaya::nodes::ProxyShape::kTypeId);

        AL::usdmaya::nodes::ProxyShape* proxy
            = static_cast<AL::usdmaya::nodes::ProxyShape*>(fn.userNode());

        auto stage = proxy->getUsdStage();
        auto hip = stage->GetPrimAtPath(hipPath);
        auto session = stage->GetSessionLayer();

        ASSERT_TRUE(hip.HasAttribute(fooToken));
        float outValue;
        hip.GetAttribute(fooToken).Get(&outValue);
        EXPECT_EQ(outValue, fooValue);
        auto fooLayerAttr = session->GetAttributeAtPath(fooPath);
        ASSERT_TRUE(fooLayerAttr);
        EXPECT_EQ(fooLayerAttr->GetDefaultValue(), fooValue);
    };

    // generate some data for the proxy shape
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    {
        // Verify that, in the layer, /root/hip1.foo attribute is not present
        auto layer = SdfLayer::FindOrOpen(temp_path);
        EXPECT_FALSE(layer->GetAttributeAtPath(fooPath));
    }

    {
        SCOPED_TRACE("Pre Save");

        MFnDagNode fn;
        MObject    xform = fn.create("transform");
        MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.fullPathName();

        AL::usdmaya::nodes::ProxyShape* proxy
            = static_cast<AL::usdmaya::nodes::ProxyShape*>(fn.userNode());

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();
        auto hip = stage->GetPrimAtPath(hipPath);
        auto session = stage->GetSessionLayer();

        // Verify that initially, in the stage, /root/hip1.foo attribute is not present
        EXPECT_FALSE(hip.HasAttribute(fooToken));
        EXPECT_FALSE(session->GetAttributeAtPath(fooPath));

        // Now add the foo attribute
        EXPECT_EQ(stage->GetEditTarget().GetLayer(), session);
        auto fooStageAttr = hip.CreateAttribute(fooToken, SdfValueTypeNames->Float);
        // ...and set it...
        fooStageAttr.Set(fooValue);

        // Then check that both stage / layer have the attr set to the right value
        ASSERT_TRUE(hip.HasAttribute(fooToken));
        float outValue;
        hip.GetAttribute(fooToken).Get(&outValue);
        EXPECT_EQ(outValue, fooValue);
        auto fooLayerAttr = session->GetAttributeAtPath(fooPath);
        ASSERT_TRUE(fooLayerAttr);
        EXPECT_EQ(fooLayerAttr->GetDefaultValue(), fooValue);
    }

    {
        SCOPED_TRACE("Post Save");
        // Save the scene
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::saveAs(temp_ma_path));
        confirmLayerEditsPresent();
    }

    {
        SCOPED_TRACE("File Open");
        // Now re-open the file, and re-check everything to make sure it restored correctly
        newFileAndClearCache();
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::open(temp_ma_path, NULL, true));
        confirmLayerEditsPresent();
    }

    {
        SCOPED_TRACE("File Import");
        // Make sure everything works as expected when we import, instead of open
        newFileAndClearCache();
        EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::importFile(temp_ma_path));
        confirmLayerEditsPresent();
    }

    // TODO: fix file references
    // (need to figure out how to deal with conflicting edits to to the same layer)
    //  {
    //    SCOPED_TRACE("File Reference");
    //    // Make sure everything works with a reference
    //    newFileAndClearCache();
    //    EXPECT_EQ(MStatus(MS::kSuccess), MFileIO::reference(temp_ma_path));
    //    confirmLayerEditsPresent();
    //  }
}
