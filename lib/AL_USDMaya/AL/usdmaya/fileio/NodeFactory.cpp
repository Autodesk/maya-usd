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
#include "AL/usdmaya/AttributeType.h"
#include "AL/usdmaya/fileio/Import.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/CameraTranslator.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/MeshTranslator.h"
#include "AL/usdmaya/fileio/translators/NurbsCurveTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"

#include "maya/MObject.h"
#include "maya/MString.h"
#include "maya/MFnDependencyNode.h"
#include "AL/usdmaya/utils/Utils.h"

namespace AL {
namespace usdmaya {
namespace fileio {

NodeFactory* g_nodeFactory = 0;

//----------------------------------------------------------------------------------------------------------------------
void freeNodeFactory()
{
  delete g_nodeFactory;
  g_nodeFactory = 0;
}

//----------------------------------------------------------------------------------------------------------------------
NodeFactory& getNodeFactory()
{
  if(!g_nodeFactory)
  {
    g_nodeFactory = new NodeFactory;
  }
  return *g_nodeFactory;
}

//----------------------------------------------------------------------------------------------------------------------
NodeFactory::NodeFactory()
: m_builders(), m_params(0)
{
  translators::DgNodeTranslator::registerType();
  translators::DagNodeTranslator::registerType();
  translators::TransformTranslator::registerType();
  translators::MeshTranslator::registerType();
  translators::NurbsCurveTranslator::registerType();
  translators::CameraTranslator::registerType();
  m_builders.insert(std::make_pair("node", new translators::DgNodeTranslator));
  m_builders.insert(std::make_pair("dagNode", new translators::DagNodeTranslator));
  m_builders.insert(std::make_pair("transform", new translators::TransformTranslator));
  m_builders.insert(std::make_pair("mesh", new translators::MeshTranslator));
  m_builders.insert(std::make_pair("nurbsCurve", new translators::NurbsCurveTranslator));
  m_builders.insert(std::make_pair("camera", new translators::CameraTranslator));
}

//----------------------------------------------------------------------------------------------------------------------
NodeFactory::~NodeFactory()
{
  auto it = m_builders.begin();
  auto e = m_builders.end();
  for(; it != e; ++it)
  {
    delete it->second;
  }
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeFactory::createNode(const UsdPrim& from, const char* const nodeType, MObject parent)
{
  std::unordered_map<std::string, translators::DgNodeTranslator*>::iterator it = m_builders.find(nodeType);
  if(it == m_builders.end()) return MObject::kNullObj;
  MObject obj = it->second->createNode(from, parent, nodeType, *m_params);
  if(obj != MObject::kNullObj)
  {
    MFnDependencyNode fn(obj);

    MString nodeName;
    MString newNodeName;

    nodeName = from.GetName().GetText();
    if(obj.hasFn(MFn::kShape))
    {
      nodeName += "Shape";

      // Write in the shapes parent transform node's path instead of the shape.
      // This was done because we want the xform to be selected when chosen through the outliner instead of the shape.
      AL::usdmaya::utils::mapUsdPrimToMayaNode(from, parent);
    }
    else
    {
      AL::usdmaya::utils::mapUsdPrimToMayaNode(from, obj);
    }
    newNodeName = fn.setName(nodeName);

    // if the name has changed on import, add the original name to the node so we can keep track of this.
    if(nodeName != newNodeName)
    {
      fileio::translators::DgNodeTranslator::addStringValue(obj, "alusd_originalName", newNodeName.asChar());
    }
  }
  return obj;
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

