//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MAYA_MATERIALX_HWIMAGECODENODE_H
#define MAYA_MATERIALX_HWIMAGECODENODE_H

#include <mayaUsd/render/MaterialXGenOgsXml/CombinedMaterialXVersion.h>

#if MX_COMBINED_VERSION >= 13905
#include <MaterialXGenHw/Nodes/HwImageNode.h>
#else
#include <MaterialXGenShader/Nodes/HwImageNode.h>
#endif

MATERIALX_NAMESPACE_BEGIN

/// Extending the HwImageNode to propagate the inputs.
class MayaHwImageNode : public HwImageNode
{
public:
    static ShaderNodeImplPtr create();

    void addInputs(ShaderNode& node, GenContext& context) const override;
};

MATERIALX_NAMESPACE_END

#endif
