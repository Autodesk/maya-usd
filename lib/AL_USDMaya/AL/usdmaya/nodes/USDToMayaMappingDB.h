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
#pragma once
#include <cstdint>
#include "maya/MDagPath.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a mapping between a prim path and the transform node under which the prim was imported via a custom plugin
///         translator.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class SchemaNodeRef
{
public:

  /// \brief  ctor
  /// \param  path the prim path of the items we will be tracking
  /// \param  mayaObj the maya transform
  SchemaNodeRef(const SdfPath& path, MObject mayaObj)
    : m_primPath(path), m_mayaObj(mayaObj) {}

  /// \brief  dtor
  ~SchemaNodeRef() {}

  /// \brief  get the prim path of this reference
  /// \return the prim path for this reference
  const SdfPath& primPath() const
    { return m_primPath; }

  /// \brief  get the maya object of the node
  /// \return the maya node for this reference
  MObject mayaObject() const
    { return m_mayaObj.object(); }

private:
  SdfPath m_primPath;
  MObjectHandle m_mayaObj;
};


//----------------------------------------------------------------------------------------------------------------------
/// \brief  The proxy shape node needs to store a mapping of all the schema nodes it has brought into the Maya scene.
///         This class holds this mapping
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class SchemaNodeRefDB
{
public:

  /// comparison utility (for sorting array of pointers to node references based on their path)
  struct value_compare
  {
    /// \brief  compare schema node ref to path
    /// \param  a the node ref pointer on the left of the < operator
    /// \param  b the sdf path on the right of the < operator
    /// \return true if a->primPath() < b, false otherwise
    inline bool operator() (const SchemaNodeRef& a, const SdfPath& b) const
      { return a.primPath() < b; }

    /// \brief  compare schema node ref to path
    /// \param  a the sdf path on the left of the < operator
    /// \param  b the node ref pointer on the right of the < operator
    /// \return true if a < b->primPath(), false otherwise
    inline bool operator() (const SdfPath& a, const SchemaNodeRef& b) const
      { return a < b.primPath(); }

    /// compare schema node ref to schema node ref
    /// \brief  compare schema node ref to path
    /// \param  a the node ref pointer on the left of the < operator
    /// \param  b the node ref pointer on the right of the < operator
    /// \return true if a->primPath() < b->primPath(), false otherwise
    inline bool operator() (const SchemaNodeRef& a, const SchemaNodeRef& b) const
      { return a.primPath() < b.primPath(); }
  };

  /// \brief  constructor
  /// \param  proxy the owning proxy shape of this object
  SchemaNodeRefDB(nodes::ProxyShape* const proxy);

  /// \brief  dtor
  ~SchemaNodeRefDB();

  /// \brief  When adding new schema node entries into this DB, rather than forcing a sort after each entry is added,
  ///         it makes more sense to add all the node refs we need to, and then sort at the end. To this end, you
  ///         should call lock on the DB prior to adding any entries, and unlock when you have finished adding refs
  void lock()
    {}

  /// \brief  This method is used to determine whether this DB has an entry for the specified prim path and the given type.
  ///         This is used within a variant switch to determine if a node can be updated, or whether it needs to be imported.
  /// \param  path the path to the prim to query
  /// \param  type the type of prim
  /// \return true if an entry is found that matches, false otherwise
  bool hasEntry(const SdfPath& path, const TfToken& type)
  {
    auto it = std::lower_bound(m_nodeRefs.begin(), m_nodeRefs.end(), path, value_compare());
    if(it != m_nodeRefs.end() && (it->primPath() == path))
    {
      return type == context()->getTypeForPath(path);
    }
    return false;
  }

  /// \brief  create a mapping between the prim path, and the MObject that was created by a translator plugin
  ///         when importing it.
  /// \param  primPath the prim path that was being translated by a custom translator, and resulted in a Maya node
  ///         being created.
  /// \param  primObj the maya node that was created by a custom translator plug-in.
  void addEntry(const SdfPath& primPath, const MObject& primObj);

  /// \brief  call this method after adding some entries into the DB
  void unlock()
    { std::sort(m_nodeRefs.begin(), m_nodeRefs.end(), value_compare()); }

  /// \brief  This is called during a variant switch to determine whether the variant switch will allow Maya nodes
  ///         to be updated, or whether they need to be deleted.
  /// \param  primPath the path to the prim that triggered the variant switch
  /// \param  itemsToRemove the returned list of items that need to be removed
  /// \param callPreUnload true calling the preUnload on all the prims is needed.
  void preRemoveEntry(const SdfPath& primPath, SdfPathVector& itemsToRemove, bool callPreUnload=true);

  /// \brief  call this to remove a prim from the DB (you do not need to lock/unlock here).
  /// \param  itemsToRemove the prims that need to be removed from the DB. tearDown will be called on each prim
  void removeEntries(const SdfPathVector& itemsToRemove);

  /// \brief  debugging util - prints a list of the schema nodes that currently exist within maya
  /// \param  os the output stream to write to
  void outputPrims(std::ostream& os);

  /// \brief  access the current translator context for the schema prims
  /// \return the current translator context associated with this DB
  fileio::translators::TranslatorContextPtr context();

  /// \brief  access the current translator factory for the schema prims
  /// \return the current translator manufacture associated with this DB
  inline fileio::translators::TranslatorManufacture& translatorManufacture()
    { return m_translatorManufacture; }

  /// \brief  returns the proxy shape node associated with the schema prims in this DB
  /// \return the proxy shape associated with this DB
  inline nodes::ProxyShape* proxy() const
    { return m_proxy; }

  /// \brief  serialises the database into a text string
  /// \return the serialised state of the DB
  MString serialize();

  /// \brief  deserialises the database from a text string
  /// \param  str  the serialised state of the DB
  void deserialize(const MString& str);

private:
  void unloadPrim(
      const SdfPath& primPath,
      const MObject& primObj);
  void preUnloadPrim(
      UsdPrim& primPath,
      const MObject& primObj);

  typedef std::vector<SchemaNodeRef> SchemaNodeRefs;
  SchemaNodeRefs m_nodeRefs;
  nodes::ProxyShape* const m_proxy;
  fileio::translators::TranslatorContextPtr m_context;
  fileio::translators::TranslatorManufacture m_translatorManufacture;
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

