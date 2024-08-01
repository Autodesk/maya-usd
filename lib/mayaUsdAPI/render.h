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
#ifndef MAYAUSDAPI_RENDER_H
#define MAYAUSDAPI_RENDER_H

#include <mayaUsdAPI/api.h>

#include <string>

#ifdef WANT_MATERIALX_BUILD
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Element.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/File.h>

#include <memory>
#endif

namespace MAYAUSDAPI_NS_DEF {

namespace ColorManagementPreferences {

//! \brief Returns true if color management is active.
MAYAUSD_API_PUBLIC
bool IsActive();

//! \brief Returns the OCIO color space name according to config file rules.
//! \param path The path of the file to be color managed.
MAYAUSD_API_PUBLIC
std::string getFileRule(const std::string& path);

} // namespace ColorManagementPreferences

#ifdef WANT_MATERIALX_BUILD

namespace OgsXmlGenerator {

MAYAUSD_API_PUBLIC
std::string textureToSamplerName(const std::string&);

MAYAUSD_API_PUBLIC
void setUseLightAPI(int);

MAYAUSD_API_PUBLIC
const std::string& getPrimaryUVSetName();

MAYAUSD_API_PUBLIC
void setPrimaryUVSetName(const std::string& val);

} // namespace OgsXmlGenerator

namespace OgsFragment {

// Forward declarations
struct OgsFragmentImpl;

// Implements transparency detection for some known types and then
// delegates to MaterialX for complex ones.
MAYAUSD_API_PUBLIC
bool isTransparentSurface(const MaterialX::ElementPtr& element);

// Derive a matrix4 parameter name from a matrix3 parameter name.
// Required because OGS doesn't support matrix3 parameters.
MAYAUSD_API_PUBLIC
std::string getMatrix4Name(const std::string& matrix3Name);

// Prepare all data structures to handle an internal Maya OCIO fragment:
MAYAUSD_API_PUBLIC
std::string registerOCIOFragment(const std::string& fragName);

// Get a library with all known internal Maya OCIO fragment.
MAYAUSD_API_PUBLIC
MaterialX::DocumentPtr getOCIOLibrary();

class MAYAUSD_API_PUBLIC OgsFragment
{
public:
    OgsFragment(MaterialX::ElementPtr, const MaterialX::FileSearchPath& librarySearchPath);
    ~OgsFragment();

    const std::string&          getFragmentSource() const;
    const std::string&          getFragmentName() const;
    const MaterialX::StringMap& getPathInputMap() const;
    std::string                 getMatrix4Name(const std::string& matrix3Name);

private:
    std::unique_ptr<OgsFragmentImpl> _imp;
};

} // namespace OgsFragment

namespace ShaderGenUtil {

// Forward declarations.
struct TopoNeutralGraphImpl;

class MAYAUSD_API_PUBLIC TopoNeutralGraph
{
public:
    TopoNeutralGraph(const MaterialX::ElementPtr&);
    ~TopoNeutralGraph();

    MaterialX::NodeGraphPtr nodeGraph();

    MaterialX::DocumentPtr getDocument() const;
    const std::string&     getOriginalPath(const std::string& topoPath) const;

    static bool isTopologicalNodeDef(const MaterialX::NodeDef& nodeDef);

    static const std::string& getMaterialName();

private:
    std::unique_ptr<TopoNeutralGraphImpl> _imp;
};

} // namespace ShaderGenUtil

#endif // WANT_MATERIALX_BUILD

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_RENDER_H
