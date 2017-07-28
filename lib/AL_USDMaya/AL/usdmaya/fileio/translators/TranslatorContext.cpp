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
#include "maya/MSelectionList.h"
#include "maya/MFnDagNode.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cerr << X << std::endl;
#else
# define Trace(X)
#endif

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
  Trace("** VALIDATE PRIMS **");
  for(auto it : m_primMapping)
  {
    if(it.second.m_object.isValid() && it.second.m_object.isAlive())
    {
      Trace("** VALID HANDLE DETECTED **" << it.first);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorContext::getTransform(const SdfPath& path, MObjectHandle& object)
{
  Trace("gettingTransform: " << path.GetText());
  auto it = m_primMapping.find(path.GetString());
  if(it != m_primMapping.end())
  {
    if(!it->second.m_object.isValid())
    {
      return false;
    }
    object = it->second.m_object;
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
    SdfPath path(it->first);
    UsdPrim prim = stage->GetPrimAtPath(path);
    if(!prim)
    {
      m_primMapping.erase(it++);
    }
    else
    if(it->second.m_type != prim.GetTypeName())
    {
      it->second.m_type = prim.GetTypeName();
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
  Trace("getMObject: " << path.GetText());
  auto it = m_primMapping.find(path.GetString());
  if(it != m_primMapping.end())
  {
    const MTypeId zero(0);
    if(zero != typeId)
    {
      for(auto temp : it->second.m_createdNodes)
      {
        MFnDependencyNode fn(temp.object());
        Trace("getting: " << fn.typeName());
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
      if(!it->second.m_createdNodes.empty())
      {
        Trace("getting anything: " << path.GetString());
        object = it->second.m_createdNodes[0];

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
  Trace("getMObject: " << path.GetText());
  auto it = m_primMapping.find(path.GetString());
  if(it != m_primMapping.end())
  {
    const MTypeId zero(0);
    if(MFn::kInvalid != type)
    {
      for(auto temp : it->second.m_createdNodes)
      {
        Trace("getting: " << temp.object().apiTypeStr());
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
      if(!it->second.m_createdNodes.empty())
      {
        Trace("getting anything: " << path.GetString());
        object = it->second.m_createdNodes[0];

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
  Trace("getMObjects: " << path.GetText());
  auto it = m_primMapping.find(path.GetString());
  if(it != m_primMapping.end())
  {
    returned = it->second.m_createdNodes;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::registerItem(const UsdPrim& prim, MObjectHandle object)
{
  //Trace("registerItem: " << prim.GetPath().GetText());
  auto str = prim.GetPath().GetString();
  auto& item = m_primMapping[str];
  item.m_type = prim.GetTypeName();
  item.m_object = object;

  if(object.object() == MObject::kNullObj)
  {
    Trace("TranslatorContext::registerItem primPath=" << prim.GetPath().GetText() << " primType=" << item.m_type.GetText() << " to null MObject");
  }
  else
  {
    Trace("TranslatorContext::registerItem primPath=" << prim.GetPath().GetText() << " primType=" << item.m_type.GetText() << " to MObject type " << object.object().apiTypeStr());
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::insertItem(const UsdPrim& prim, MObjectHandle object)
{
  Trace("insertItem: " << prim.GetPath().GetText());
  auto str = prim.GetPath().GetString();
  auto& item = m_primMapping[str];
  item.m_createdNodes.push_back(object);

  if(object.object() == MObject::kNullObj)
  {
    Trace("TranslatorContext::insertItem primPath=" << prim.GetPath().GetText() << " primType=" << item.m_type.GetText() << " to null object");
  }
  else
  {
    Trace("TranslatorContext::insertItem primPath=" << prim.GetPath().GetText() << " primType=" << item.m_type.GetText() << " to object type " << object.object().apiTypeStr());
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TranslatorContext::removeItems(const SdfPath& path)
{
  Trace("removeItems: " << path.GetText());
  auto it = m_primMapping.find(path.GetString());
  if(it != m_primMapping.end())
  {
    Trace("TranslatorContext::removeItems primPath="<<path.GetText())
    MDGModifier modifier1;
    MDagModifier modifier2;
    MStatus status;
    bool hasDagNodes = false;
    bool hasDependNodes = false;

    auto& nodes = it->second.m_createdNodes;
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
        MGlobal::displayError(MString("invalid MObject found at path \"") + path.GetText() + MString("\""));
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
    oss << it.first << "=" << it.second.m_type.GetText() << ",";
    oss << getNodeName(it.second.m_object.object());
    for(uint32_t i = 0; i < it.second.m_createdNodes.size(); ++i)
    {
      oss << "," << getNodeName(it.second.m_createdNodes[i].object());
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

    PrimLookup lookup;
    lookup.m_type = TfToken(strings3[0].asChar());

    MSelectionList sl;
    sl.add(strings3[1].asChar());
    MObject obj;
    sl.getDependNode(0, obj);
    lookup.m_object = obj;

    for(uint32_t j = 2; j < strings3.length(); ++j)
    {
      sl.add(strings3[j].asChar());
      MObject obj;
      sl.getDependNode(0, obj);
      lookup.m_createdNodes.push_back(obj);
    }

    m_primMapping.emplace(strings2[0].asChar(), lookup);
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
