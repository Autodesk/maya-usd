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

#include "MaterialXCore/Library.h"

#include <mayaUsdAPI/api.h>

#include <string>

#ifdef WANT_MATERIALX_BUILD
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Element.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/File.h>

#include <memory>
#include <vector>
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
    const MaterialX::StringMap& getEmbeddedTextureMap() const;
    std::string                 getMatrix4Name(const std::string& matrix3Name);

private:
    std::unique_ptr<OgsFragmentImpl> _imp;
};

} // namespace OgsFragment

namespace ShaderGenUtil {

// Forward declaration:
struct LobePrunerImpl;
struct TopoNeutralGraphImpl;

/*! \brief  This class will process MaterialX surface shader nodes and provide optimized version of
 * the shader based on the current value of a node attribute.
 */
class MAYAUSD_API_PUBLIC LobePruner
{
public:
    using Ptr = std::shared_ptr<LobePruner>;

    /*! Creates a shared LobePruner.
     *  \return the freshly created LobePruner.
     */
    static Ptr create();

    ~LobePruner();

    /*! Sets the LobePruner library. This is used first to explore all surface shaders for
     * optimization candidated, then to store the optimized NodeDef and NodeGraph this class
     * generates.
     * @param[in] library is a fully loaded MaterialX library where the generated optimized shaders
     * will be stored.
     */
    void setLibrary(const MaterialX::DocumentPtr& library);

private:
    LobePruner();

    std::unique_ptr<LobePrunerImpl> _imp;
    friend struct TopoNeutralGraphImpl;
};

class MAYAUSD_API_PUBLIC TopoNeutralGraph
{
public:
    /*! Creates a barebones TopoNeutralGraph that will process the provided material and generate a
     * topo neutral version of it.
     * @param[in] material the material to process
     */
    TopoNeutralGraph(const MaterialX::ElementPtr& material);

    /*! Creates a TopoNeutralGraph that will process the provided material and generate a topo
     * neutral version of it. It will also substitute lobe pruned categories if a LobePruner is
     * provided.
     * @param[in] material the material to process
     * @param[in] lobePruner an instance of a LobePruner. These are usually singletons that
     * accumulate pruned NodeDefs
     */
    TopoNeutralGraph(const MaterialX::ElementPtr& material, const LobePruner::Ptr& lobePruner);

    /*! Creates a TopoNeutralGraph that will process the provided material and generate a topo
     * neutral version of it. It will also substitute lobe pruned categories if a LobePruner is
     * provided.
     * @param[in] material the material to process
     * @param[in] lobePruner an instance of a LobePruner. These are usually singletons that
     * accumulate pruned NodeDefs
     * @param[in] textured is true if the full material is to be processed. When false, we will
     * generate an untextured topo neutral material instead
     */
    TopoNeutralGraph(
        const MaterialX::ElementPtr& material,
        const LobePruner::Ptr&       lobePruner,
        bool                         textured);
    ~TopoNeutralGraph();

    MaterialX::NodeGraphPtr nodeGraph();

    MaterialX::DocumentPtr getDocument() const;
    const std::string&     getOriginalPath(const std::string& topoPath) const;

    /*! Get the list of node.attribute paths used by the LobePruner to optimize surface shader nodes
     * found in the material that was processed.
     * \return a string vector containing all paths to attibutes that were used to optimize nodes in
     * this graph.
     */
    const MaterialX::StringVec& getOptimizedAttributes() const;

    static bool isTopologicalNodeDef(const MaterialX::NodeDef& nodeDef);

    static const std::string& getMaterialName();

private:
    std::unique_ptr<TopoNeutralGraphImpl> _imp;
};

} // namespace ShaderGenUtil

#endif // WANT_MATERIALX_BUILD

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_RENDER_H
