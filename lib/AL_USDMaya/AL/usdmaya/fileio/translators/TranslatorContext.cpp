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
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/DebugCodes.h"
#include "maya/MSelectionList.h"
#include "maya/MFnDagNode.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {


//----------------------------------------------------------------------------------------------------------------------
TranslatorContext::~TranslatorContext()
{
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr TranslatorContext::getUsdStage() const
{
  return m_proxyShape->getUsdStage();
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::validatePrims()
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::validatePrims ** VALIDATE PRIMS **\n");
  for(auto it : m_primMapping)
  {
    if(it.objectHandle().isValid() && it.objectHandle().isAlive())
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::validatePrims ** VALID HANDLE DETECTED %s **\n", it.path().GetText());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getTransform(const SdfPath& path, MObjectHandle& object)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getTransform %s\n", path.GetText());
  auto it = find(path);
  if(it != m_primMapping.end())
  {
    if(!it->objectHandle().isValid())
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getTransform - invalid handle\n");
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
  auto stage = m_proxyShape->getUsdStage();
  for(auto it = m_primMapping.begin(); it != m_primMapping.end(); )
  {
    SdfPath path(it->path());
    UsdPrim prim = stage->GetPrimAtPath(path);
    if(!prim)
    {
      m_primMapping.erase(it++);
    }
    else
    if(it->type() != prim.GetTypeName())
    {
      it->type() = prim.GetTypeName();
      ++it;
    }
    else
    {
      ++it;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MTypeId typeId)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject %s\n", path.GetText());
  auto it = find(path);
  if(it != m_primMapping.end())
  {
    const MTypeId zero(0);
    if(zero != typeId)
    {
      for(auto temp : it->createdNodes())
      {
        MFnDependencyNode fn(temp.object());
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject getting %s\n", fn.typeName().asChar());
        if(fn.typeId() == typeId)
        {
          object = temp;
          if(!object.isAlive())
            MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
          if(!object.isValid())
            MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
          return true;
        }
      }
    }
    else
    {
      if(!it->createdNodes().empty())
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject getting anything %s\n", path.GetString().c_str());
        object = it->createdNodes()[0];

        if(!object.isAlive())
          MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
        if(!object.isValid())
          MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getMObject(const SdfPath& path, MObjectHandle& object, MFn::Type type)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject: %s\n", path.GetText());
  auto it = find(path);
  if(it != m_primMapping.end())
  {
    const MTypeId zero(0);
    if(MFn::kInvalid != type)
    {
      for(auto temp : it->createdNodes())
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject getting: %s\n", temp.object().apiTypeStr());
        if(temp.object().apiType() == type)
        {
          object = temp;
          if(!object.isAlive())
            MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
          if(!object.isValid())
            MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
          return true;
        }
      }
    }
    else
    {
      if(!it->createdNodes().empty())
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::getMObject getting anything: %s\n", path.GetString().c_str());
        object = it->createdNodes()[0];

        if(!object.isAlive())
          MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not alive"));
        if(!object.isValid())
          MGlobal::displayError(MString("VALIDATION: ") + path.GetText() + MString(" is not valid"));
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
  if(it != m_primMapping.end())
  {
    returned = it->createdNodes();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::registerItem(const UsdPrim& prim, MObjectHandle object)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::registerItem adding entry %s[%s]\n", prim.GetPath().GetText(), object.object().apiTypeStr());
  auto iter = findLocation(prim.GetPath());
  if(iter == m_primMapping.end() || iter->path() != prim.GetPath())
  {
    iter = m_primMapping.insert(iter, PrimLookup(prim.GetPath(), prim.GetTypeName(), object.object()));
  }

  if(object.object() == MObject::kNullObj)
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::registerItem primPath=%s primType=%s to null MObject\n", prim.GetPath().GetText(), iter->type().GetText());
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::registerItem primPath=%s primType=%s to MObject type %s\n", prim.GetPath().GetText(), iter->type().GetText(), object.object().apiTypeStr());
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::insertItem(const UsdPrim& prim, MObjectHandle object)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::insertItem adding entry %s[%s]\n", prim.GetPath().GetText(), object.object().apiTypeStr());
  auto iter = findLocation(prim.GetPath());
  if(iter == m_primMapping.end() || iter->path() != prim.GetPath())
  {
    iter = m_primMapping.insert(iter, PrimLookup(prim.GetPath(), prim.GetTypeName(), object.object()));
  }
  iter->createdNodes().push_back(object);

  if(object.object() == MObject::kNullObj)
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::insertItem primPath=%s primType=%s to null MObject\n", prim.GetPath().GetText(), iter->type().GetText());
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::insertItem primPath=%s primType=%s to MObject type %s\n", prim.GetPath().GetText(), iter->type().GetText(), object.object().apiTypeStr());
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::removeItems(const SdfPath& path)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeItems primPath=%s\n", path.GetText());
  auto it = find(path);
  if(it != m_primMapping.end() && it->path() == path)
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeItems path=%s\n", it->path().GetText());
    MDGModifier modifier1;
    MDagModifier modifier2;
    MStatus status;
    bool hasDagNodes = false;
    bool hasDependNodes = false;

    auto& nodes = it->createdNodes();
    for(std::size_t j = 0, n = nodes.size(); j < n; ++j)
    {
      if(nodes[j].isAlive() && nodes[j].isValid())
      {
        MObject obj = nodes[j].object();
        MFnDependencyNode fn(obj);
        if(obj.hasFn(MFn::kTransform))
        {
          hasDagNodes = true;
          modifier2.reparentNode(obj);
          status = modifier2.deleteNode(obj);
        }
        else
        if(obj.hasFn(MFn::kDagNode))
        {
          MObject temp = fn.create("transform");
          hasDagNodes = true;
          modifier2.reparentNode(obj, temp);
          status = modifier2.deleteNode(obj);
          status = modifier2.deleteNode(temp);
        }
        else
        {
          hasDependNodes = true;
          status = modifier1.deleteNode(obj);
          AL_MAYA_CHECK_ERROR2(status, MString("failed to delete node "));
        }
      }
      else
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Invalid MObject was registered with the path \"%s\"\n", path.GetText());
      }
    }
    nodes.clear();

    if(hasDependNodes)
    {
      status = modifier1.doIt();
      AL_MAYA_CHECK_ERROR2(status, "failed to delete node");
    }
    if(hasDagNodes)
    {
      status = modifier2.doIt();
      AL_MAYA_CHECK_ERROR2(status, "failed to delete node");
    }
    m_primMapping.erase(it);
  }
  validatePrims();
}

//----------------------------------------------------------------------------------------------------------------------
MString getNodeName(MObject obj)
{
  if(obj.hasFn(MFn::kDagNode))
  {
    MFnDagNode fn(obj);
    MDagPath path;
    fn.getPath(path);
    return path.fullPathName();
  }
  MFnDependencyNode fn(obj);
  return fn.name();
}

//----------------------------------------------------------------------------------------------------------------------
MString TranslatorContext::serialise() const
{
  std::ostringstream oss;
  for(auto it : m_primMapping)
  {
    oss << it.path() << "=" << it.type().GetText() << ",";
    oss << getNodeName(it.object());
    for(uint32_t i = 0; i < it.createdNodes().size(); ++i)
    {
      oss << "," << getNodeName(it.createdNodes()[i].object());
    }
    oss << ";";
  }
  return MString(oss.str().c_str());
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::deserialise(const MString& string)
{
  MStringArray strings;
  string.split(';', strings);

  for(uint32_t i = 0; i < strings.length(); ++i)
  {
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

    PrimLookup lookup(SdfPath(strings2[0].asChar()), TfToken(strings3[0].asChar()), obj);

    for(uint32_t j = 2; j < strings3.length(); ++j)
    {
      MSelectionList sl;
      sl.add(strings3[j].asChar());
      MObject obj;
      sl.getDependNode(0, obj);
      lookup.createdNodes().push_back(obj);
    }

    m_primMapping.push_back(lookup);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::preRemoveEntry(const SdfPath& primPath, SdfPathVector& itemsToRemove, bool callPreUnload)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::preRemoveEntry primPath=%s\n", primPath.GetText());
  PrimLookups::iterator end = m_primMapping.end();
  PrimLookups::iterator range_begin = std::lower_bound(m_primMapping.begin(), end, primPath, value_compare());
  PrimLookups::iterator range_end = range_begin;
  const std::string& pathBeingRemoved = primPath.GetString();
  const std::size_t length = pathBeingRemoved.size();
  for(; range_end != end; ++range_end)
  {
    // due to the joys of sorting, any child prims of this prim being destroyed should appear next to each
    // other (one would assume); So if compare does not find a match (the value is something other than zero),
    // we are no longer in the same prim root

    const char* const parentPath = pathBeingRemoved.c_str();
    const char* const childPath = range_end->path().GetText();

    if(strncmp(parentPath, childPath, length))
    {
      break;
    }
  }

  auto stage = m_proxyShape->getUsdStage();

  // run the preTearDown stage on each prim. We will walk over the prims in the reverse order here (which will guarentee
  // the the itemsToRemove will be ordered such that the child prims will be destroyed before their parents).
  auto iter = range_end;
  itemsToRemove.reserve(range_end - range_begin);
  while(iter != range_begin)
  {
    --iter;
    PrimLookup& node = *iter;
    itemsToRemove.push_back(node.path());
    auto prim = stage->GetPrimAtPath(node.path());
    if(prim && callPreUnload)
    {
      preUnloadPrim(prim, node.object());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::removeEntries(const SdfPathVector& itemsToRemove)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries\n");
  auto stage = m_proxyShape->getUsdStage();

  PrimLookups::iterator begin = m_primMapping.begin();
  PrimLookups::iterator end = m_primMapping.end();

  SdfPathVector pathsToErase;

  // so now we need to unload the prims from (range_end - 1) to temp (in that order, otherwise we'll nuke parents before children)
  auto iter = itemsToRemove.begin();
  while(iter != itemsToRemove.end())
  {
    auto path = *iter;
    PrimLookups::iterator node = std::lower_bound(begin, end, path, value_compare());

    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries checking %s\n", iter->GetText());
    auto prim = stage->GetPrimAtPath(*iter);
    TfToken type = getTypeForPath(path);

    // if the prim is no longer there, let's kill it
    if(!prim)
    {
      fileio::translators::TranslatorRefPtr translator = m_proxyShape->translatorManufacture().get(type);
      if(translator)
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries %s prim is not valid anymore will be tear down\n", path.GetText());
        unloadPrim(path, node->object());
        pathsToErase.push_back(*iter);
      }
    }
    else
    {
      // so we have the prim, let's check the type to see whether that has changed
      TfToken primType = prim.GetTypeName();
      if(type != primType)
      {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries %s prim is valid but changed type: it will be tear down\n", path.GetText());
        // type has changed, update the prim type and unload the old prim
        unloadPrim(path, node->object());
        pathsToErase.push_back(path);
      }
      else
      {
        fileio::translators::TranslatorRefPtr translator = m_proxyShape->translatorManufacture().get(type);
        if(translator)
        {
          if(!translator->supportsInactive() || !translator->supportsUpdate())
          {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries %s prim is valid but doesnt support inactive or update so will be tear down\n", path.GetText());
            unloadPrim(path, node->object());
            pathsToErase.push_back(*iter);
          }
        }
        else
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries %s prim is valid but no translator found update so will be tear down\n", path.GetText());
          unloadPrim(path, node->object());
          pathsToErase.push_back(*iter);
        }
      }
    }
    ++iter;
  }

  if(!pathsToErase.empty())
  {
    MDagModifier modifier;

    // remove the prims that died
    for(auto path : pathsToErase)
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::removeEntries primPath=%s\n", path.GetText());
      PrimLookups::iterator node = std::lower_bound(m_primMapping.begin(), m_primMapping.end(), path, value_compare());

      // The item might already have been removed by a translator...
      if(node != m_primMapping.end() && node->path() == path)
      {
        // remove nodes from map
        m_primMapping.erase(node);
      }
      
      m_proxyShape->removeUsdTransformChain(path, modifier, nodes::ProxyShape::kRequired);
    }

    modifier.doIt();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::preUnloadPrim(UsdPrim& prim, const MObject& primObj)
{
  assert(m_proxyShape);
  auto stage = m_proxyShape->getUsdStage();
  if(stage)
  {
    TfToken type = m_proxyShape->context()->getTypeForPath(prim.GetPath());

    fileio::translators::TranslatorRefPtr translator = m_proxyShape->translatorManufacture().get(type);
    if(translator)
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::preUnloadPrim [preTearDown] prim=%s\n", prim.GetPath().GetText());
      translator->preTearDown(prim);
    }
    else
    {
      MGlobal::displayError(MString("could not find usd translator plugin instance for prim: ") + prim.GetPath().GetText());
    }
  }
  else
  {
    MGlobal::displayError(MString("Could not unload prim: \"") + prim.GetPath().GetText() + MString("\", the stage is invalid"));
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::unloadPrim(const SdfPath& path, const MObject& primObj)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::unloadPrim\n");
  assert(m_proxyShape);
  auto stage = m_proxyShape->getUsdStage();
  if(stage)
  {
    MDagModifier modifier;
    TfToken type = m_proxyShape->context()->getTypeForPath(path);

    fileio::translators::TranslatorRefPtr translator = m_proxyShape->translatorManufacture().get(type);
    if(translator)
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("TranslatorContext::unloadPrim [tearDown] prim=%s\n", path.GetText());
      MStatus status = translator->tearDown(path);
      switch(status.statusCode())
      {
      case MStatus::kSuccess:
        break;

      case MStatus::kNotImplemented:
        MGlobal::displayError(
          MString("A variant switch has occurred on a NON-CONFORMING prim, of type: ") + type.GetText() +
          MString(" located at prim path \"") + path.GetText() + MString("\"")
          );
        break;

      default:
        MGlobal::displayError(
          MString("A variant switch has caused an error on tear down on prim, of type: ") + type.GetText() +
          MString(" located at prim path \"") + path.GetText() + MString("\"")
          );
        break;
      }

      m_proxyShape->removeUsdTransformChain(path, modifier, nodes::ProxyShape::kRequired);
    }
    else
    {
      MGlobal::displayError(MString("could not find usd translator plugin instance for prim: ") + path.GetText());
    }
    modifier.doIt();
  }
  else
  {
    MGlobal::displayError(MString("Could not unload prim: \"") + path.GetText() + MString("\", the stage is invalid"));
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
