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
#include "test_usdmaya.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/StageCache.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MDagModifier.h"
#include "maya/MFileIO.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

//#define TEST(X, Y) void X##Y()

//  SchemaNodeRef::SchemaNodeRef(const SdfPath& path, MObject mayaObj)
//  SchemaNodeRef::~SchemaNodeRef()
//  const SdfPath& SchemaNodeRef::primPath() const
//  MObject SchemaNodeRef::mayaObject() const
TEST(SchemaNodeRef, SchemaNodeRef)
{
  MFnTransform fnx;
  MObject obj = fnx.create();
  SdfPath path("/hello/dave");

  AL::usdmaya::nodes::SchemaNodeRef nref(path, obj);
  EXPECT_TRUE(obj == nref.mayaObject());
  EXPECT_TRUE(path == nref.primPath());

  AL::usdmaya::nodes::SchemaNodeRef cnref(nref);
  EXPECT_TRUE(obj == cnref.mayaObject());
  EXPECT_TRUE(path == cnref.primPath());
}

// bool SchemaNodeRefDB::value_compare::operator() (const SchemaNodeRef& a, const SdfPath& b) const
// bool SchemaNodeRefDB::value_compare::operator() (const SdfPath& a, const SchemaNodeRef& b) const
// bool SchemaNodeRefDB::value_compare::operator() (const SchemaNodeRef& a, const SchemaNodeRef& b) const
TEST(SchemaNodeRefDB, value_compare)
{
  SdfPath path1("/hello/dave");
  SdfPath path2("/hello/fred");

  AL::usdmaya::nodes::SchemaNodeRef aref(path1, MObject());
  AL::usdmaya::nodes::SchemaNodeRef bref(path2, MObject());
  AL::usdmaya::nodes::SchemaNodeRefDB::value_compare compare;

  EXPECT_EQ(compare(aref, path2), path1 < path2);
  EXPECT_EQ(compare(path1, bref), path1 < path2);
  EXPECT_EQ(compare(aref, bref), path1 < path2);
  EXPECT_EQ(compare(path2, aref), path2 < path1);
  EXPECT_EQ(compare(bref, path1), path2 < path1);
  EXPECT_EQ(compare(bref, aref), path2 < path1);
}

//  SchemaNodeRefDB::SchemaNodeRefDB(nodes::ProxyShape* const proxy);
//  SchemaNodeRefDB::~SchemaNodeRefDB();
//  void SchemaNodeRefDB::lock()
//  bool SchemaNodeRefDB::hasEntry(const SdfPath& path, const TfToken& type)
//  void SchemaNodeRefDB::addEntry(const SdfPath& primPath, const MObject& primObj);
//  void SchemaNodeRefDB::unlock()
//  void SchemaNodeRefDB::preRemoveEntry(const SdfPath& primPath, SdfPathVector& itemsToRemove);
//  void SchemaNodeRefDB::removeEntries(const SdfPathVector& itemsToRemove);
TEST(SchemaNodeRefDB, addRemoveEntries)
{
  AL_USDMAYA_UNTESTED;
}


//  fileio::translators::TranslatorContextPtr SchemaNodeRefDB::context();
//  fileio::translators::TranslatorManufacture& SchemaNodeRefDB::translatorManufacture()
//  nodes::ProxyShape* SchemaNodeRefDB::proxy() const
TEST(SchemaNodeRefDB, proxy)
{
  AL_USDMAYA_UNTESTED;
}

//  void SchemaNodeRefDB::outputPrims(std::ostream& os);
