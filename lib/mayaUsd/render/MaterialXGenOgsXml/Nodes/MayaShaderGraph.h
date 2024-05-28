//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_SHADERGRAPH_H
#define MAYA_MATERIALX_SHADERGRAPH_H

/// @file
/// Shader graph class

#include <MaterialXGenShader/ShaderGraph.h>

MATERIALX_NAMESPACE_BEGIN

/// @class ShaderGraph
/// Class representing a graph (DAG) for shader generation
class MayaShaderGraph : public ShaderGraph
{
public:
    /// Constructor.
    MayaShaderGraph(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context);

    /// Desctructor.
    virtual ~MayaShaderGraph();

    /// Create a new shader graph from a nodegraph.
    static ShaderGraphPtr
    create(const ShaderGraph* parent, const NodeGraph& nodeGraph, GenContext& context);

    void addPropagatedInput(ShaderNode& node, string const& name);

    StringVec const& getPropagatedInputs() const;

protected:
    StringVec _propagatedInputs;
};

MATERIALX_NAMESPACE_END

#endif
