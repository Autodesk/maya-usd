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

#include "AL/maya/event/MayaEventManager.h"

#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/maya/utils/NodeHelper.h"

#include "AL/usdmaya/Api.h"

#include "AL/usdmaya/ForwardDeclares.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"
#include "AL/usdmaya/SelectabilityDB.h"

#include "AL/usd/transaction/Notice.h"

#include "maya/MDagModifier.h"
#include "maya/MDagPath.h"
#include "maya/MGlobal.h"
#include "maya/MNodeMessage.h"
#include "maya/MPxSurfaceShape.h"
#include "maya/MSelectionList.h"

#if MAYA_API_VERSION < 201800
#include "maya/MViewport2Renderer.h"
#endif

#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImagingGL/renderParams.h"

#include <mayaUsd/nodes/proxyShapeBase.h>

#if defined(WANT_UFE_BUILD)
#include "ufe/ufe.h"

UFE_NS_DEF {
    class Path;
    class PathSegment;
}
#endif

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

// Note: you MUST forward declare LayerManager, and not include LayerManager.h;
// The reason is that LayerManager.h includes MPxLocatorNode.h, which on Linux,
// ends up bringing in Xlib.h, which has this unfortunate macro:
//
//    #define Bool int
//
// This, in turn, will cause problems if you try to use SdfValueTypeNames->Bool,
// as in test_usdmaya_AttributeType.cpp
class LayerManager;

PXR_NAMESPACE_CLOSE_SCOPE;

namespace AL {
namespace usdmaya {
namespace nodes {

class Engine;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A helper class to store the state that is modified during a change to the current selection within a
///         proxy shape. The state it maintains includes:
///
///         \li The USD paths to be selected / deselected
///         \li The Dag modifiers needed to create/destroy the associated maya nodes
///         \li The maya selection list prior to the selection
///         \li The maya selection list after the selection change
///         \li The internal transformation references before and after the selection change
///
///         This class is intended to exist as a member variable on a MEL selection command. Once constructed, the class
///         should be passed to the ProxyShape::doSelect method to construct the internal state changes. At that point,
///         you may call doIt() to perform the changes, undoIt() to revert the changes.
//----------------------------------------------------------------------------------------------------------------------
struct SelectionUndoHelper
{
  /// a hash set of SdfPaths
  typedef TfHashSet<SdfPath, SdfPath::Hash> SdfPathHashSet;

  /// \brief  Construct with the arguments to select / deselect nodes on a proxy shape
  /// \param  proxy pointer to the maya node on which the selection operation will be performed.
  /// \param  paths the USD paths to be selected / toggled / unselected
  /// \param  mode the selection mode (add, remove, xor, etc)
  /// \param  internal if the internal flag is set, then modifications to Maya's selection list will NOT occur.
  SelectionUndoHelper(nodes::ProxyShape* proxy, const SdfPathHashSet& paths, MGlobal::ListAdjustment mode, bool internal = false);

  /// \brief  performs the selection changes
  void doIt();

  /// \brief  will undo the selection changes
  void undoIt();

private:
  friend class ProxyShape;
  nodes::ProxyShape* m_proxy;
  SdfPathHashSet m_paths;
  SdfPathHashSet m_previousPaths;
  MGlobal::ListAdjustment m_mode;
  MDagModifier m_modifier1;
  MDagModifier m_modifier2;
  MSelectionList m_previousSelection;
  MSelectionList m_newSelection;
  std::vector<std::pair<SdfPath, MObject>> m_insertedRefs;
  std::vector<std::pair<SdfPath, MObject>> m_removedRefs;
  bool m_internal;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Used as a way to construct a simple selection list that allows for selection highlighting without
///         creating/destroying transforms.
//----------------------------------------------------------------------------------------------------------------------
class SelectionList
{
public:
  /// a hash set of SdfPaths
  typedef TfHashSet<SdfPath, SdfPath::Hash> SdfPathHashSet;

  /// \brief  default ctor
  SelectionList() = default;

  /// \brief  copy ctor
  /// \param  sl the selection list to copy
  SelectionList(const SelectionList& sl) = default;

  /// \brief  dtor
  ~SelectionList() = default;

  /// \brief  clear the selection list
  inline void clear()
    { m_selected.clear(); }

  /// \brief  adds a path to the selection
  /// \param  path to add
  inline void add(SdfPath path)
    {
      m_selected.insert(path);
    }

  /// \brief  removes the path from the selection
  /// \param  path to remove
  inline void remove(SdfPath path)
    {
      auto it = m_selected.find(path);
      if(it != m_selected.end())
      {
        m_selected.erase(it);
      }
    }

  /// \brief  toggles the path in the selection
  /// \param  path to toggle
  inline void toggle(SdfPath path)
    {
      auto insertResult = m_selected.insert(path);
      if (!insertResult.second)
      {
        m_selected.erase(insertResult.first);
      }
    }

  /// \brief  toggles the path in the selection
  /// \param  path to toggle
  inline bool isSelected(const SdfPath& path) const
    { return m_selected.count(path) > 0; }

  /// \brief  the paths in the selection list
  /// \return the selected paths
  inline const SdfPathHashSet& paths() const
    { return m_selected; }

  /// \brief  the paths in the selection list
  /// \return the selected paths
  inline size_t size() const
    { return m_selected.size(); }

private:
  SdfPathHashSet m_selected;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that provides the logic behind a hierarchy traversal through a UsdStage
//----------------------------------------------------------------------------------------------------------------------
struct  HierarchyIterationLogic
{
  /// \brief  ctor
  HierarchyIterationLogic():
      preIteration(nullptr),
      iteration(nullptr),
      postIteration(nullptr)
  {}

  /// \brief  provide a method to be called prior to iteration of the UsdStage hierarchy
  std::function<void()> preIteration;

  /// \brief  a visitor method that is called on each of the UsdPrims in the stage hierarchy
  std::function<void(const fileio::TransformIterator& transformIterator,const UsdPrim& prim)> iteration;

  /// \brief  provide a method to be called after iteration of the UsdStage hierarchy
  std::function<void()> postIteration;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  implements the logic that constructs a list of objects that need to be added or removed from the selectable
///         list of prims within a UsdStage
//----------------------------------------------------------------------------------------------------------------------
struct FindUnselectablePrimsLogic
  : public HierarchyIterationLogic
{
  SdfPathVector newUnselectables; ///< items that need to be made unselectable
  SdfPathVector removeUnselectables; ///< items that are unselectable, but need to be made selectable
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  implements the logic required when searching for locked prims within a UsdStage
//----------------------------------------------------------------------------------------------------------------------
struct FindLockedPrimsLogic
  : public HierarchyIterationLogic
{
};

typedef const HierarchyIterationLogic*  HierarchyIterationLogics[3];
typedef std::unordered_map<SdfPath, MString, SdfPath::Hash > PrimPathToDagPath;

extern AL::event::EventId kPreClearStageCache;
extern AL::event::EventId kPostClearStageCache;
//----------------------------------------------------------------------------------------------------------------------
/// \brief  A custom proxy shape node that attaches itself to a USD file, and then renders it.
///         The stage is held internally as a member variable, and it will be composed based on a change to the
///         "filePath" attribute.
/// \ingroup nodes
//----------------------------------------------------------------------------------------------------------------------
class ProxyShape
  : public MayaUsdProxyShapeBase,
    public AL::maya::utils::NodeHelper,
    public proxy::PrimFilterInterface,
    public AL::event::NodeEvents,
    public TfWeakBase
{
  friend struct SelectionUndoHelper;
  friend class ProxyShapeUI;
  friend class StageReloadGuard;
  friend class ProxyDrawOverride;

  typedef MayaUsdProxyShapeBase ParentClass;
public:

  /// a method that registers all of the events in the ProxyShape
  AL_USDMAYA_PUBLIC
  void registerEvents();

  /// a set of SdfPaths
  typedef TfHashSet<SdfPath, SdfPath::Hash> SdfPathHashSet;

  /// \brief  a mapping between a maya transform (or MObject::kNullObj), and the prim that exists at that location
  ///         in the DAG graph.
  typedef std::vector<std::pair<MObject, UsdPrim> > MObjectToPrim;
  AL_USDMAYA_PUBLIC
  static const char* s_selectionMaskName;

  /// \brief  ctor
  AL_USDMAYA_PUBLIC
  ProxyShape();

  /// \brief  dtor
  AL_USDMAYA_PUBLIC
  ~ProxyShape();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Type Info & Registration
  //--------------------------------------------------------------------------------------------------------------------
  AL_MAYA_DECLARE_NODE();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Input Attributes
  //--------------------------------------------------------------------------------------------------------------------

  // Convenience declarations for attributes inherited from proxy shape
  // base class.
#define AL_INHERIT_ATTRIBUTE(XX) \
  AL_USDMAYA_PUBLIC \
  static const MObject& XX() { return XX##Attr; } \
  AL_USDMAYA_PUBLIC \
  MPlug XX##Plug() const { return MPlug( thisMObject(), XX##Attr ); }

  /// the input USD file path for this proxy
  AL_INHERIT_ATTRIBUTE(filePath);

  /// a path to a prim you want to view with this shape
  AL_INHERIT_ATTRIBUTE(primPath);

  /// a comma seperated list of prims you *don't* want to see.
  AL_INHERIT_ATTRIBUTE(excludePrimPaths);

  /// the input time value (probably connected to time1.outTime)
  AL_INHERIT_ATTRIBUTE(time);

  /// an offset, in GUI time units, where the animation should start playback
  AL_DECL_ATTRIBUTE(timeOffset);

  /// a scalar value that can speed up (values greater than 1) or slow down (values less than one) or reverse (negative
  /// values) the playback of the animation.
  AL_DECL_ATTRIBUTE(timeScalar);

  /// the subdiv complexity used
  AL_INHERIT_ATTRIBUTE(complexity);

  /// display guide - sets shape to display geometry of purpose "guide". See <a href="https://github.com/PixarAnimationStudios/USD/blob/95eef7c9a6662a5362dfc312a186f50c58e27ecd/pxr/usd/lib/usdGeom/imageable.h#L165">imageable.h</a>
  AL_INHERIT_ATTRIBUTE(drawGuidePurpose);

  /// display guide - sets shape to display geometry of purpose "proxy". See <a href="https://github.com/PixarAnimationStudios/USD/blob/95eef7c9a6662a5362dfc312a186f50c58e27ecd/pxr/usd/lib/usdGeom/imageable.h#L165">imageable.h</a>
  AL_INHERIT_ATTRIBUTE(drawProxyPurpose);

  /// display render guide - sets shape to display geometry of purpose "render". See <a href="https://github.com/PixarAnimationStudios/USD/blob/95eef7c9a6662a5362dfc312a186f50c58e27ecd/pxr/usd/lib/usdGeom/imageable.h#L165">imageable.h</a>
  AL_INHERIT_ATTRIBUTE(drawRenderPurpose);

  /// Connection to any layer DG nodes
  AL_DECL_ATTRIBUTE(layers);

  /// serialised session layer (obsolete / deprecated)
  AL_DECL_ATTRIBUTE(serializedSessionLayer);

  /// name of serialized session layer (on the LayerManager)
  AL_DECL_ATTRIBUTE(sessionLayerName);

  /// serialised translator context
  AL_DECL_ATTRIBUTE(serializedTrCtx);

  /// Open the stage unloaded.
  AL_DECL_ATTRIBUTE(unloaded);

  /// ambient display colour
  AL_DECL_ATTRIBUTE(ambient);

  /// diffuse display colour
  AL_DECL_ATTRIBUTE(diffuse);

  /// specular display colour
  AL_DECL_ATTRIBUTE(specular);

  /// emission display colour
  AL_DECL_ATTRIBUTE(emission);

  /// material shininess
  AL_DECL_ATTRIBUTE(shininess);

  /// Serialised reference counts to rebuild the transform reference information
  AL_DECL_ATTRIBUTE(serializedRefCounts);

  /// The path list joined by ",", that will be used as a mask when doing UsdStage::OpenMask()
  AL_DECL_ATTRIBUTE(populationMaskIncludePaths);

  /// Version of the plugin at the time of creation (read-only)
  AL_DECL_ATTRIBUTE(version);

  /// Force the outStageData to be marked dirty (write-only)
  AL_DECL_ATTRIBUTE(stageDataDirty);

  /// Excluded geometry that has been explicitly translated
  AL_DECL_ATTRIBUTE(excludedTranslatedGeometry);

  /// Cache ID of the currently loaded stage)
  AL_DECL_ATTRIBUTE(stageCacheId);

  /// A place to put a custom assetResolver Config string that's passed to the Resolver Context when stage is opened
  AL_DECL_ATTRIBUTE(assetResolverConfig);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Output Attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// outTime = (time - timeOffset) * timeScalar
  AL_DECL_ATTRIBUTE(outTime);

  /// Inject m_stage and m_path members into DG as a data attribute.
  AL_INHERIT_ATTRIBUTE(outStageData);


  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Public Utils
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  provides access to the UsdStage that this proxy shape is currently representing. This will cause a compute
  ///         on the output stage.
  /// \return the proxy shape
  AL_USDMAYA_PUBLIC
  UsdStageRefPtr getUsdStage() const override;

  AL_USDMAYA_PUBLIC
  UsdTimeCode    getTime() const override;  
  
  /// \brief  provides access to the UsdStage that this proxy shape is currently representing
  /// \return the proxy shape
  UsdStageRefPtr usdStage() const
    { return m_stage; }

  /// \brief  gets hold of the attributes on this node that control the rendering in some way
  /// \param  attribs the returned set of render attributes
  /// \param  frameContext the frame context for rendering
  /// \param  dagPath the dag path of the node being rendered
  /// \return true if the attribs could be retrieved (i.e. is the stage is valid)
  bool getRenderAttris(UsdImagingGLRenderParams& attribs, const MHWRender::MFrameContext& frameContext, const MDagPath& dagPath);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   AL_usdmaya_Transform utils
  /// \brief  A set of commands to manipulate the chains of transforms that map to the usd prims found in a stage.
  ///         There are 3 main reasons for a transform node to exist within maya:
  ///         \li They are required to provide a parent transform for a schema prim that will be imported into maya.
  ///         \li They are the parent node of an object that has been selected
  ///         \li The user has requested that they be imported
  //--------------------------------------------------------------------------------------------------------------------

  /// an enum describing the reason that a transform exists in the scene
  enum TransformReason
  {
    kSelection = 1 << 0,  ///< the node exists for selection
    kRequested = 1 << 1,  ///< the node has been requested by a user
    kRequired = 1 << 2    ///< the node is required for an imported schema prim
  };

  /// Selection pick modes (based on USD View application)
  enum class PickMode : int
  {
    kPrims = 0,      ///< Pick the target prim
    kModels = 1,     ///< Pick the nearest model kind ancestor of target
    kInstances = 2,  ///< Pick an instance of the target (if available)
  };

  /// \brief  returns true if the path is required for an imported schema prim
  /// \param  path the path to query
  /// \return true if the path represents a prim that is required.
  inline bool isRequiredPath(const SdfPath& path) const
    { return m_requiredPaths.find(path) != m_requiredPaths.end(); }

  /// \brief  returns the MObject of the maya transform for requested path (or MObject::kNullObj)
  /// \param  path the usd prim path to look up
  /// \return the MObject for the parent transform to the path specified
  inline MObject findRequiredPath(const SdfPath& path) const
    {
      const auto it = m_requiredPaths.find(path);
      if(it != m_requiredPaths.end())
      {
        const MObject& object = it->second.node();
        const MObjectHandle handle(object);
        if(handle.isValid() && handle.isAlive())
        {
            return object;
        }
      }
      return MObject::kNullObj;
    }

  /// \brief  traverses the UsdStage looking for the prims that are going to be handled by custom transformer
  ///         plug-ins.
  /// \param  proxyTransformPath the DAG path of the proxy shape
  /// \param  startPath the path from which iteration needs to start in the UsdStage
  /// \param  manufacture the translator registry
  /// \return the array of prims found that will need to be imported
  AL_USDMAYA_PUBLIC
  std::vector<UsdPrim> huntForNativeNodesUnderPrim(
      const MDagPath& proxyTransformPath,
      SdfPath startPath,
      fileio::translators::TranslatorManufacture& manufacture,
      bool importAll = false);

  /// \brief  constructs a single chain of transform nodes from the usdPrim to the root of this proxy shape.
  /// \param  usdPrim  the leaf of the prim we wish to create
  /// \param  modifier will store the changes as this path is constructed.
  /// \param  reason  the reason why this path is being generated.
  /// \param  modifier2 is specified, the modifier will end up containing a set of commands to switch the pushToPrim
  ///         flags to true. (This has to be done after the transform has been created and initialized, otherwise
  ///         the default maya values will be pushed in the UsdPrim, wiping out the values you just loaded)
  /// \param  createCount the returned number of transforms that were created.
  /// \return the MObject of the parent transform node for the usdPrim
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  AL_USDMAYA_PUBLIC
  MObject makeUsdTransformChain(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0,
      uint32_t* createCount = 0,
      bool pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"),
      bool readAnimatedValues = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues"));

  /// \brief  Will construct AL_usdmaya_Transform nodes for all of the prims from the specified usdPrim and down.
  /// \param  usdPrim the root for the transforms to be created
  /// \param  modifier the modifier that will store the creation steps for the transforms
  /// \param  reason the reason for creating the transforms (use with selection, etc).
  /// \param  modifier2 if specified, this will contain a set of commands that turn on the pushToPrim flag on the transform
  ///         nodes. These flags need to be set after the transforms have been created
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  AL_USDMAYA_PUBLIC
  MObject makeUsdTransforms(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0);

  /// \brief  will destroy all of the AL_usdmaya_Transform nodes from the prim specified, up to the root (unless any
  ///         of those transform nodes are in use by another imported prim).
  /// \param  usdPrim the leaf node in the chain of transforms we wish to remove
  /// \param  modifier will store the changes as this path is removed.
  /// \param  reason  the reason why this path is being removed.
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  AL_USDMAYA_PUBLIC
  void removeUsdTransformChain(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  will destroy all of the AL_usdmaya_Transform nodes from the prim specified, up to the root (unless any
  ///         of those transform nodes are in use by another imported prim).
  /// \param  path the leaf node in the chain of transforms we wish to remove
  /// \param  modifier will store the changes as this path is removed.
  /// \param  reason  the reason why this path is being removed.
  AL_USDMAYA_PUBLIC
  void removeUsdTransformChain(
      const SdfPath& path,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  Will destroy all AL_usdmaya_Transform nodes found underneath the prim (unless those nodes are required
  ///         for another purpose).
  /// \param  usdPrim
  /// \param  modifier the modifier that will be filled with the instructions to delete the transform nodes
  /// \param  reason Are we deleting selected transforms, or those that are required, or requested?
  /// \todo   The mode ProxyShape::kSelection will cause the possibility of instability in the selection system.
  ///         This mode will be removed at a future date
  AL_USDMAYA_PUBLIC
  void removeUsdTransforms(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  /// \brief  Debugging util - prints out the reference counts for each AL_usdmaya_Transform that currently exists
  ///         in the scene
  AL_USDMAYA_PUBLIC
  void printRefCounts() const;

  /// \brief  destroys all internal transform references
  void destroyTransformReferences()
    { m_requiredPaths.clear(); }

  /// \brief  Internal method. Used to filter out a set of paths into groups that need to be created, deleted, or updating.
  /// \param  previousPrims the previous list of prims underneath a prim in the process of a variant change
  /// \param  newPrimSet the list of prims found under neath the prim after the variant change. Prims that can be
  ///         updated will be removed from this list, leaving only the prims that need to be created.
  /// \param  transformsToCreate of the new nodes that can be created, if they are a DAG node (i.e. require a transform),
  ///         then this list will contain those nodes
  /// \param  updatablePrimSet a returned list of prims that can be updated
  /// \param  removedPrimSet a returned set of paths for objects that need to be deleted.
  AL_USDMAYA_PUBLIC
  void filterPrims(
      const SdfPathVector& previousPrims,
      std::vector<UsdPrim>& newPrimSet,
      std::vector<UsdPrim>& transformsToCreate,
      std::vector<UsdPrim>& updatablePrimSet,
      SdfPathVector& removedPrimSet);

  /// \brief  a method that is used within testing only. Returns the current reference count state for the path
  /// \param  path the prim path to test
  /// \param  selected the returned selected reference count
  /// \param  required the returned required reference count
  /// \param  refCount the returned refCount reference count
  AL_USDMAYA_PUBLIC
  void getCounts(SdfPath path, uint32_t& selected, uint32_t& required, uint32_t& refCount)
  {
    auto it = m_requiredPaths.find(path);
    if(it != m_requiredPaths.end())
    {
      selected = it->second.selected();
      required = it->second.required();
      refCount = it->second.refCount();
    }
  }

  /// \brief  Tests to see if a given MObject is currently selected in the proxy shape. If the specified MObject is
  ///         selected, then the path will be filled with the corresponding usd prim path.
  /// \param  obj the input MObject to see if it's selected.
  /// \param  path the returned prim path (if the node is found)
  /// \return true if the maya node is currently selected
  AL_USDMAYA_PUBLIC
  bool isSelectedMObject(MObject obj, SdfPath& path)
  {
    for(auto it : m_requiredPaths)
    {
      if(obj == it.second.node())
      {
        path = it.first;
        if(m_selectedPaths.count(it.first) > 0)
        {
          return true;
        }
        break;
      }
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Plug-in Translator node methods
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  serialises the translator context
  AL_USDMAYA_PUBLIC
  void serialiseTranslatorContext();

  /// \brief  deserialises the translator context
  AL_USDMAYA_PUBLIC
  void deserialiseTranslatorContext();

  /// \brief  gets the maya node path for a prim, stores the mapping and returns it
  /// \param  usdPrim the prim we are bringing in to maya
  /// \param  mayaObject the corresponding maya node
  /// \return  a dag path to the maya object
  AL_USDMAYA_PUBLIC
  MString recordUsdPrimToMayaPath(const UsdPrim &usdPrim,
                                  const MObject &mayaObject);

  /// \brief  returns the stored maya node path for a prim
  /// \param  usdPrim a prim that has been brought into maya
  /// \return  a dag path to the maya object
  AL_USDMAYA_PUBLIC
  MString getMayaPathFromUsdPrim(const UsdPrim& usdPrim) const;

  /// \brief aggregates logic that needs to iterate through the hierarchy looking for properties/metdata on prims
  AL_USDMAYA_PUBLIC
  void findTaggedPrims();

  AL_USDMAYA_PUBLIC
  void findTaggedPrims(const HierarchyIterationLogics& iterationLogics);

  /// \brief  searches for the excluded geometry
  AL_USDMAYA_PUBLIC
  void findExcludedGeometry();

  /// \brief searches for paths which are selectable
  AL_USDMAYA_PUBLIC
  void findSelectablePrims();

  //// \brief iterates the prim hierarchy calling pre/iterate/post like functions that are stored in the passed in objects
  AL_USDMAYA_PUBLIC
  void iteratePrimHierarchy();

  /// \brief  returns the plugin translator registry assigned to this shape
  /// \return the translator registry
  fileio::translators::TranslatorManufacture& translatorManufacture()
    { return m_translatorManufacture; }

  /// \brief  returns the plugin translator context assigned to this shape
  /// \return the translator context
  inline fileio::translators::TranslatorContextPtr& context()
    { return m_context; }

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   ProxyShape selection
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  returns the paths of the selected items within the proxy shape
  /// \return the paths of the selected prims
  AL_USDMAYA_PUBLIC
  SdfPathHashSet& selectedPaths()
    { return m_selectedPaths; }

  /// \brief  Performs a selection operation on this node. Intended for use by the ProxyShapeSelect command only
  /// \param  helper provides the arguments to the selection system, and stores the internal proxy shape state
  ///         changes that need to be done/undone
  /// \param  orderedPaths provides the original (deduplicated) input paths, in order; provided just so that the
  ///         selection commands will return results in the same order they were provided - this is useful so that, if
  ///         the user does, ie, "AL_usdmaya_ProxyShapeSelect -pp /foo/bar -pp /some/thing -proxy myProxyShape",
  //          they will get as the result of the command, ["|proxyRoot|foo|bar", "|proxyRoot|some|thing"], and be able
  //          to know what input SdfPath corresponds to what ouptut maya path
  /// \return true if the operation succeeded
  AL_USDMAYA_PUBLIC
  bool doSelect(SelectionUndoHelper& helper, const SdfPathVector& orderedPaths);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   UsdImaging
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  returns and optionally constructs the usd imaging engine for this proxy shape
  /// \return the imagine engine instance for this shape (shared between draw override and shape ui)
  AL_USDMAYA_PUBLIC
  Engine* engine(bool construct=true);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Miscellaneous
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  unloads all maya references
  /// \todo   I think we could remove this now? The only place this is used is within the post load process to ensure we
  ///         don't duplicate any references in the scene. This can probably be removed.
  AL_USDMAYA_PUBLIC
  void unloadMayaReferences();

  /// \brief  if a root prim has been specified by the user in the proxy shape AE, then this method will return
  ///         the associated prim (used by the Hydra rendering code to select a root to render from). If no prim
  ///         has been specified, the pseudo root will be passed to UsdImaging
  /// \return the prim specified by the user (if valid), the pseudo root if no prim has been specified, or a NULL
  ///         prim if the stage is invalid.
  UsdPrim getRootPrim()
    {
      if(m_stage)
      {
        if(!m_path.IsEmpty())
        {
          UsdPrim prim = m_stage->GetPrimAtPath(m_path);
          if(prim)
            return prim;
        }
        return m_stage->GetPseudoRoot();
      }
      return UsdPrim();
    }

  /// \brief  serialise the state of the transform ref counts prior to saving the file
  AL_USDMAYA_PUBLIC
  void serialiseTransformRefs();

  /// \brief  deserialise the state of the transform ref counts prior to saving the file
  AL_USDMAYA_PUBLIC
  void deserialiseTransformRefs();

  /// \brief Finds the corresponding translator for each decendant prim that has a corresponding Translator 
  ///        and calls preTearDown.
  /// \param[in] path of the point in the hierarchy that is potentially undergoing structural changes
  /// \param[out] outPathVector of SdfPaths that are decendants of 'path'
  AL_USDMAYA_PUBLIC
  void onPrePrimChanged(const SdfPath& path, SdfPathVector& outPathVector);

  /// \brief Re-Creates and updates the maya prim hierarchy starting from the specified primpath
  /// \param[in] primPath of the point in the hierarchy that is potentially undergoing structural changes
  /// \param[in] changedPaths are child paths that existed previously and may not be existing now.
  AL_USDMAYA_PUBLIC
  void onPrimResync(SdfPath primPath, SdfPathVector& changedPaths);

  /// \brief Preps translators for change, and then re-ceates and updates the maya prim hierarchy below the
  ///        specified primPath as if a variant change occurred.
  /// \param[in] primPath of the point in the hierarchy that is potentially undergoing structural changes
  AL_USDMAYA_PUBLIC
  void resync(const SdfPath& primPath);


  // \brief Serialize information unique to this shape
  AL_USDMAYA_PUBLIC
  void serialize(UsdStageRefPtr stage, LayerManager* layerManager);

  // \brief Serialize all layers in proxyShapes to layerManager attributes; called before saving
  AL_USDMAYA_PUBLIC
  static void serializeAll();

  static inline std::vector<MObjectHandle>& GetUnloadedProxyShapes()
  {
    return m_unloadedProxyShapes;
  }

  /// \brief This function starts the prim changed process within the proxyshape
  /// \param[in] changePath is point at which the scene is going to be modified.
  inline void primChangedAtPath(const SdfPath& changePath)
  {
    UsdPrim p = m_stage->GetPrimAtPath(changePath);

    if(!p.IsValid())
    {
      MGlobal::displayInfo("ProxyShape: Could not change prim at path since there was no valid prim at the passed in path");
      return;
    }
    m_compositionHasChanged = true;
    m_changedPath = changePath;
    onPrePrimChanged(m_changedPath, m_variantSwitchedPrims);
  }

  /// \brief  change the status of the composition changed status
  /// \param  hasObjectsChanged
  inline void setHaveObjectsChangedAtPath(const bool hasObjectsChanged)
    { m_compositionHasChanged = hasObjectsChanged; }

  /// \brief  provides access to the selection list on this proxy shape
  /// \return the internal selection list
  SelectionList& selectionList()
    { return m_selectionList; }

  /// \brief  internal method used to correctly schedule changes to the selection list
  /// \param  hasSelectabilityChanged the state
  inline void setChangedSelectionState(const bool hasSelectabilityChanged)
    { m_hasChangedSelection = hasSelectabilityChanged; }

  /// \brief Returns the SelectionDatabase owned by the ProxyShape
  /// \return A SelectableDB owned by the ProxyShape
  AL::usdmaya::SelectabilityDB& selectabilityDB()
    { return m_selectabilityDB; }

  /// \brief Returns the SelectionDatabase owned by the ProxyShape
  /// \return A constant SelectableDB owned by the ProxyShape
  const AL::usdmaya::SelectabilityDB& selectabilityDB() const
    { return const_cast<ProxyShape*>(this)->selectabilityDB(); }

  /// \brief  used to reload the stage after file open
  AL_USDMAYA_PUBLIC
  void loadStage();

  AL_USDMAYA_PUBLIC
  void constructLockPrims();

  /// \brief Translates prims at the specified paths, the operation conducted by the translator depends on
  ///        which list you populate.
  /// \param importPaths paths you wish to import
  /// \param teardownPaths paths you wish to teardown
  /// \param param are params which direct the translation of the prims
  AL_USDMAYA_PUBLIC
  void translatePrimPathsIntoMaya(
      const SdfPathVector& importPaths,
      const SdfPathVector& teardownPaths,
      const fileio::translators::TranslatorParameters& param = fileio::translators::TranslatorParameters());

  /// \brief Translates prims at the specified paths, the operation conducted by the translator depends on
  ///        which list you populate.
  /// \param importPrims array of prims you wish to import
  /// \param teardownPaths paths you wish to teardown
  /// \param param are flags which direct the translation of the prims
  AL_USDMAYA_PUBLIC
  void translatePrimsIntoMaya(
      const AL::usd::utils::UsdPrimVector& importPrims,
      const SdfPathVector& teardownPaths,
      const fileio::translators::TranslatorParameters& param = fileio::translators::TranslatorParameters());

  /// \brief  Breaks a comma separated string up into a SdfPath Vector
  /// \param  paths the comma separated list of paths
  /// \return the separated list of paths
  AL_USDMAYA_PUBLIC
  SdfPathVector getPrimPathsFromCommaJoinedString(const MString &paths) const;

#if defined(WANT_UFE_BUILD)
  /// \brief Get the UFE path of the maya proxy shape
  /// \return An UFE path containing the path to the proxy shape
  AL_USDMAYA_PUBLIC
  Ufe::Path ufePath() const;

  /// \brief Get the UFE path segment of the maya proxy shape
  /// \return An UFE path segment containing the maya path to the proxy shape
  AL_USDMAYA_PUBLIC
  Ufe::PathSegment ufePathSegment() const;
#endif

  /// \brief  Returns the selection mask of the shape
  AL_USDMAYA_PUBLIC
  MSelectionMask getShapeSelectionMask() const override;

private:
  /// \brief  constructs the USD imaging engine for this shape
  void constructGLImagingEngine();

  /// \brief destroys the USD imaging engine for this shape
  void destroyGLImagingEngine();

  static void onSelectionChanged(void* ptr);
  bool removeAllSelectedNodes(SelectionUndoHelper& helper);
  void removeTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason);
  void insertTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason);

  void constructExcludedPrims();
  bool updateLockPrims(const SdfPathSet& lockTransformPrims, const SdfPathSet& lockInheritedPrims,
                       const SdfPathSet& unlockedPrims);
  bool lockTransformAttribute(const SdfPath& path, bool lock);

  MObject makeUsdTransformChain_internal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2 = 0,
      uint32_t* createCount = 0,
      MString* newPath = 0,
      bool pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"),
      bool readAnimatedValues = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues"));

  void removeUsdTransformChain_internal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  MObject makeUsdTransformChain(
      UsdPrim usdPrim,
      const MPlug& outStage,
      const MPlug& outTime,
      const MObject& parentXForm,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2,
      uint32_t* createCount,
      MString* newPath = 0,
      bool pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"),
      bool readAnimatedValues = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues"));

  void makeUsdTransformsInternal(
      const UsdPrim& usdPrim,
      const MObject& parentXForm,
      MDagModifier& modifier,
      TransformReason reason,
      MDGModifier* modifier2,
      bool pushToPrim = MGlobal::optionVarIntValue("AL_usdmaya_pushToPrim"),
      bool readAnimatedValues = MGlobal::optionVarIntValue("AL_usdmaya_readAnimatedValues"));

  void removeUsdTransformsInternal(
      const UsdPrim& usdPrim,
      MDagModifier& modifier,
      TransformReason reason);

  struct TransformReference
  {
    TransformReference(const MObject& node, const TransformReason reason);
    TransformReference(MObject mayaNode, Scope* node, uint32_t r, uint32_t s, uint32_t rc);
    MObject node() const { return m_node; }
    Scope* getTransformNode() const;

    bool decRef(const TransformReason reason);
    void incRef(const TransformReason reason);
    void checkIncRef(const TransformReason reason);
    bool checkRef(const TransformReason reason);

    void printRefCounts() const
    {
      MObjectHandle handle(m_node);
      std::cout << "[valid = " << handle.isValid() << ", alive = " << handle.isAlive() << "] ";
      std::cout
                << m_required << ":"
                << m_selectedTemp << ":"
                << m_selected << ":"
                << int(m_refCount) << std::endl;
    }
    uint16_t selected() const { return m_selected; }
    uint16_t required() const { return m_required; }
    uint16_t refCount() const { return m_refCount; }
    void prepSelect()
      { m_selectedTemp = m_selected; }
  private:
    MObject m_node;
    Scope* m_transform;
    // ref counting values
    struct
    {
      uint16_t m_required;
      uint16_t m_selectedTemp;
      uint16_t m_selected;
      uint16_t m_refCount;
    };
  };

  /// if the USD stage contains a maya reference et-al, then we have a set of *REQUIRED* AL::usdmaya::nodes::Transform nodes.
  /// If we then later create a USD transform node (because we're bringing in all of them, or just a selection of them),
  /// then we must make sure that we don't end up duplicating paths. This map is use to store a LUT of the paths that
  /// must always exist, and never get deleted.
  typedef std::map<SdfPath, TransformReference>  TransformReferenceMap;
  TransformReferenceMap m_requiredPaths;


  /// it is possible to end up with some invalid data in here as a result of a variant switch. When it looks as though a
  /// schema prim is going to change type, in cases where a payload fails to resolve, we can end up with null prims in the
  /// stage. As a result, it's corresponding transform ref can fail to load.
  void cleanupTransformRefs();

  /// insert a new path into the requiredPaths map
  void makeTransformReference(const SdfPath& path, const MObject& node, TransformReason reason);

  /// selection can cause multiple transform chains to be removed. To ensure the ref counts are correctly correlated,
  /// we need to make sure we can remove
  void prepSelect();

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Virtual overrides
  //--------------------------------------------------------------------------------------------------------------------

  void postConstructor() override;
  MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
  bool setInternalValue(const MPlug& plug, const MDataHandle& dataHandle) override;
  bool getInternalValue(const MPlug& plug, MDataHandle& dataHandle) override;
  MStatus setDependentsDirty(const MPlug& plugBeingDirtied, MPlugArray& plugs) override;
  bool isBounded() const override;
  #if MAYA_API_VERSION < 201700
  MPxNode::SchedulingType schedulingType() const override { return kSerialize; }
  #else
  MPxNode::SchedulingType schedulingType() const override { return kSerial; }
  #endif
  MStatus preEvaluation(const MDGContext & context, const MEvaluationNode& evaluationNode) override;
  void CacheEmptyBoundingBox(MBoundingBox&) override;
  UsdTimeCode GetOutputTime(MDataBlock) const override;
  void copyInternalData(MPxNode* srcNode) override;

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Compute methods
  //--------------------------------------------------------------------------------------------------------------------

  // split out compute methods.
  MStatus computeInStageDataCached(const MPlug& plug, MDataBlock& dataBlock);
  MStatus computeOutStageData(const MPlug& plug, MDataBlock& dataBlock);
  MStatus computeOutputTime(const MPlug& plug, MDataBlock& dataBlock, MTime&);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Utils
  //--------------------------------------------------------------------------------------------------------------------

  UsdPrim getUsdPrim(MDataBlock& dataBlock) const;
  SdfPathVector getExcludePrimPaths() const override;
  UsdStagePopulationMask constructStagePopulationMask(const MString &paths) const;

  bool isStageValid() const;
  bool primHasExcludedParent(UsdPrim prim);
  bool initPrim(const uint32_t index, MDGContext& ctx);

  void layerIdChanged(SdfNotice::LayerIdentifierDidChange const& notice, UsdStageWeakPtr const& sender);
  void onObjectsChanged(UsdNotice::ObjectsChanged const&, UsdStageWeakPtr const& sender);
  void variantSelectionListener(SdfNotice::LayersDidChange const& notice);
  void onEditTargetChanged(UsdNotice::StageEditTargetChanged const& notice, UsdStageWeakPtr const& sender);
  void trackEditTargetLayer(LayerManager* layerManager=nullptr);
  void trackAllDirtyLayers(LayerManager* layerManager=nullptr);
  void validateTransforms();
  void onTransactionNotice(AL::usd::transaction::CloseNotice const &notice, const UsdStageWeakPtr& stage);
  void onRedraw() { m_requestedRedraw = false; }

  /// get the stored Translator ID for a Path
  std::string getTranslatorIdForPath(const SdfPath& path) override
    { return m_context->getTranslatorIdForPath(path); }

  bool getTranslatorInfo(const std::string& translatorId, bool& supportsUpdate, bool& requiresParent, bool& importableByDefault) override
    {
      auto translator = m_translatorManufacture.getTranslatorFromId(translatorId);
      if(translator)
      {
        supportsUpdate = translator->supportsUpdate();
        requiresParent = translator->needsTransformParent();
        importableByDefault = translator->importableByDefault();
      }
      return translator != 0;
    }

  /// generate the Translator ID for a Path - this is used for testing only
  std::string generateTranslatorId(UsdPrim prim) override
   { return m_translatorManufacture.generateTranslatorId(prim); }

  bool isPrimDirty(const UsdPrim& prim) override
  {
    const SdfPath path(prim.GetPath());
    auto previous(m_context->getUniqueKeyForPath(path));
    if (!previous)
    {
      return true;
    }
    std::string translatorId = m_translatorManufacture.generateTranslatorId(prim);
    auto translator = m_translatorManufacture.getTranslatorFromId(translatorId);
    auto current(translator->generateUniqueKey(prim));
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg(
        "ProxyShape:isPrimDirty prim='%s' uniqueKey='%lu', previous='%lu'\n",
        path.GetText(), current, previous);
    return !current || current != previous;
  }

private:
  SdfPathVector m_pathsOrdered;
  static std::vector<MObjectHandle> m_unloadedProxyShapes;

  AL::usdmaya::SelectabilityDB m_selectabilityDB;
  HierarchyIterationLogics m_hierarchyIterationLogics;
  HierarchyIterationLogic m_findExcludedPrims;
  SelectionList m_selectionList;
  FindUnselectablePrimsLogic m_findUnselectablePrims;
  SdfPathHashSet m_selectedPaths;
  FindLockedPrimsLogic m_findLockedPrims;
  PrimPathToDagPath m_primPathToDagPath;
  std::vector<SdfPath> m_paths;
  std::vector<UsdPrim> m_prims;
  TfNotice::Key m_objectsChangedNoticeKey;
  TfNotice::Key m_variantChangedNoticeKey;
  TfNotice::Key m_editTargetChanged;
  TfNotice::Key m_transactionNoticeKey;

  MCallbackId m_onSelectionChanged = 0;
  SdfPathVector m_excludedGeometry;
  SdfPathVector m_excludedTaggedGeometry;
  SdfPathSet m_lockTransformPrims;
  SdfPathSet m_lockInheritedPrims;
  SdfPathSet m_currentLockedPrims;
  static MObject m_transformTranslate;
  static MObject m_transformRotate;
  static MObject m_transformScale;
  static MObject m_visibleInReflections;
  static MObject m_visibleInRefractions;
  UsdStageRefPtr m_stage;
  SdfPath m_path;
  fileio::translators::TranslatorContextPtr m_context;
  fileio::translators::TranslatorManufacture m_translatorManufacture;
  SdfPath m_changedPath;
  SdfPathVector m_variantSwitchedPrims;
  SdfLayerHandle m_prevEditTarget;
  Engine* m_engine = 0;

  uint32_t m_engineRefCount = 0;
  bool m_compositionHasChanged = false;
  bool m_pleaseIgnoreSelection = false;
  bool m_hasChangedSelection = false;
  bool m_filePathDirty = false;
  bool m_requestedRedraw = false;
};

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
