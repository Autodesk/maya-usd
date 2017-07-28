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
#include <iostream>
#include <stack>
#include "maya/MString.h"
#include "maya/MDagModifier.h"
#include "maya/MPlug.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "AL/usdmaya/nodes/USDToMayaMappingDB.h"
#include "AL/usdmaya/StageData.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/maya/CodeTimings.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/modelAPI.h"

// printf debugging
#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cerr << X << std::endl;
#else
# define Trace(X)
#endif

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
SchemaNodeRefDB::SchemaNodeRefDB(nodes::ProxyShape* const proxy)
 : m_nodeRefs(),
   m_proxy(proxy),
   m_context(fileio::translators::TranslatorContext::create(proxy)),
   m_translatorManufacture(context())
{
  if(!m_context)
  {
    MPlug outStage = m_proxy->outStageDataPlug();
    auto handle = outStage.asMDataHandle();
    //if(handle)
    {
      StageData* stageData = static_cast<StageData*>(handle.asPluginData());
      if(stageData)
      {
        stageData->stage = m_proxy->getUsdStage();
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
fileio::translators::TranslatorContextPtr SchemaNodeRefDB::context()
{
  return m_context;
}

//----------------------------------------------------------------------------------------------------------------------
SchemaNodeRefDB::~SchemaNodeRefDB()
{
}

//----------------------------------------------------------------------------------------------------------------------
void SchemaNodeRefDB::addEntry(const SdfPath& primPath, const MObject& primObj)
{
  Trace("SchemaNodeRefDB::addEntry primPath="<<primPath);
  m_nodeRefs.emplace_back(primPath, primObj);
}

//----------------------------------------------------------------------------------------------------------------------
void SchemaNodeRefDB::preRemoveEntry(const SdfPath& primPath, SdfPathVector& itemsToRemove, bool callPreUnload)
{
  Trace("SchemaNodeRefDB::preRemoveEntry primPath=" << primPath);
  SchemaNodeRefs::iterator end = m_nodeRefs.end();
  SchemaNodeRefs::iterator range_begin = std::lower_bound(m_nodeRefs.begin(), end, primPath, value_compare());
  SchemaNodeRefs::iterator range_end = range_begin;
  const std::string& pathBeingRemoved = primPath.GetString();
  const std::size_t length = pathBeingRemoved.size();
  for(; range_end != end; ++range_end)
  {
    // due to the joys of sorting, any child prims of this prim being destroyed should appear next to each
    // other (one would assume); So if compare does not find a match (the value is something other than zero),
    // we are no longer in the same prim root

    const char* const parentPath = pathBeingRemoved.c_str();
    const char* const childPath = range_end->primPath().GetText();

    if(strncmp(parentPath, childPath, length))
    {
      break;
    }
  }

  auto stage = m_proxy->getUsdStage();

  // run the preTearDown stage on each prim. We will walk over the prims in the reverse order here (which will guarentee
  // the the itemsToRemove will be ordered such that the child prims will be destroyed before their parents).
  auto iter = range_end;
  itemsToRemove.reserve(range_end - range_begin);
  while(iter != range_begin)
  {
    --iter;
    SchemaNodeRef& node = *iter;
    itemsToRemove.push_back(node.primPath());
    auto prim = stage->GetPrimAtPath(node.primPath());
    if(prim && callPreUnload)
    {
      preUnloadPrim(prim, node.mayaObject());
    }
    else
    {
      Trace("invalid path found! " << node.primPath());
    }
  }
}


//----------------------------------------------------------------------------------------------------------------------
void SchemaNodeRefDB::removeEntries(const SdfPathVector& itemsToRemove)
{
  Trace("SchemaNodeRefDB::removeEntries");
  auto stage = m_proxy->getUsdStage();

  SchemaNodeRefs::iterator begin = m_nodeRefs.begin();
  SchemaNodeRefs::iterator end = m_nodeRefs.end();

  SdfPathVector pathsToErase;

  // so now we need to unload the prims from (range_end - 1) to temp (in that order, otherwise we'll nuke parents before children)
  auto iter = itemsToRemove.begin();
  while(iter != itemsToRemove.end())
  {
    auto path = *iter;
    SchemaNodeRefs::iterator node = std::lower_bound(begin, end, path, value_compare());

    auto prim = stage->GetPrimAtPath(*iter);
    TfToken type = m_context->getTypeForPath(path);

    // if the prim is no longer there, let's kill it
    if(!prim)
    {
      fileio::translators::TranslatorRefPtr translator = m_translatorManufacture.get(type);
      if(translator)
      {
        unloadPrim(path, node->mayaObject());
        pathsToErase.push_back(*iter);
      }
    }
    else
    {
      // so we have the prim, let's check the type to see whether that has changed
      TfToken primType = prim.GetTypeName();
      if(type != primType)
      {
        // type has changed, update the prim type and unload the old prim
        unloadPrim(path, node->mayaObject());
        pathsToErase.push_back(path);
      }
      else
      {
        fileio::translators::TranslatorRefPtr translator = m_translatorManufacture.get(type);
        if(translator)
        {
          if(!translator->supportsInactive() || !translator->supportsUpdate())
          {
            unloadPrim(path, node->mayaObject());
            pathsToErase.push_back(*iter);
          }
        }
        else
        {
          unloadPrim(path, node->mayaObject());
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
      Trace("SchemaNodeRefDB::removeEntry primPath="<<path);
      SchemaNodeRefs::iterator node = std::lower_bound(m_nodeRefs.begin(), m_nodeRefs.end(), path, value_compare());

      // remove nodes from map
      m_nodeRefs.erase(node);
      m_proxy->removeUsdTransformChain(path, modifier, ProxyShape::kRequired);
    }

    modifier.doIt();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void SchemaNodeRefDB::preUnloadPrim(UsdPrim& prim, const MObject& primObj)
{
  Trace("SchemaNodeRefDB::preUnloadPrim");
  assert(m_proxy);
  auto stage = m_proxy->getUsdStage();
  if(stage)
  {
    TfToken type = m_context->getTypeForPath(prim.GetPath());

    fileio::translators::TranslatorRefPtr translator = m_translatorManufacture.get(type);
    if(translator)
    {
      Trace("Translator-VariantSwitch: preTearDown prim: " << prim.GetPath().GetText() << " " << type);
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
void SchemaNodeRefDB::unloadPrim(const SdfPath& path, const MObject& primObj)
{
  Trace("SchemaNodeRefDB::unloadPrim");
  assert(m_proxy);
  auto stage = m_proxy->getUsdStage();
  if(stage)
  {
    MDagModifier modifier;
    TfToken type = m_context->getTypeForPath(path);

    fileio::translators::TranslatorRefPtr translator = m_translatorManufacture.get(type);
    if(translator)
    {
      Trace("Translator-VariantSwitch: tearDown prim: " << path.GetText() << " " << type);
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

      m_proxy->removeUsdTransformChain(path, modifier, ProxyShape::kRequired);
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
void SchemaNodeRefDB::outputPrims(std::ostream& os)
{
  for(auto ptr : m_nodeRefs)
  {
    os << ptr.primPath().GetText() << "[" << context()->getTypeForPath(ptr.primPath()) << "]" << std::endl;
  }
}

//----------------------------------------------------------------------------------------------------------------------
MString SchemaNodeRefDB::serialize()
{
  MString str;
  for(auto iter : m_nodeRefs)
  {
    if(iter.mayaObject().hasFn(MFn::kDagNode))
    {
      MFnDagNode fn(iter.mayaObject());
      MDagPath path;
      fn.getPath(path);
      str += path.fullPathName();
      str += ",";
      str += iter.primPath().GetText();
      str += ";";
    }
    else
    {
      MFnDependencyNode fn(iter.mayaObject());
      str += fn.name();
      str += ",";
      str += iter.primPath().GetText();
      str += ";";
    }
  }
  return str;
}

//----------------------------------------------------------------------------------------------------------------------
void SchemaNodeRefDB::deserialize(const MString& str)
{
  MStringArray pairs;
  str.split(';', pairs);
  for(uint32_t i = 0; i < pairs.length(); ++i)
  {
    MStringArray pair;
    pairs[i].split(',', pair);
    MSelectionList sl;
    if(sl.add(pair[0]))
    {
      MObject node;
      if(sl.getDependNode(0, node))
      {
        m_nodeRefs.emplace_back(SdfPath(pair[1].asChar()), node);
      }
    }
  }

  // They should in theory be sorted already (can probably remove this line)
  std::sort(m_nodeRefs.begin(), m_nodeRefs.end(), value_compare());
}


//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

