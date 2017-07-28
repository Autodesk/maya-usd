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
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/Metadata.h"

#include "maya/MFnDagNode.h"
#include "maya/MPxCommand.h"

#include <set>
#include <algorithm>

// printf debugging
#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cout << X << std::endl;
#else
# define Trace(X)
#endif

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// I have to handle the case where maya commands are issued (e.g. select -cl) that will remove our transform nodes
/// from mayas global selection list (but will have left those nodes behind, and left them in the transform refs
/// within the proxy shape).
/// In those cases, it should just be a case of traversing the selected paths on the proxy shape, determine which
/// paths are no longer in the maya selection list, and then issue a command to AL_usdmaya_ProxyShapeSelect to deselct
/// those nodes. This will ensure that the nodes are nicely removed, and insert an item into the undo stack.
//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onSelectionChanged(void* ptr)
{
  Trace("ProxyShapeSelection::onSelectionChanged " << MGlobal::isUndoing());

  const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
  if(selectionMode)
  {
    ProxyShape* proxy = (ProxyShape*)ptr;
    if(!proxy)
      return;

    if(proxy->m_pleaseIgnoreSelection)
      return;

    if(proxy->selectedPaths().empty())
      return;

    auto stage = proxy->getUsdStage();

    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl);

    std::set<SdfPath> selectedSet;
    std::vector<SdfPath> unselectedSet;
    MFnDagNode fnDag;
    for(uint32_t i = 0; i < sl.length(); ++i)
    {
      MDagPath mayaPath;
      sl.getDagPath(i, mayaPath);

      if(mayaPath.node().hasFn(MFn::kPluginTransformNode))
      {
        fnDag.setObject(mayaPath);

        if(fnDag.typeId() == AL_USDMAYA_TRANSFORM)
        {
          Transform* nodePtr = (Transform*)fnDag.userNode();
          UsdPrim prim = nodePtr->transform()->prim();
          if(prim.GetStage() == stage)
          {
            selectedSet.insert(prim.GetPath());
          }
        }
      }
    }
    for(auto selected : proxy->selectedPaths())
    {
      if(!selectedSet.count(selected))
      {
        Trace("  onSelectionChanged " << selected.GetText());
        unselectedSet.push_back(selected);
      }
    }

    if(!unselectedSet.empty())
    {
      struct compare_length {
        bool operator() (const SdfPath& a, const SdfPath& b) const {
           return a.GetString().size() > b.GetString().size();
        }
      };
      std::sort(unselectedSet.begin(), unselectedSet.end(), compare_length());

      // construct command to unselect the nodes (specifying the internal flag to ensure the selection list is not modified)
      MString command = "AL_usdmaya_ProxyShapeSelect -i -d";
      for(auto removed : unselectedSet)
      {
        command += " -pp \"";
        command += removed.GetText();
        command += "\"";
      }

      fnDag.setObject(proxy->thisMObject());

      command += " \"";
      command += fnDag.name().asChar();
      command += "\"";

      proxy->m_pleaseIgnoreSelection = true;
      MGlobal::executeCommand(command, false, true);
    }
  }
  else
  {
    ProxyShape* proxy = (ProxyShape*)ptr;
    if(!proxy)
      return;

    if(proxy->m_pleaseIgnoreSelection)
      return;

    if(proxy->m_hasChangedSelection)
      return;

    if(proxy->selectedPaths().empty())
      return;

    MSelectionList sl;
    MGlobal::getActiveSelectionList(sl, false);

    bool hasItems = false;
    MString command = "AL_usdmaya_ProxyShapeSelect -i -d";

    for(auto selected : proxy->selectedPaths())
    {
      MObject obj = proxy->findRequiredPath(selected);
      MFnDependencyNode fn(obj);
      if(!sl.hasItem(obj))
      {
        hasItems = true;
        command += " -pp \"";
        command += selected.GetText();
        command += "\"";
      }
    }

    if(hasItems)
    {
      MFnDagNode fnDag(proxy->thisMObject());
      command += " \"";
      command += fnDag.name().asChar();
      command += "\"";
      proxy->m_pleaseIgnoreSelection = true;
      MGlobal::executeCommand(command, false, true);
      proxy->m_pleaseIgnoreSelection = false;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::printRefCounts() const
{
  for(auto it = m_requiredPaths.begin(); it != m_requiredPaths.end(); ++it)
  {
    std::cout << it->first.GetText() << " :- ";
    it->second.printRefCounts();
  }
}

//----------------------------------------------------------------------------------------------------------------------
inline bool ProxyShape::TransformReference::decRef(const TransformReason reason)
{
  Trace("ProxyShapeSelection::TransformReference::decRef " << m_selected << " " << m_refCount << " " << m_required);
  switch(reason)
  {
  case kSelection: assert(m_selected); --m_selected; break;
  case kRequested: assert(m_refCount); --m_refCount; break;
  case kRequired: assert(m_required); --m_required; break;
  default:
    assert(0);
    break;
  }
  return !m_required && !m_selected && !m_refCount;
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::TransformReference::incRef(const TransformReason reason)
{
  Trace("ProxyShapeSelection::TransformReference::incRef " << m_selected << " " << m_refCount << " " << m_required);
  switch(reason)
  {
  case kSelection: ++m_selected; break;
  case kRequested: ++m_refCount; break;
  case kRequired: ++m_required; break;
  default: assert(0); break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::TransformReference::checkIncRef(const TransformReason reason)
{
  Trace("ProxyShapeSelection::TransformReference::checkIncRef " << m_selected << " " << m_refCount << " " << m_required);
  switch(reason)
  {
  case kSelection: ++m_selectedTemp; break;
  default: break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
inline bool ProxyShape::TransformReference::checkRef(const TransformReason reason)
{
  Trace("ProxyShapeSelection::TransformReference::checkRef " << m_selectedTemp << " : " << m_selected << " " << m_refCount << " " << m_required);
  uint32_t sl = m_selected;
  uint32_t rc = m_refCount;
  uint32_t rq = m_required;

  switch(reason)
  {
  case kSelection: assert(m_selected); --sl; --m_selectedTemp; break;
  case kRequested: assert(m_refCount); --rc; break;
  case kRequired: assert(m_required); --rq; break;
  default: assert(0); break;
  }
  return !rq && !m_selectedTemp && !rc;
}

//----------------------------------------------------------------------------------------------------------------------
inline ProxyShape::TransformReference::TransformReference(const MObject& node, const TransformReason reason)
  : m_node(node)
{
  m_required = 0;
  m_selected = 0;
  m_selectedTemp = 0;
  m_refCount = 0;
  MFnDependencyNode fn(node);
  m_transform = (Transform*)fn.userNode();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::makeTransformReference(const SdfPath& path, const MObject& node, TransformReason reason)
{
  Trace("ProxyShapeSelection::makeTransformReference " << path.GetText());
  assert(!isRequiredPath(path));

  SdfPath tempPath = path;
  MDagPath dagPath;
  MStatus status;
  MObjectHandle handle(node);
  if(handle.isAlive() && handle.isValid())
  {
    MFnDagNode fn(node, &status);
    status = fn.getPath(dagPath);
    while(tempPath != SdfPath("/"))
    {
      MObject tempNode = dagPath.node(&status);
      auto existing = m_requiredPaths.find(tempPath);
      if(existing != m_requiredPaths.end())
      {
        existing->second.incRef(reason);
      }
      else
      {
        TransformReference ref(tempNode, reason);
        ref.incRef(reason);
        m_requiredPaths.emplace(tempPath, ref);
      }
      status = dagPath.pop();
      tempPath = tempPath.GetParentPath();
    }
  }
  else
  {
    while(tempPath != SdfPath("/"))
    {
      auto existing = m_requiredPaths.find(tempPath);
      if(existing != m_requiredPaths.end())
      {
        existing->second.incRef(reason);
      }
      else
      {
        MGlobal::displayError("invalid MObject encountered when making transform reference");
      }
      tempPath = tempPath.GetParentPath();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
inline void ProxyShape::prepSelect()
{
  for(auto& iter : m_requiredPaths)
  {
    iter.second.prepSelect();
  }
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain_internal(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason,
    MDGModifier* modifier2,
    uint32_t* createCount,
    MString* resultingPath)
{
  Trace("ProxyShapeSelection::makeUsdTransformChain_internal");

  const MPlug outTimeAttr = outTimePlug();
  const MPlug outStageAttr = outStageDataPlug();

  // makes the assumption that instancing isn't supported.
  MFnDagNode fn(thisMObject());
  const MObject parent = fn.parent(0);
  auto ret = makeUsdTransformChain(usdPrim, outStageAttr, outTimeAttr, parent, modifier, reason, modifier2, createCount, resultingPath);
  return ret;
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason,
    MDGModifier* modifier2,
    uint32_t* createCount)
{
  if(!usdPrim)
  {
    return MObject::kNullObj;
  }

  // special case for selection. Do not allow duplicate paths to be selected.
  if(reason == kSelection)
  {
    if(std::find(m_selectedPaths.begin(), m_selectedPaths.end(), usdPrim.GetPath()) != m_selectedPaths.end())
    {
      TransformReferenceMap::iterator previous = m_requiredPaths.find(usdPrim.GetPath());
      return previous->second.m_node;
    }
    m_selectedPaths.push_back(usdPrim.GetPath());
  }

  MObject newNode = makeUsdTransformChain_internal(usdPrim, modifier, reason, modifier2, createCount);
  insertTransformRefs( { std::pair<SdfPath, MObject>(usdPrim.GetPath(), newNode) }, reason);
  return newNode;
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransformChain(
    UsdPrim usdPrim,
    const MPlug& outStage,
    const MPlug& outTime,
    const MObject& parentXForm,
    MDagModifier& modifier,
    TransformReason reason,
    MDGModifier* modifier2,
    uint32_t* createCount,
    MString* resultingPath)
{
  Trace("ProxyShapeSelection::makeUsdTransformChainB " << usdPrim.GetPath().GetText());

  SdfPath path = usdPrim.GetPath();
  auto iter = m_requiredPaths.find(path);

  // If this path has been found.
  if(iter != m_requiredPaths.end())
  {
    MObject nodeToReturn = iter->second.m_node;
    switch(reason)
    {
    case kSelection:
      {
        while(usdPrim && iter != m_requiredPaths.end())
        {
          iter->second.checkIncRef(reason);

          // grab the parent.
          usdPrim = usdPrim.GetParent();

          // if valid, grab reference to path
          if(usdPrim)
          {
            iter = m_requiredPaths.find(usdPrim.GetPath());
          }
        };
      }
      break;

    case kRequested:
      {
        while(usdPrim && iter != m_requiredPaths.end())
        {
          // grab the parent.
          usdPrim = usdPrim.GetParent();

          // if valid, grab reference to path
          if(usdPrim)
          {
            iter = m_requiredPaths.find(usdPrim.GetPath());
          }
        };
      }
      break;

    case kRequired:
      {
        while(usdPrim && iter != m_requiredPaths.end() && !iter->second.required())
        {
          iter->second.checkIncRef(reason);

          // grab the parent.
          usdPrim = usdPrim.GetParent();

          // if valid, grab reference to path
          if(usdPrim)
          {
            iter = m_requiredPaths.find(usdPrim.GetPath());
          }
        };
      }
      break;
    }
    if(resultingPath)
    {
      MFnDagNode fn;
      MDagPath path;
      fn.getPath(path);
      *resultingPath = path.fullPathName();
    }
    // return the lowest point on the found chain.
    return nodeToReturn;
  }

  MObject parentPath;
  // descend into the parent first
  if(path.GetPathElementCount() > 1)
  {
    // if there is a parent to this node, continue building the chain.
    parentPath = makeUsdTransformChain(usdPrim.GetParent(), outStage, outTime, parentXForm, modifier, reason, modifier2, createCount);
  }

  // if we've hit the top of the chain, make sure we get the correct parent
  if(parentPath == MObject::kNullObj)
  {
    parentPath = parentXForm;
  }

  if(createCount) (*createCount)++;

  MFnDagNode fn;
  MObject parentNode = parentPath;

  bool isTransform = usdPrim.HasAttribute(TfToken("xformOpOrder"));
  bool isUsdTransform = true;
  MObject node;
  std::string transformType;
  bool hasMetadata = usdPrim.GetMetadata(Metadata::transformType, &transformType);
  if(hasMetadata && !transformType.empty())
  {
    node = modifier.createNode(convert(transformType), parentNode);
    isTransform = false;
    isUsdTransform = false;
    Trace("ProxyShape::makeUsdTransformChain created transformType = " << transformType << " name = " << usdPrim.GetName().GetString())
  }
  else
  {
    node = modifier.createNode(Transform::kTypeId, parentNode);
    Trace("ProxyShape::makeUsdTransformChain created transformType = AL_usdmaya_Transform name = "<<usdPrim.GetName().GetString())
  }

  fn.setObject(node);
  fn.setName(convert(usdPrim.GetName().GetString()));

  //Retrieve the proxy shapes transform path which will be used in the UsdPrim->MayaNode mapping in the case where there is delayed node creation.
  MFnDagNode shapeFn(thisMObject());
  const MObject shapeParent = shapeFn.parent(0);
  MDagPath mayaPath;
  MDagPath::getAPathTo(shapeParent, mayaPath);
  if(resultingPath)
    *resultingPath = mapUsdPrimToMayaNode(usdPrim, node, &mayaPath);
  else
    mapUsdPrimToMayaNode(usdPrim, node, &mayaPath);

  if(isUsdTransform)
  {
    Transform* ptrNode = (Transform*)fn.userNode();
    MPlug inStageData = ptrNode->inStageDataPlug();
    MPlug inTime = ptrNode->timePlug();

    modifier.connect(outStage, inStageData);
    modifier.connect(outTime, inTime);

    if(modifier2)
    {
      modifier2->newPlugValueBool(MPlug(node, Transform::pushToPrim()), true);
    }

    if(!isTransform)
    {
      MPlug(node, MPxTransform::translate).setLocked(true);
      MPlug(node, MPxTransform::rotate).setLocked(true);
      MPlug(node, MPxTransform::scale).setLocked(true);
      MPlug(node, MPxTransform::transMinusRotatePivot).setLocked(true);
      MPlug(node, MPxTransform::rotateAxis).setLocked(true);
      MPlug(node, MPxTransform::scalePivotTranslate).setLocked(true);
      MPlug(node, MPxTransform::scalePivot).setLocked(true);
      MPlug(node, MPxTransform::rotatePivotTranslate).setLocked(true);
      MPlug(node, MPxTransform::rotatePivot).setLocked(true);
      MPlug(node, MPxTransform::shearXY).setLocked(true);
      MPlug(node, MPxTransform::shearXZ).setLocked(true);
      MPlug(node, MPxTransform::shearYZ).setLocked(true);
    }

    // set the primitive path
    fileio::translators::DgNodeTranslator::setString(node, Transform::primPath(), path.GetText());
  }
  TransformReference ref(node, reason);
  ref.checkIncRef(reason);
  m_requiredPaths.emplace(path, ref);
  return node;
}

//----------------------------------------------------------------------------------------------------------------------
MObject ProxyShape::makeUsdTransforms(const UsdPrim& usdPrim, MDagModifier& modifier, TransformReason reason, MDGModifier* modifier2)
{
  Trace("ProxyShapeSelection::makeUsdTransforms");

  // Ok, so let's go wondering up the transform chain making sure we have all of those transforms created.
  MObject node = makeUsdTransformChain(usdPrim, modifier, reason, modifier2, 0);

  // we only need child transforms if they have been requested
  if(reason == kRequested)
  {
    makeUsdTransformsInternal(usdPrim, node, modifier, reason, modifier2);
  }

  return node;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::makeUsdTransformsInternal(const UsdPrim& usdPrim, const MObject& parentNode, MDagModifier& modifier, TransformReason reason, MDGModifier* modifier2)
{
  Trace("ProxyShapeSelection::makeUsdTransformsInternal");
  MFnDagNode fn;

  MPlug outStageAttr = outStageDataPlug();
  MPlug outTimeAttr = outTimePlug();

  for(auto it = usdPrim.GetChildren().begin(), e = usdPrim.GetChildren().end(); it != e; ++it)
  {
    UsdPrim prim = *it;
    /// must always exist, and never get deleted.
    auto check = m_requiredPaths.find(prim.GetPath());
    if(check == m_requiredPaths.end())
    {
      UsdPrim prim = *it;
      MObject node = modifier.createNode(Transform::kTypeId, parentNode);
      fn.setObject(node);
      fn.setName(convert(prim.GetName().GetString()));
      Transform* ptrNode = (Transform*)fn.userNode();
      MPlug inStageData = ptrNode->inStageDataPlug();
      MPlug inTime = ptrNode->timePlug();
      modifier.connect(outStageAttr, inStageData);
      modifier.connect(outTimeAttr, inTime);

      if(modifier2)
      {
        modifier2->newPlugValueBool(MPlug(node, Transform::pushToPrim()), true);
      }

      // set the primitive path
      fileio::translators::DgNodeTranslator::setString(node, Transform::primPath(), prim.GetPath().GetText());
      TransformReference transformRef(node, reason);
      transformRef.incRef(reason);
      m_requiredPaths.emplace(prim.GetPath(), transformRef);

      makeUsdTransformsInternal(prim, node, modifier, reason, modifier2);
    }
    else
    {
      makeUsdTransformsInternal(prim, check->second.m_node, modifier, reason, modifier2);
    }
  }
}


//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain_internal(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason)
{
  Trace("ProxyShapeSelection::removeUsdTransformChain");
  UsdPrim parentPrim = usdPrim;
  MObject parentTM = MObject::kNullObj;
  MObject object = MObject::kNullObj;
  while(parentPrim)
  {
    auto it = m_requiredPaths.find(parentPrim.GetPath());
    if(it == m_requiredPaths.end())
    {
      return;
    }

    if(it->second.checkRef(reason))
    {
      MObject object = it->second.m_node;
      if(object != MObject::kNullObj)
      {
        modifier.reparentNode(object);
        modifier.deleteNode(object);
      }
    }

    parentPrim = parentPrim.GetParent();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain(
    const SdfPath& path,
    MDagModifier& modifier,
    TransformReason reason)
{
  Trace("ProxyShapeSelection::removeUsdTransformChain");
  SdfPath parentPrim = path;
  MObject parentTM = MObject::kNullObj;
  MObject object = MObject::kNullObj;
  while(!parentPrim.IsEmpty())
  {
    auto it = m_requiredPaths.find(parentPrim);
    if(it == m_requiredPaths.end())
    {
      return;
    }
    if(it->second.decRef(reason))
    {
      MObject object = it->second.m_node;
      if(object != MObject::kNullObj)
      {
        modifier.reparentNode(object);
        modifier.deleteNode(object);
      }
      m_requiredPaths.erase(it);
    }

    parentPrim = parentPrim.GetParentPath();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformChain(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason)
{
  Trace("ProxyShapeSelection::removeUsdTransformChain");
  if(!usdPrim)
  {
    return;
  }

  if(reason == kSelection)
  {
    auto selectedPath = std::find(m_selectedPaths.begin(), m_selectedPaths.end(), usdPrim.GetPath());
    if(selectedPath != m_selectedPaths.end())
    {
      m_selectedPaths.erase(selectedPath);
    }
    else
    {
      return;
    }
  }

  UsdPrim parentPrim = usdPrim;
  MObject parentTM = MObject::kNullObj;
  MObject object = MObject::kNullObj;
  while(parentPrim)
  {
    auto it = m_requiredPaths.find(parentPrim.GetPath());
    if(it == m_requiredPaths.end())
    {
      return;
    }

    if(it->second.decRef(reason))
    {
      MObject object = it->second.m_node;
      if(object != MObject::kNullObj)
      {
        modifier.reparentNode(object);
        modifier.deleteNode(object);
      }

      m_requiredPaths.erase(it);
    }

    parentPrim = parentPrim.GetParent();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransformsInternal(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason)
{
  Trace("ProxyShapeSelection::removeUsdTransformsInternal " << usdPrim.GetPath().GetText());
  // can we find the prim in the current set?
  auto it = m_requiredPaths.find(usdPrim.GetPath());
  if(it == m_requiredPaths.end())
  {
    return;
  }

  // first go remove the children
  for(auto iter = usdPrim.GetChildren().begin(), end = usdPrim.GetChildren().end(); iter != end; ++iter)
  {
    removeUsdTransformsInternal(*iter, modifier, ProxyShape::kRequested);
  }

  if(it->second.decRef(reason))
  {
    // work around for Maya's love of deleting the parent transforms of custom transform nodes :(
    MFnTransform tm;
    tm.create();
    tm.addChild(it->second.m_node);
    modifier.deleteNode(it->second.m_node);
    m_requiredPaths.erase(it);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeUsdTransforms(
    const UsdPrim& usdPrim,
    MDagModifier& modifier,
    TransformReason reason)
{
  Trace("ProxyShapeSelection::removeUsdTransforms");

  // can we find the prim in the current set?
  auto it = m_requiredPaths.find(usdPrim.GetPath());
  if(it == m_requiredPaths.end())
  {
    return;
  }

  // no need to iterate through children if we are requesting a shape
  if(reason == kRequested)
  {
    // first go remove the children
    for(auto it = usdPrim.GetChildren().begin(), end = usdPrim.GetChildren().end(); it != end; ++it)
    {
      removeUsdTransformsInternal(*it, modifier, ProxyShape::kRequested);
    }
  }

  // finally walk back up the chain and do magic. I'm not sure I want to do this?
  removeUsdTransformChain(usdPrim, modifier, reason);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::insertTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason)
{
  for(auto iter : removedRefs)
  {
    MObjectHandle handle(iter.second);
    makeTransformReference(iter.first, iter.second, reason);
  }
}

//----------------------------------------------------------------------------------------------------------------------
SelectionUndoHelper::SelectionUndoHelper(nodes::ProxyShape* proxy, SdfPathVector paths, MGlobal::ListAdjustment mode, bool internal)
  : m_proxy(proxy), m_paths(paths), m_mode(mode), m_modifier1(), m_modifier2(), m_insertedRefs(), m_removedRefs(), m_internal(internal)
{
}

//----------------------------------------------------------------------------------------------------------------------
void SelectionUndoHelper::doIt()
{
  Trace("ProxyShapeSelection::SelectionUndoHelper::doIt " << m_insertedRefs.size() << " " << m_removedRefs.size());
  m_proxy->m_pleaseIgnoreSelection = true;
  m_modifier1.doIt();
  m_modifier2.doIt();
  m_proxy->insertTransformRefs(m_insertedRefs, nodes::ProxyShape::kSelection);
  m_proxy->removeTransformRefs(m_removedRefs, nodes::ProxyShape::kSelection);
  m_proxy->selectedPaths() = m_paths;
  if(!m_internal)
  {
    MGlobal::setActiveSelectionList(m_newSelection, MGlobal::kReplaceList);
  }
  m_proxy->m_pleaseIgnoreSelection = false;
}

//----------------------------------------------------------------------------------------------------------------------
void SelectionUndoHelper::undoIt()
{
  Trace("ProxyShapeSelection::SelectionUndoHelper::undoIt " << m_insertedRefs.size() << " " << m_removedRefs.size());
  m_proxy->m_pleaseIgnoreSelection = true;
  m_modifier2.undoIt();
  m_modifier1.undoIt();
  m_proxy->insertTransformRefs(m_removedRefs, nodes::ProxyShape::kSelection);
  m_proxy->removeTransformRefs(m_insertedRefs, nodes::ProxyShape::kSelection);
  m_proxy->selectedPaths() = m_previousPaths;
  if(!m_internal)
  {
    MGlobal::setActiveSelectionList(m_previousSelection, MGlobal::kReplaceList);
  }
  m_proxy->m_pleaseIgnoreSelection = false;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::removeTransformRefs(const std::vector<std::pair<SdfPath, MObject>>& removedRefs, TransformReason reason)
{
  Trace("ProxyShapeSelection::removeTransformRefs " << removedRefs.size());
  for(auto iter : removedRefs)
  {
    UsdPrim parentPrim = m_stage->GetPrimAtPath(iter.first);
    while(parentPrim)
    {
      auto it = m_requiredPaths.find(parentPrim.GetPath());
      if(it != m_requiredPaths.end())
      {
        if(it->second.decRef(reason))
        {
          m_requiredPaths.erase(it);
        }
      }

      parentPrim = parentPrim.GetParent();
      if(parentPrim.GetPath() == SdfPath("/"))
      {
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::removeAllSelectedNodes(SelectionUndoHelper& helper)
{
  Trace("ProxyShapeSelection::removeAllSelectedNodes " << m_selectedPaths.size());

  std::vector<TransformReferenceMap::iterator> toRemove;

  auto it = m_requiredPaths.begin();
  auto end = m_requiredPaths.end();
  for(; it != end; ++it)
  {
    // decrement the ref count. If this comes back as 'please remove'
    if(it->second.checkRef(ProxyShape::kSelection))
    {
      // add it to the list of things to kill
      toRemove.push_back(it);
    }
  }

  if(toRemove.size() > 1)
  {
    // sort the array of iterators so that the transforms with the longest path appear first. Those with shorter
    // paths will appear at the end. This is to ensure the child nodes are deleted before their parents.
    struct compare_length {
      bool operator() (const TransformReferenceMap::iterator a, const TransformReferenceMap::iterator b) const {
         return a->first.GetString().size() > b->first.GetString().size();
      }
    };
    std::sort(toRemove.begin(), toRemove.end(), compare_length());
  }

  if(!toRemove.empty())
  {
    std::vector<MObjectHandle> tempNodes;

    // now go and delete all of the nodes in order
    for(auto value = toRemove.begin(), e = toRemove.end(); value != e; ++value)
    {
      // reparent the custom transform under world prior to deleting
      MObject temp = (*value)->second.m_node;
      helper.m_modifier1.reparentNode(temp);

      // now we can delete (without accidentally nuking all parent transforms in the chain)
      helper.m_modifier1.deleteNode(temp);

      SdfPathVector& paths = selectedPaths();
      for(auto iter = paths.begin(), end = paths.end(); iter != end; ++iter)
      {
        if(*iter ==  (*value)->first)
        {
          helper.m_removedRefs.emplace_back((*value)->first, temp);
          paths.erase(iter);
          break;
        }
      }
    }
    m_selectedPaths.clear();

    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::doSelect(SelectionUndoHelper& helper)
{
  Trace("ProxyShapeSelection::doSelect");
  auto stage = m_stage;
  if(!stage)
    return false;

  m_pleaseIgnoreSelection = true;
  prepSelect();

  MGlobal::getActiveSelectionList(helper.m_previousSelection);

  helper.m_previousPaths = selectedPaths();
  if(MGlobal::kReplaceList == helper.m_mode)
  {
    if(helper.m_paths.empty())
    {
      helper.m_mode = MGlobal::kRemoveFromList;
      helper.m_paths = selectedPaths();
    }
  }
  else
  {
    helper.m_newSelection = helper.m_previousSelection;
  }
  MStringArray newlySelectedPaths;

  switch(helper.m_mode)
  {
  case MGlobal::kReplaceList:
    {
      std::vector<SdfPath> keepPrims;
      std::vector<UsdPrim> insertPrims;
      for(auto path : helper.m_paths)
      {
        bool alreadySelected = false;
        for(auto it : m_selectedPaths)
        {
          if(path == it)
          {
            alreadySelected = true;
            break;
          }
        }

        auto prim = stage->GetPrimAtPath(path);
        if(prim)
        {
          if(!alreadySelected)
            insertPrims.push_back(prim);
          else
            keepPrims.push_back(path);
        }
      }

      if(keepPrims.empty() && insertPrims.empty())
      {
        m_pleaseIgnoreSelection = false;
        return false;
      }

      std::sort(keepPrims.begin(), keepPrims.end());

      m_selectedPaths.clear();

      uint32_t hasNodesToCreate = 0;
      for(auto prim : insertPrims)
      {
        m_selectedPaths.push_back(prim.GetPath());
        MString pathName;
        MObject object = makeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection, &helper.m_modifier2, &hasNodesToCreate, &pathName);
        newlySelectedPaths.append(pathName);
        helper.m_newSelection.add(object, true);
        helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
      }

      for(auto iter : helper.m_previousPaths)
      {
        auto temp = m_requiredPaths.find(iter);
        MObject object = temp->second.m_node;
        if(!std::binary_search(keepPrims.begin(), keepPrims.end(), iter))
        {
          auto prim = stage->GetPrimAtPath(iter);
          removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
          helper.m_removedRefs.emplace_back(iter, object);
        }
        else
        {
          helper.m_newSelection.add(object, true);
          m_selectedPaths.push_back(iter);
        }
      }

      helper.m_paths = m_selectedPaths;
    }
    break;

  case MGlobal::kAddToHeadOfList:
  case MGlobal::kAddToList:
    {
      std::vector<UsdPrim> prims;
      for(auto path : helper.m_paths)
      {
        bool alreadySelected = false;
        for(auto it : m_selectedPaths)
        {
          if(path == it)
          {
            alreadySelected = true;
            break;
          }
        }

        if(!alreadySelected)
        {
          auto prim = stage->GetPrimAtPath(path);
          if(prim)
          {
            prims.push_back(prim);
          }
        }
      }

      helper.m_paths.insert(helper.m_paths.end(), helper.m_previousPaths.begin(), helper.m_previousPaths.end());

      uint32_t hasNodesToCreate = 0;
      for(auto prim : prims)
      {
        m_selectedPaths.push_back(prim.GetPath());
        MString pathName;
        MObject object = makeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection, &helper.m_modifier2, &hasNodesToCreate, &pathName);
        newlySelectedPaths.append(pathName);
        helper.m_newSelection.add(object, true);
        helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
      }
    }
    break;

  case MGlobal::kRemoveFromList:
    {
      std::vector<UsdPrim> prims;
      for(auto path : helper.m_paths)
      {
        bool alreadySelected = false;
        for(auto it : m_selectedPaths)
        {
          if(path == it)
          {
            alreadySelected = true;
            break;
          }
        }

        if(alreadySelected)
        {
          auto prim = stage->GetPrimAtPath(path);
          if(prim)
          {
            prims.push_back(prim);
          }
        }
      }

      if(prims.empty())
      {
        m_pleaseIgnoreSelection = false;
        return false;
      }

      if(!prims.empty())
      {
        for(auto prim : prims)
        {
          auto temp = m_requiredPaths.find(prim.GetPath());
          MObject object = temp->second.m_node;

          m_selectedPaths.erase(std::find(m_selectedPaths.begin(), m_selectedPaths.end(), prim.GetPath()));

          removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
          for(int i = 0; i < helper.m_newSelection.length(); ++i)
          {
            MObject obj;
            helper.m_newSelection.getDependNode(i, object);

            if(object == obj)
            {
              helper.m_newSelection.remove(i);
              break;
            }
          }
          helper.m_paths = m_selectedPaths;
          helper.m_removedRefs.emplace_back(prim.GetPath(), object);
        }
      }
      else
      {
        helper.m_paths.clear();
      }
    }
    break;

  case MGlobal::kXORWithList:
    {
      std::vector<UsdPrim> removePrims;
      std::vector<UsdPrim> insertPrims;
      for(auto path : helper.m_paths)
      {
        bool alreadySelected = false;
        for(auto it : m_selectedPaths)
        {
          if(path == it)
          {
            alreadySelected = true;
            break;
          }
        }

        auto prim = stage->GetPrimAtPath(path);
        if(prim)
        {
          if(alreadySelected)
            removePrims.push_back(prim);
          else
            insertPrims.push_back(prim);
        }
      }

      if(removePrims.empty() && insertPrims.empty())
      {
        m_pleaseIgnoreSelection = false;
        return false;
      }

      for(auto prim : removePrims)
      {
        auto temp = m_requiredPaths.find(prim.GetPath());
        MObject object = temp->second.m_node;

        m_selectedPaths.erase(std::find(m_selectedPaths.begin(), m_selectedPaths.end(), prim.GetPath()));

        removeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection);
        for(int i = 0; i < helper.m_newSelection.length(); ++i)
        {
          MObject obj;
          helper.m_newSelection.getDependNode(i, object);

          if(object == obj)
          {
            helper.m_newSelection.remove(i);
            break;
          }
        }
        helper.m_removedRefs.emplace_back(prim.GetPath(), object);
      }

      uint32_t hasNodesToCreate = 0;
      for(auto prim : insertPrims)
      {
        m_selectedPaths.push_back(prim.GetPath());
        MString pathName;
        MObject object = makeUsdTransformChain_internal(prim, helper.m_modifier1, ProxyShape::kSelection, &helper.m_modifier2, &hasNodesToCreate, &pathName);
        newlySelectedPaths.append(pathName);
        helper.m_newSelection.add(object, true);
        helper.m_insertedRefs.emplace_back(prim.GetPath(), object);
      }
      helper.m_paths = m_selectedPaths;
    }
    break;
  }

  if(newlySelectedPaths.length())
  {
    MPxCommand::setResult(newlySelectedPaths);
  }

  m_pleaseIgnoreSelection = false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
