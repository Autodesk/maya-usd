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
#include "AL/usdmaya/ForwardDeclares.h"

#include <string>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A simple node factory to convert nodes between Maya and USD
/// \todo   Phase this class out, and migrate the existing code into the translator framework.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct NodeFactory
{
    /// \brief  ctor. Currently initialises the inbuilt translators.
    NodeFactory();

    /// \brief  dtor
    ~NodeFactory();

    /// \brief  create a node
    /// \param  from the prim we are copying the data from
    /// \param  nodeType can be one of "transform", "mesh", "nurbsCurve", or "camera".
    /// \param  parent the parent transform for the Maya data
    /// \param  parentUnmerged if false, the parent transform will be merged with a shape. If true,
    /// the nodes will remain
    ///         separate
    MObject createNode(
        const UsdPrim&    from,
        const char* const nodeType,
        MObject           parent,
        bool              parentUnmerged = false);

    static void setupNode(const UsdPrim& from, MObject obj, MObject parent, bool parentUnmerged);

    /// \brief  Some of the translators rely on import settings specified in the import params.
    /// Prior to use of this factory,
    ///         you should set the import params for it to use.
    /// \param  params the import params
    void setImportParams(const ImporterParams* params) { m_params = params; }

private:
    std::unordered_map<std::string, translators::DgNodeTranslator*> m_builders;
    const ImporterParams*                                           m_params;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  returns the global node factory instance
/// \return the node factory
//----------------------------------------------------------------------------------------------------------------------
NodeFactory& getNodeFactory();

//----------------------------------------------------------------------------------------------------------------------
/// \brief  destroys the global node factory
//----------------------------------------------------------------------------------------------------------------------
void freeNodeFactory();

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
