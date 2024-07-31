//
// Copyright 2024 Autodesk
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

#include "render.h"

#include <mayaUsd/render/MaterialXGenOgsXml/OgsFragment.h>
#include <mayaUsd/render/MaterialXGenOgsXml/OgsXmlGenerator.h>
#include <mayaUsd/render/MaterialXGenOgsXml/ShaderGenUtil.h>
#include <mayaUsd/render/vp2RenderDelegate/colorManagementPreferences.h>

namespace MAYAUSDAPI_NS_DEF {

namespace ColorManagementPreferences {

bool IsActive() { return MayaUsd::ColorManagementPreferences::Active(); }

std::string getFileRule(const std::string& path)
{
    return MayaUsd::ColorManagementPreferences::getFileRule(path);
}

} // namespace ColorManagementPreferences

namespace OgsXmlGenerator {

std::string textureToSamplerName(const std::string& textureName)
{
    return MaterialX::OgsXmlGenerator::textureToSamplerName(textureName);
}

void setUseLightAPI(int val) { MaterialX::OgsXmlGenerator::setUseLightAPI(val); }

const std::string& getPrimaryUVSetName()
{
    return MaterialX::OgsXmlGenerator::getPrimaryUVSetName();
}

void setPrimaryUVSetName(const std::string& val)
{
    MaterialX::OgsXmlGenerator::setPrimaryUVSetName(val);
}

} // namespace OgsXmlGenerator

namespace OgsFragment {

bool isTransparentSurface(const MaterialX::ElementPtr& element)
{
    return MaterialXMaya::OgsFragment::isTransparentSurface(element);
}

std::string getMatrix4Name(const std::string& matrix3Name)
{
    return MaterialXMaya::OgsFragment::getMatrix4Name(matrix3Name);
}

std::string registerOCIOFragment(const std::string& fragName)
{
    return MaterialXMaya::OgsFragment::registerOCIOFragment(fragName);
}

MaterialX::DocumentPtr getOCIOLibrary() { return MaterialXMaya::OgsFragment::getOCIOLibrary(); }

struct OgsFragmentImpl
{
    OgsFragmentImpl(MaterialX::ElementPtr ptr, const MaterialX::FileSearchPath& librarySearchPath)
        : _ogsFragment(ptr, librarySearchPath)
    {
    }

    const MaterialXMaya::OgsFragment _ogsFragment;
};

OgsFragment::OgsFragment(
    MaterialX::ElementPtr            ptr,
    const MaterialX::FileSearchPath& librarySearchPath)
    : _imp(new OgsFragmentImpl(ptr, librarySearchPath))
{
}

// When using a pimpl you need to define the destructor here in the
// .cpp so the compiler has access to the impl.
OgsFragment::~OgsFragment() = default;

const std::string& OgsFragment::getFragmentSource() const
{
    return _imp->_ogsFragment.getFragmentSource();
}

const std::string& OgsFragment::getFragmentName() const
{
    return _imp->_ogsFragment.getFragmentName();
}

const MaterialX::StringMap& OgsFragment::getPathInputMap() const
{
    return _imp->_ogsFragment.getPathInputMap();
}

std::string OgsFragment::getMatrix4Name(const std::string& matrix3Name)
{
    return _imp->_ogsFragment.getMatrix4Name(matrix3Name);
}

} // namespace OgsFragment

namespace ShaderGenUtil {

struct TopoNeutralGraphImpl
{
    TopoNeutralGraphImpl(const MaterialX::ElementPtr& material)
        : _topoGraph(material)
    {
    }

    MaterialXMaya::ShaderGenUtil::TopoNeutralGraph _topoGraph;
};

TopoNeutralGraph::TopoNeutralGraph(const MaterialX::ElementPtr& material)
    : _imp(new TopoNeutralGraphImpl(material))
{
}

// When using a pimpl you need to define the destructor here in the
// .cpp so the compiler has access to the impl.
TopoNeutralGraph::~TopoNeutralGraph() = default;

MaterialX::NodeGraphPtr TopoNeutralGraph::nodeGraph() { return _imp->_topoGraph.getNodeGraph(); }

MaterialX::DocumentPtr TopoNeutralGraph::getDocument() const
{
    return _imp->_topoGraph.getDocument();
}

const std::string& TopoNeutralGraph::getOriginalPath(const std::string& topoPath) const
{
    return _imp->_topoGraph.getOriginalPath(topoPath);
}

bool TopoNeutralGraph::isTopologicalNodeDef(const MaterialX::NodeDef& nodeDef)
{
    return MaterialXMaya::ShaderGenUtil::TopoNeutralGraph::isTopologicalNodeDef(nodeDef);
}

const std::string& TopoNeutralGraph::getMaterialName()
{
    return MaterialXMaya::ShaderGenUtil::TopoNeutralGraph::getMaterialName();
}

} // namespace ShaderGenUtil

} // namespace MAYAUSDAPI_NS_DEF
