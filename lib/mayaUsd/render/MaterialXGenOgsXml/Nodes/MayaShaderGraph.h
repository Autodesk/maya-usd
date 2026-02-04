//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_SHADERGRAPH_H
#define MAYA_MATERIALX_SHADERGRAPH_H

/// @file
/// Shader graph class

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#include <MaterialXGenShader/ShaderGraph.h>

MATERIALX_NAMESPACE_BEGIN

/// @class ShaderGraph
/// Class representing a graph (DAG) for shader generation
class MayaShaderGraph : public ShaderGraph
{
public:
    /// Constructor
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    MayaShaderGraph(
        const ShaderGraph* parent,
        const string&      name,
        const ElementPtr&  element,
        GenContext&        context);
#endif
    /// Constructor.
    MayaShaderGraph(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context);

    /// Desctructor.
    virtual ~MayaShaderGraph();

#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    /// Create a new shader graph from an element.
    /// Supported elements are outputs and shader nodes.
    static ShaderGraphPtr
    create(const ShaderGraph* parent, const string& name, ElementPtr element, GenContext& context);
#endif

    /// Create a new shader graph from a nodegraph.
    static ShaderGraphPtr
    create(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context);

    void addPropagatedInput(ShaderNode& node, string const& name);

    StringVec const& getPropagatedInputs() const;

protected:
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    /// Create node connections corresponding to the connection between a pair of elements.
    /// @param downstreamElement Element representing the node to connect to.
    /// @param upstreamElement Element representing the node to connect from
    /// @param connectingElement If non-null, specifies the element on on the downstream node to
    /// connect to.
    /// @param context Context for generation.
    void createConnectedNodes(
        const ElementPtr& downstreamElement,
        const ElementPtr& upstreamElement,
        ElementPtr        connectingElement,
        GenContext&       context);

    /// Traverse from the given root element and add all dependencies upstream.
    /// The traversal is done in the context of a material, if given, to include
    /// bind input elements in the traversal.
    void addUpstreamDependencies(const Element& root, GenContext& context);
#endif

    StringVec _propagatedInputs;
#if MX_COMBINED_VERSION >= 13810 && MX_COMBINED_VERSION < 13903
    bool _shouldPropagateInputs;
#endif
};

MATERIALX_NAMESPACE_END

#endif
