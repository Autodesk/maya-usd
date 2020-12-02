//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_STAGE_NODE_H
#define PXRUSDMAYA_STAGE_NODE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define PXRUSDMAYA_STAGE_NODE_TOKENS \
    ((MayaTypeName, "pxrUsdStageNode"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(UsdMayaStageNodeTokens, MAYAUSD_CORE_PUBLIC, PXRUSDMAYA_STAGE_NODE_TOKENS);

/// Maya dependency node that reads and outputs a USD stage.
///
/// This is a simple MPxNode that reads in the USD stage identified by its
/// file path attribute and makes that stage available as a stage data object
/// on its output attribute. Downstream Maya nodes can connect this output to
/// their own stage data input attributes to gain access to the stage. This
/// allows sharing of a single USD stage by multiple downstream consumer nodes,
/// and it keeps all of the specifics of reading/caching USD stages and layers
/// in this stage node so that consumers can simply focus on working with the
/// stage and its contents.
class UsdMayaStageNode : public MPxNode
{
public:
    MAYAUSD_CORE_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_CORE_PUBLIC
    static const MString typeName;

    // Attributes
    MAYAUSD_CORE_PUBLIC
    static MObject filePathAttr;
    MAYAUSD_CORE_PUBLIC
    static MObject outUsdStageAttr;

    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    static MStatus initialize();

    // MPxNode overrides
    MAYAUSD_CORE_PUBLIC
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

private:
    UsdMayaStageNode();
    ~UsdMayaStageNode() override;

    UsdMayaStageNode(const UsdMayaStageNode&);
    UsdMayaStageNode& operator=(const UsdMayaStageNode&);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
