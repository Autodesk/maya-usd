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
#include "AL/usdmaya/Common.h"
#include "maya/MPxData.h"
#include "maya/MGlobal.h"
#include "maya/MObject.h"
#include "maya/MObjectHandle.h"
#include "maya/MObjectArray.h"
#include "maya/MDGModifier.h"
#include "pxr/pxr.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/usd/usd/prim.h"

#include <map>
#include <unordered_map>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE


namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

typedef std::vector<MObjectHandle> MObjectHandleArray;

//----------------------------------------------------------------------------------------------------------------------
/// \brief   This class provides a context to store mappings between UsdPrims, and the Maya nodes that represent them.
///
/// \ingroup translators
//----------------------------------------------------------------------------------------------------------------------
struct TranslatorContext
  : public TfRefBase
{
public:
  typedef TranslatorContext This; ///< this type
  typedef TfRefPtr<This> RefPtr; ///< pointer to this type

  /// \brief  construct a new context for the specified proxy shape node
  /// \param  proxyShape the proxy shape to associate the context with
  /// \return a new context
  static RefPtr create(nodes::ProxyShape* proxyShape)
    { return TfCreateRefPtr(new This(proxyShape)); }

  /// \brief  return the proxy shape associated with this context
  /// \return the proxy shape
  const nodes::ProxyShape* getProxyShape() const
    { return m_proxyShape; }

  /// \brief  return the usd stage associated with this context
  /// \return the usd stage
  UsdStageRefPtr getUsdStage() const;

  /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  prim the usd prim
  /// \param  object the returned handle
  /// \return true if the prim exists
  bool getTransform(const UsdPrim& prim, MObjectHandle& object)
    { return getTransform(prim.GetPath(), object); }

  /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  path the usd prim path
  /// \param  object the returned handle
  /// \return true if the prim exists
  bool getTransform(const SdfPath& path, MObjectHandle& object);

  /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  prim the usd prim
  /// \param  object the returned handle
  /// \param  type the type ID of the maya object you wish to retrieve. If the type ID is 0, the first node
  ///         found will be returned. This may be useful if a prim type may create a type of node
  ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or phong based on
  ///         some enum attribute). Another alternative would be to query all of the maya nodes via getMObjects
  /// \return true if the prim exists
  bool getMObject(const UsdPrim& prim, MObjectHandle& object, MTypeId type)
    { return getMObject(prim.GetPath(), object, type); }

  /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  path the usd prim path
  /// \param  object the returned handle
  /// \param  type the type ID of the maya object you wish to retrieve. If the type ID is 0, the first node
  ///         found will be returned. This may be useful if a prim type may create a type of node
  ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or phong based on
  ///         some enum attribute). Another alternative would be to query all of the maya nodes via getMObjects
  /// \return true if the prim exists
  bool getMObject(const SdfPath& path, MObjectHandle& object, MTypeId type);

  /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  prim the usd prim
  /// \param  object the returned handle
  /// \param  type the type of the maya object you wish to retrieve. If the type is MFn::kInvalid, then the
  ///         first node found will be returned. This may be useful if a prim type may create a type of node
  ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or phong based on
  ///         some enum attribute). Another alternative would be to query all of the maya nodes via getMObjects
  /// \return true if the prim exists
  bool getMObject(const UsdPrim& prim, MObjectHandle& object, MFn::Type type)
    { return getMObject(prim.GetPath(), object, type); }

  /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does, that will
  ///         be returned in the object handle. If the object is found, true is returned, otherwise false.
  /// \param  path the usd prim path
  /// \param  object the returned handle
  /// \param  type the type of the maya object you wish to retrieve. If the type is MFn::kInvalid, then the
  ///         first node found will be returned. This may be useful if a prim type may create a type of node
  ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or phong based on
  ///         some enum attribute). Another alternative would be to query all of the maya nodes via getMObjects
  /// \return true if the prim exists
  bool getMObject(const SdfPath& path, MObjectHandle& object, MFn::Type type);

  /// \brief  returns all of the maya nodes that were created by the specific prim
  /// \param  prim the prim to query
  /// \param  returned the returned list of MObjects
  /// \return true if a reference to the prim was found
  bool getMObjects(const UsdPrim& prim, MObjectHandleArray& returned)
    { return getMObjects(prim.GetPath(), returned); }

  /// \brief  returns all of the maya nodes that were created by the specific prim
  /// \param  path the path to the prim to query
  /// \param  returned the returned list of MObjects
  /// \return true if a reference to the prim was found
  bool getMObjects(const SdfPath& path, MObjectHandleArray& returned);

  /// \brief  If within your custom translator plug-in you need to create any maya nodes, associate that maya
  ///         node with the prim path by calling this method
  /// \param  prim the prim you are currently importing in a translator
  /// \param  object the handle to the maya node you have created.
  void insertItem(const UsdPrim& prim, MObjectHandle object);

  /// \brief  during a variant switch, if we lose a prim, then it's path will be passed into this method, and
  ///         all the maya nodes that were created for it will be nuked.
  /// \param  prim the usd prim that was removed due to a variant switch
  void removeItems(const UsdPrim& prim)
    { removeItems(prim.GetPath()); }

  /// \brief  during a variant switch, if we lose a prim, then it's path will be passed into this method, and
  ///         all the maya nodes that were created for it will be nuked.
  /// \param  path path to the usd prim that was removed due to a variant switch
  void removeItems(const SdfPath& path);

  /// \brief  dtor
  ~TranslatorContext();
   
  /// \brief  given a path to a prim, return the prim type we are aware of at that path
  /// \param  path the prim path of a prim that was imported via a custom translator plug-in
  /// \return the type name for that prim
  TfToken getTypeForPath(SdfPath path) const
  {
    const auto it = m_primMapping.find(path.GetString());
    if(it != m_primMapping.end())
    {
      return it->second.m_type;
    }
    return TfToken();
  }

  /// \brief  this method is used after a variant switch to check to see if the prim types have changed in the
  ///         stage, and will update the internal state accordingly.
  void updatePrimTypes();

  /// \brief  Internal method.
  ///         If within your custom translator plug-in you need to create any maya nodes, associate that maya
  ///         node with the prim path by calling this method
  /// \param  prim the prim you are currently importing in a translator
  /// \param  object the handle to the maya node you have created.
  void registerItem(const UsdPrim& prim, MObjectHandle object);
   
  /// \brief  serialises the content of the translator context to a text string.
  /// \return the translator context serialised into a string
  MString serialise() const;

  /// \brief  deserialises the string back into the translator context
  /// \param  string the string to deserialised
  void deserialise(const MString& string);

  /// \brief  debugging utility to help keep track of prims during a variant switch
  void validatePrims();

private:

  TranslatorContext(const nodes::ProxyShape* proxyShape)
    : m_proxyShape(proxyShape), m_primMapping()
    {}

  const nodes::ProxyShape* m_proxyShape;

  struct PrimLookup
  {
    TfToken m_type;
    MObjectHandle m_object;
    MObjectHandleArray m_createdNodes;
  };

  // map between a usd prim path and either a dag parent node or
  // a dependency node
  std::unordered_map<std::string, PrimLookup> m_primMapping;
};

typedef TfRefPtr<TranslatorContext> TranslatorContextPtr;

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
