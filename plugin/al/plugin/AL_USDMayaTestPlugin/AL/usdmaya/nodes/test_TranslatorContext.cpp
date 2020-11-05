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
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

#include <fstream>

using AL::maya::test::buildTempPath;

// PrimLookup::PrimLookup(const SdfPath& path, const TfToken& type, MObject mayaObj);
// PrimLookup::~PrimLookup();
// const SdfPath& PrimLookup::path() const;
// MObjectHandle PrimLookup::objectHandle() const;
// MObject PrimLookup::object() const;
// TfToken PrimLookup::type() const;
// MObjectHandleArray& PrimLookup::createdNodes();
// const MObjectHandleArray& PrimLookup::createdNodes() const;
TEST(TranslatorContext, PrimLookup)
{
    MFnTransform fnx;
    MObject      obj = fnx.create();
    MObject      obj2 = fnx.create();
    SdfPath      path("/hello/dave");

    AL::usdmaya::fileio::translators::TranslatorContext::PrimLookup nref(
        path, TfToken("transform"), obj);
    nref.createdNodes().push_back(obj2);
    EXPECT_TRUE(obj == nref.object());
    EXPECT_TRUE(path == nref.path());
    EXPECT_TRUE(TfToken("transform") == nref.translatorId());

    AL::usdmaya::fileio::translators::TranslatorContext::PrimLookup cnref(nref);
    EXPECT_TRUE(obj == cnref.object());
    EXPECT_TRUE(path == cnref.path());
    EXPECT_TRUE(TfToken("transform") == cnref.translatorId());
    EXPECT_EQ(1u, cnref.createdNodes().size());
    EXPECT_TRUE(obj2 == cnref.createdNodes()[0].object());
}

// bool TranslatorContext::value_compare::operator() (const PrimLookup& a, const SdfPath& b) const
// bool TranslatorContext::value_compare::operator() (const SdfPath& a, const PrimLookup& b) const
// bool TranslatorContext::value_compare::operator() (const PrimLookup& a, const PrimLookup& b)
// const
TEST(TranslatorContext, value_compare)
{
    SdfPath path1("/hello/dave");
    SdfPath path2("/hello/fred");

    AL::usdmaya::fileio::translators::TranslatorContext::PrimLookup aref(
        path1, TfToken("transform"), MObject());
    AL::usdmaya::fileio::translators::TranslatorContext::PrimLookup bref(
        path2, TfToken("transform"), MObject());
    AL::usdmaya::fileio::translators::TranslatorContext::value_compare compare;

    EXPECT_EQ(compare(aref, path2), path1 < path2);
    EXPECT_EQ(compare(path1, bref), path1 < path2);
    EXPECT_EQ(compare(aref, bref), path1 < path2);
    EXPECT_EQ(compare(path2, aref), path2 < path1);
    EXPECT_EQ(compare(bref, path1), path2 < path1);
    EXPECT_EQ(compare(bref, aref), path2 < path1);
}

// static RefPtr TranslatorContext::create(nodes::ProxyShape* proxyShape);
// const nodes::ProxyShape* TranslatorContext::getProxyShape() const;
// UsdStageRefPtr TranslatorContext::getUsdStage() const;
// bool TranslatorContext::getTransform(const UsdPrim& prim, MObjectHandle& object);
// bool TranslatorContext::getTransform(const SdfPath& path, MObjectHandle& object);
// bool TranslatorContext::getMObject(const UsdPrim& prim, MObjectHandle& object, MTypeId type);
// bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MTypeId type);;
// bool TranslatorContext::getMObject(const UsdPrim& prim, MObjectHandle& object, MFn::Type type);
// bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MFn::Type type);
// bool TranslatorContext::getMObjects(const UsdPrim& prim, MObjectHandleArray& returned);
// bool TranslatorContext::getMObjects(const SdfPath& path, MObjectHandleArray& returned);
// void TranslatorContext::insertItem(const UsdPrim& prim, MObjectHandle object);
// void TranslatorContext::removeItems(const UsdPrim& prim);
// void TranslatorContext::removeItems(const SdfPath& path);
// TfToken TranslatorContext::getTypeForPath(SdfPath path) const
// MString TranslatorContext::serialise() const;
// void TranslatorContext::deserialise(const MString& string);
TEST(TranslatorContext, TranslatorContext)
{
    const MString     temp_ma_path = buildTempPath("AL_USDMayaTests_cube.ma");
    const std::string temp_path = buildTempPath("AL_USDMayaTests_simpleRig.usda");

    const MString g_simpleRig = MString("#usda 1.0\n"
                                        "\n"
                                        "def Xform \"root\"\n"
                                        "{\n"
                                        "    def ALMayaReference \"rig\""
                                        "    {\n"
                                        "      asset mayaReference = \"")
        + temp_ma_path
        + "\"\n"
          "      string mayaNamespace = \"cube\"\n"
          "    }\n"
          "}\n";

    // pCube1, pCubeShape1, polyCube1
    MFileIO::newFile(true);
    MGlobal::executeCommand("polyCube -w 1 -h 1 -d 1 -sd 1 -sh 1 -sw 1", false, false);
    MFileIO::saveAs(temp_ma_path, 0, true);
    MFileIO::newFile(true);

    {
        std::ofstream os(temp_path);
        os << g_simpleRig;
    }

    {
        MString           shapeName;
        MFnDagNode        fn;
        MFnDependencyNode fnd;
        MObject           xform = fn.create("transform");
        MObject           shape = fn.create("AL_usdmaya_ProxyShape", xform);
        shapeName = fn.name();

        AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

        // force the stage to load
        proxy->filePathPlug().setString(temp_path.c_str());

        auto stage = proxy->getUsdStage();

        AL::usdmaya::fileio::translators::TranslatorContextPtr context = proxy->context();
        {
            EXPECT_TRUE(proxy == context->getProxyShape());
            EXPECT_TRUE(stage == context->getUsdStage());
        }
        UsdPrim prim = stage->GetPrimAtPath(SdfPath("/root/rig"));

        MSelectionList sl;
        sl.add("rig");
        MObject       rigObj;
        MObjectHandle transformHandle;
        sl.getDependNode(0, rigObj);
        {
            context->getTransform(SdfPath("/root/rig"), transformHandle);
            EXPECT_TRUE(transformHandle.object() == rigObj);
            transformHandle = MObject::kNullObj;
        }
        {
            context->getTransform(prim, transformHandle);
            EXPECT_TRUE(transformHandle.object() == rigObj);
        }

        std::string translatorId = context->getTranslatorIdForPath(SdfPath("/root/rig"));
        EXPECT_TRUE("schematype:ALMayaReference" == translatorId);

        MObject obj = fnd.create("polyCube");
        {
            context->insertItem(prim, obj);
            {
                MObjectHandle handle;
                context->getMObject(SdfPath("/root/rig"), handle, MFn::kPolyCube);
                EXPECT_TRUE(handle.object() == obj);
            }
            {
                MObjectHandle handle;
                context->getMObject(prim, handle, MFn::kPolyCube);
                EXPECT_TRUE(handle.object() == obj);
            }
            {
                AL::usdmaya::fileio::translators::MObjectHandleArray handles;
                context->getMObjects(SdfPath("/root/rig"), handles);
                ASSERT_EQ(handles.size(), 1u);

                EXPECT_TRUE(handles[0].object() == obj);
            }
            {
                AL::usdmaya::fileio::translators::MObjectHandleArray handles;
                context->getMObjects(prim, handles);
                ASSERT_EQ(handles.size(), 1u);
                EXPECT_TRUE(handles[0].object() == obj);
            }
            context->removeItems(prim);
            {
                AL::usdmaya::fileio::translators::MObjectHandleArray handles;
                context->getMObjects(prim, handles);
                EXPECT_TRUE(handles.empty());
            }
            context->registerItem(prim, transformHandle);
            context->insertItem(prim, obj);
            context->removeItems(SdfPath("/root/rig"));
            {
                AL::usdmaya::fileio::translators::MObjectHandleArray handles;
                context->getMObjects(prim, handles);
                EXPECT_TRUE(handles.empty());
            }
        }

        {
            obj = fnd.create("polyCube");
            context->registerItem(prim, transformHandle);
            context->insertItem(prim, obj);
            MString text = context->serialise();
            context->clearPrimMappings();
            context->deserialise(text);
            {
                AL::usdmaya::fileio::translators::MObjectHandleArray handles;
                context->getMObjects(SdfPath("/root/rig"), handles);
                ASSERT_EQ(handles.size(), 1u);
                EXPECT_TRUE(handles[0].object() == obj);
            }
            translatorId = context->getTranslatorIdForPath(SdfPath("/root/rig"));
            EXPECT_TRUE("schematype:ALMayaReference" == translatorId);
            {
                MObjectHandle handle;
                context->getTransform(SdfPath("/root/rig"), handle);
                EXPECT_TRUE(handle.object() == rigObj);
            }
            context->removeItems(SdfPath("/root/rig"));
        }

        {
            obj = fnd.create("polyCube");
            context->registerItem(prim, transformHandle);
            context->insertItem(prim, obj);
            MString text = context->serialise();
            context->clearPrimMappings();
            context->deserialise(text);

            SdfPathVector itemsToRemove;
            context->preRemoveEntry(SdfPath("/root/rig"), itemsToRemove);
            ASSERT_EQ(itemsToRemove.size(), 1u);
            // preRemoveEntry is often called multiple times before changes are handled.
            context->preRemoveEntry(SdfPath("/root"), itemsToRemove);
            // Make sure prims have not been added twice
            ASSERT_EQ(itemsToRemove.size(), 1u);

            context->removeEntries(itemsToRemove);

            // context's prim mapping should be empty and it should not find type or transform
            translatorId = context->getTranslatorIdForPath(SdfPath("/root/rig"));
            EXPECT_TRUE(translatorId.empty());
            MObjectHandle handle;
            EXPECT_FALSE(context->getTransform(SdfPath("/root/rig"), handle));
        }
    }
}

// TranslatorContext::~TranslatorContext();
// void TranslatorContext::updatePrimTypes();
// void TranslatorContext::registerItem(const UsdPrim& prim, MObjectHandle object);
// void TranslatorContext::validatePrims();
// bool TranslatorContext::hasEntry(const SdfPath& path, const TfToken& type);
// void TranslatorContext::addEntry(const SdfPath& primPath, const MObject& primObj);
// void TranslatorContext::preRemoveEntry(const SdfPath& primPath, SdfPathVector& itemsToRemove,
// bool callPreUnload=true); void TranslatorContext::removeEntries(const SdfPathVector&
// itemsToRemove);
TEST(SchemaNodeRefDB, addRemoveEntries) { AL_USDMAYA_UNTESTED; }
