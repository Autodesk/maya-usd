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
#pragma once

#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/DebugCodes.h>
#include <AL/usdmaya/ForwardDeclares.h>
#include <mayaUsdUtils/ForwardDeclares.h>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/refPtr.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxData.h>

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

typedef std::vector<MObjectHandle> MObjectHandleArray;
typedef std::map<SdfPath, SdfPath> SdfInstanceMap;

//----------------------------------------------------------------------------------------------------------------------
/// \brief   Transient aggregate of values that aims to direct the Translation of Prims. Typically
/// an object of this struct is created and used for a set group of prim path translations then it
/// is destroyed.
///
/// \ingroup translators
//----------------------------------------------------------------------------------------------------------------------
struct TranslatorParameters
{
    /// \brief Flag that determines if all Prim schema types should be forced to be imported
    inline void setForcePrimImport(bool forceImport) { m_forcePrimImport = forceImport; }

    /// \brief Retrieves the flag that determines if all the Prim schema types should be imported
    inline bool forceTranslatorImport() const { return m_forcePrimImport; }

    /// \brief  should pushToPrim be enabled on created transforms
    inline void setPushToPrim(bool value) { m_pushToPrim = value; }

    /// \brief  should pushToPrim be enabled on created transforms
    inline bool pushToPrim() const { return m_pushToPrim; }

    /// \brief  should readAnimatedValues be enabled on created transforms
    inline void setReadAnimatedValues(bool value) { m_readAnimatedValues = value; }

    /// \brief  should readAnimatedValues be enabled on created transforms
    inline bool readAnimatedValues() const { return m_readAnimatedValues; }

private:
    bool m_forcePrimImport = false;
    bool m_pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim");
    bool m_readAnimatedValues = false;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   This class provides a context to store mappings between UsdPrims, and the Maya nodes
/// that represent them.
///
/// \ingroup translators
//----------------------------------------------------------------------------------------------------------------------
struct TranslatorContext
    : public TfRefBase
    , TfWeakBase
{
public:
    typedef TranslatorContext This;   ///< this type
    typedef TfRefPtr<This>    RefPtr; ///< pointer to this type

    /// \brief  construct a new context for the specified proxy shape node
    /// \param  proxyShape the proxy shape to associate the context with
    /// \return a new context
    static RefPtr create(nodes::ProxyShape* proxyShape)
    {
        RefPtr res = TfCreateRefPtr(new This(proxyShape));
        res->setForceDefaultRead(false);
        return res;
    }

    /// \brief  return the proxy shape associated with this context
    /// \return the proxy shape
    const nodes::ProxyShape* getProxyShape() const { return m_proxyShape; }

    /// \brief  return the usd stage associated with this context
    /// \return the usd stage
    AL_USDMAYA_PUBLIC
    UsdStageRefPtr getUsdStage() const;

    /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that
    /// will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  prim the usd prim
    /// \param  object the returned handle
    /// \return true if the prim exists
    bool getTransform(const UsdPrim& prim, MObjectHandle& object)
    {
        return getTransform(prim.GetPath(), object);
    }

    /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does,
    /// that will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  path the usd prim path
    /// \param  object the returned handle
    /// \return true if the prim exists
    AL_USDMAYA_PUBLIC
    bool getTransform(const SdfPath& path, MObjectHandle& object);

    /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that
    /// will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  prim the usd prim
    /// \param  object the returned handle
    /// \param  type the type ID of the maya object you wish to retrieve. If the type ID is 0, the
    /// first node
    ///         found will be returned. This may be useful if a prim type may create a type of node
    ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or
    ///         phong based on some enum attribute). Another alternative would be to query all of
    ///         the maya nodes via getMObjects
    /// \return true if the prim exists
    bool getMObject(const UsdPrim& prim, MObjectHandle& object, MTypeId type)
    {
        return getMObject(prim.GetPath(), object, type);
    }

    /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does,
    /// that will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  path the usd prim path
    /// \param  object the returned handle
    /// \param  type the type ID of the maya object you wish to retrieve. If the type ID is 0, the
    /// first node
    ///         found will be returned. This may be useful if a prim type may create a type of node
    ///         that is not known at compile time (e.g. a prim that creates a lambert, blinn, or
    ///         phong based on some enum attribute). Another alternative would be to query all of
    ///         the maya nodes via getMObjects
    /// \return true if the prim exists
    AL_USDMAYA_PUBLIC
    bool getMObject(const SdfPath& path, MObjectHandle& object, MTypeId type);

    /// \brief  given a USD prim, this will see whether a maya node exists for it. If it does, that
    /// will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  prim the usd prim
    /// \param  object the returned handle
    /// \param  type the type of the maya object you wish to retrieve. If the type is MFn::kInvalid,
    /// then the
    ///         first node found will be returned. This may be useful if a prim type may create a
    ///         type of node that is not known at compile time (e.g. a prim that creates a lambert,
    ///         blinn, or phong based on some enum attribute). Another alternative would be to query
    ///         all of the maya nodes via getMObjects
    /// \return true if the prim exists
    bool getMObject(const UsdPrim& prim, MObjectHandle& object, MFn::Type type)
    {
        return getMObject(prim.GetPath(), object, type);
    }

    /// \brief  given a USD prim path, this will see whether a maya node exists for it. If it does,
    /// that will
    ///         be returned in the object handle. If the object is found, true is returned,
    ///         otherwise false.
    /// \param  path the usd prim path
    /// \param  object the returned handle
    /// \param  type the type of the maya object you wish to retrieve. If the type is MFn::kInvalid,
    /// then the
    ///         first node found will be returned. This may be useful if a prim type may create a
    ///         type of node that is not known at compile time (e.g. a prim that creates a lambert,
    ///         blinn, or phong based on some enum attribute). Another alternative would be to query
    ///         all of the maya nodes via getMObjects
    /// \return true if the prim exists
    AL_USDMAYA_PUBLIC
    bool getMObject(const SdfPath& path, MObjectHandle& object, MFn::Type type);

    /// \brief  returns all of the maya nodes that were created by the specific prim
    /// \param  prim the prim to query
    /// \param  returned the returned list of MObjects
    /// \return true if a reference to the prim was found
    bool getMObjects(const UsdPrim& prim, MObjectHandleArray& returned)
    {
        return getMObjects(prim.GetPath(), returned);
    }

    /// \brief  returns all of the maya nodes that were created by the specific prim
    /// \param  path the path to the prim to query
    /// \param  returned the returned list of MObjects
    /// \return true if a reference to the prim was found
    AL_USDMAYA_PUBLIC
    bool getMObjects(const SdfPath& path, MObjectHandleArray& returned);

    /// \brief  If within your custom translator plug-in you need to create any maya nodes,
    /// associate that maya
    ///         node with the prim path by calling this method
    /// \param  prim the prim you are currently importing in a translator
    /// \param  object the handle to the maya node you have created.
    AL_USDMAYA_PUBLIC
    void insertItem(const UsdPrim& prim, MObjectHandle object);

    /// \brief  during a variant switch, if we lose a prim, then it's path will be passed into this
    /// method, and
    ///         all the maya nodes that were created for it will be nuked.
    /// \param  prim the usd prim that was removed due to a variant switch
    void removeItems(const UsdPrim& prim) { removeItems(prim.GetPath()); }

    /// \brief  during a variant switch, if we lose a prim, then it's path will be passed into this
    /// method, and
    ///         all the maya nodes that were created for it will be nuked.
    /// \param  path path to the usd prim that was removed due to a variant switch
    AL_USDMAYA_PUBLIC
    void removeItems(const SdfPath& path);

    /// \brief  dtor
    AL_USDMAYA_PUBLIC
    ~TranslatorContext();

    /// \brief  given a path to a prim, return the translator we are aware of at that path
    /// \param  path the prim path of a prim that was imported via a custom translator plug-in
    /// \return the type name for that prim
    std::string getTranslatorIdForPath(SdfPath path) const
    {
        const auto it = find(path);
        if (it != m_primMapping.end()) {
            return it->translatorId();
        }
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "TranslatorContext::getTranslatorForPath did not find item in mapping.%s\n",
                path.GetText());
        return std::string();
    }

    /// \brief  this method is used after a variant switch to check to see if the prim types have
    /// changed in the
    ///         stage, and will update the internal state accordingly.
    AL_USDMAYA_PUBLIC
    void updatePrimTypes();

    /// \brief  Internal method.
    ///         If within your custom translator plug-in you need to create any maya nodes,
    ///         associate that maya node with the prim path by calling this method
    /// \param  prim the prim you are currently importing in a translator
    /// \param  object the handle to the maya node you have created.
    AL_USDMAYA_PUBLIC
    void registerItem(const UsdPrim& prim, MObjectHandle object);

    /// \brief  serialises the content of the translator context to a text string.
    /// \return the translator context serialised into a string
    AL_USDMAYA_PUBLIC
    MString serialise() const;

    /// \brief  deserialises the string back into the translator context
    /// \param  string the string to deserialised
    AL_USDMAYA_PUBLIC
    void deserialise(const MString& string);

    /// \brief  debugging utility to help keep track of prims during a variant switch
    AL_USDMAYA_PUBLIC
    void validatePrims();

    /// \brief  This method is used to determine whether this DB has an entry for the specified prim
    /// path and the given translator.
    ///         This is used within a variant switch to determine if a node can be updated, or
    ///         whether it needs to be imported.
    /// \param  path the path to the prim to query
    /// \param  translatorId
    /// \return true if an entry is found that matches, false otherwise
    bool hasEntry(const SdfPath& path, const std::string& translatorId)
    {
        auto it = find(path);
        if (it != m_primMapping.end()) {
            return translatorId == it->translatorId();
        }
        return false;
    }

    /// \brief  This is called during a variant switch to determine whether the variant switch will
    /// allow Maya nodes
    ///         to be updated, or whether they need to be deleted.
    /// \param  primPath the path to the prim that triggered the variant switch
    /// \param  itemsToRemove the returned list of items that need to be removed
    /// \param callPreUnload true calling the preUnload on all the prims is needed.
    AL_USDMAYA_PUBLIC
    void preRemoveEntry(
        const SdfPath& primPath,
        SdfPathVector& itemsToRemove,
        bool           callPreUnload = true);

    /// \brief  call this to remove a prim from the DB (you do not need to lock/unlock here).
    /// \param  itemsToRemove the prims that need to be removed from the DB. tearDown will be called
    /// on each prim
    AL_USDMAYA_PUBLIC
    void removeEntries(const SdfPathVector& itemsToRemove);

    /// \brief  Get unique key for specified prim path.
    /// \param  path target prim path
    /// \return unique key value
    std::size_t getUniqueKeyForPath(const SdfPath& path)
    {
        auto it = find(path);
        if (it != m_primMapping.end()) {
            return it->uniqueKey();
        }
        return 0;
    }

    /// \brief  Update all unique keys when saving Maya scene.
    AL_USDMAYA_PUBLIC
    void updateUniqueKeys();

    /// \brief  Update unique key for a specified prim.
    /// \param  prim target prim
    AL_USDMAYA_PUBLIC
    void updateUniqueKey(const UsdPrim& prim);

    /// \brief  An internal structure used to store a mapping between an SdfPath, the type of prim
    /// found at that location,
    ///         the maya transform that may have been created (assuming the translator plugin
    ///         specifies that it needs a parent transform), and any nodes that the translator
    ///         plugin may have created.
    struct PrimLookup
    {
        /// \brief  ctor
        /// \param  path the prim path of the items we will be tracking
        /// \param  translatorId Used to help us determine which translator plugin to call to tear
        /// down this prim. \param  mayaObj the maya transform
        PrimLookup(const SdfPath& path, const std::string& translatorId, MObject mayaObj)
            : m_path(path)
            , m_translatorId(translatorId)
            , m_uniqueKey(0)
            , m_object(mayaObj)
            , m_createdNodes()
        {
        }

        /// \brief  dtor
        ~PrimLookup() { }

        /// \brief  get the prim path of this reference
        /// \return the prim path for this reference
        const SdfPath& path() const { return m_path; }

        /// \brief  get the maya object of the node
        /// \return the maya node for this reference
        MObjectHandle objectHandle() const { return m_object; }

        /// \brief  get the maya object of the node
        /// \return the maya node for this reference
        MObject object() const { return m_object.object(); }

        /// \brief  get the schema type of the prim
        /// \return the schema type stored for this prim
        std::string translatorId() const { return m_translatorId; }

        /// \brief  get the unique key of the prim
        /// \return the unique key for this prim
        const std::size_t uniqueKey() const { return m_uniqueKey; }

        /// \brief  set the unique key for this prim
        /// \param  key the unique key for this prim
        void setUniqueKey(const std::size_t key) { m_uniqueKey = key; }

        /// \brief  get the prim type
        /// \return the type stored for this prim
        TfToken type() const { return m_type; }

        /// \brief  get the maya object of the node
        /// \return the maya node for this reference
        void setNode(MObject node) { m_object = node; }

        /// \brief  get created maya nodes
        /// \return the created maya nodes for this prim translator
        MObjectHandleArray& createdNodes() { return m_createdNodes; }

        /// \brief  get created maya nodes
        /// \return the created maya nodes for this prim translator
        const MObjectHandleArray& createdNodes() const { return m_createdNodes; }

    private:
        SdfPath            m_path;
        std::string        m_translatorId;
        std::size_t        m_uniqueKey;
        TfToken            m_type;
        MObjectHandle      m_object;
        MObjectHandleArray m_createdNodes;
    };

    /// a sorted array of prim mappings
    typedef std::vector<PrimLookup> PrimLookups;

    /// comparison utility (for sorting array of pointers to node references based on their path)
    struct value_compare
    {
        /// \brief  compare schema node ref to path
        /// \param  a the node ref pointer on the left of the < operator
        /// \param  b the sdf path on the right of the < operator
        /// \return true if a->primPath() < b, false otherwise
        inline bool operator()(const PrimLookup& a, const SdfPath& b) const { return a.path() < b; }

        /// \brief  compare schema node ref to path
        /// \param  a the sdf path on the left of the < operator
        /// \param  b the node ref pointer on the right of the < operator
        /// \return true if a < b->primPath(), false otherwise
        inline bool operator()(const SdfPath& a, const PrimLookup& b) const { return a < b.path(); }

        /// compare schema node ref to schema node ref
        /// \brief  compare schema node ref to path
        /// \param  a the node ref pointer on the left of the < operator
        /// \param  b the node ref pointer on the right of the < operator
        /// \return true if a->primPath() < b->primPath(), false otherwise
        inline bool operator()(const PrimLookup& a, const PrimLookup& b) const
        {
            return a.path() < b.path();
        }
    };

    /// \brief  This is used for testing only. Do not call.
    void clearPrimMappings() { m_primMapping.clear(); }

    /// \brief  add geometry to the exclusion list
    /// \param  newPath the path to add as an excluded translator path
    /// \return true if the exclusion was added, false if it wasn't added since it might be already
    /// there
    AL_USDMAYA_PUBLIC
    bool addExcludedGeometry(const SdfPath& newPath);

    /// \brief  remove geometry from the exclusion list
    /// \param  newPath the path to add as an excluded translator path
    /// \return true if the exclusion was removed, false if it wasn't removed or it may have never
    /// existed
    AL_USDMAYA_PUBLIC
    bool removeExcludedGeometry(const SdfPath& newPath)
    {
        auto foundPath = m_excludedGeometry.find(newPath);
        if (foundPath == m_excludedGeometry.end()) {
            return false;
        }
        m_excludedGeometry.erase(newPath);
        m_isExcludedGeometryDirty = true;
        return true;
    }

    /// \brief  retrieve currently excluded translator geometries
    /// \return  retrieve currently excluded translator geometries
    inline const SdfInstanceMap& excludedGeometry() { return m_excludedGeometry; }

    /// \brief Retrieves if the the excluded geometry has been pushed to the renderer
    /// \return true if the excluded list hasn't been pushed the the renderer yet
    inline bool isExcludedGeometryDirty() { return m_isExcludedGeometryDirty; }

private:
    void unloadPrim(const SdfPath& primPath, const MObject& primObj);
    void preUnloadPrim(UsdPrim& primPath, const MObject& primObj);

    bool isNodeAncestorOf(MObjectHandle ancestorHandle, MObjectHandle objectHandleToTest);

    /// \brief test if the prim was translated into any MObject(s), that sits underneath the parent
    /// MObject. \return true if the prim maps to a MObject inside the Maya Dag tree.
    bool isPrimInTransformChain(const SdfPath& path);

    inline PrimLookups::iterator find(const SdfPath& path)
    {
        PrimLookups::iterator end = m_primMapping.end();
        PrimLookups::iterator it
            = std::lower_bound(m_primMapping.begin(), end, path, value_compare());
        if (it != end) {
            if (it->path() == path)
                return it;
        }
        return end;
    }

    inline PrimLookups::const_iterator find(const SdfPath& path) const
    {
        PrimLookups::const_iterator end = m_primMapping.end();
        PrimLookups::const_iterator it
            = std::lower_bound(m_primMapping.begin(), end, path, value_compare());
        if (it != end) {
            if (it->path() == path)
                return it;
        }
        return end;
    }

    inline PrimLookups::iterator findLocation(const SdfPath& path)
    {
        PrimLookups::iterator end = m_primMapping.end();
        PrimLookups::iterator it
            = std::lower_bound(m_primMapping.begin(), end, path, value_compare());
        return it;
    }

    TranslatorContext(nodes::ProxyShape* proxyShape)
        : m_proxyShape(proxyShape)
        , m_primMapping()
    {
    }

    nodes::ProxyShape* m_proxyShape;

    // map between a usd prim path and either a dag parent node or
    // a dependency node
    PrimLookups m_primMapping;

    // list of geometry that has been request to be excluded during the translation
    SdfInstanceMap m_excludedGeometry;
    bool           m_isExcludedGeometryDirty;

public:
    void setForceDefaultRead(bool forceDefaultRead) { m_forceDefaultRead = forceDefaultRead; }

    bool getForceDefaultRead() { return m_forceDefaultRead; }

private:
    // If true, will explicitly read default attribute values.
    bool m_forceDefaultRead;
};

typedef TfRefPtr<TranslatorContext> TranslatorContextPtr;

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
