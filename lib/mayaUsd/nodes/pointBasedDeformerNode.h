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
#ifndef PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_H
#define PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MDataBlock.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

#define PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS ((MayaTypeName, "pxrUsdPointBasedDeformerNode"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaPointBasedDeformerNodeTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS);

/// Maya deformer that uses the points of a UsdGeomPointBased prim to deform
/// the geometry.
///
/// This deformer node can be used to deform Maya geometry to match the points
/// of a UsdGeomPointBased prim. It takes as input a stage data object (which
/// can be received from a connection to a USD stage node), the prim path to a
/// UsdGeomPointBased prim in the stage data's stage, and a time sample. When
/// the deformer runs, it will read the points attribute of the prim at that
/// time sample and use the positions to modify the positions of the geometry
/// being deformed.
class UsdMayaPointBasedDeformerNode : public MPxDeformerNode
{
public:
    MAYAUSD_CORE_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_CORE_PUBLIC
    static const MString typeName;

    // Attributes
    MAYAUSD_CORE_PUBLIC
    static MObject inUsdStageAttr;
    MAYAUSD_CORE_PUBLIC
    static MObject primPathAttr;
    MAYAUSD_CORE_PUBLIC
    static MObject timeAttr;

    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    static MStatus initialize();

    // MPxGeometryFilter overrides
    MAYAUSD_CORE_PUBLIC
    MStatus
    deform(MDataBlock& block, MItGeometry& iter, const MMatrix& mat, unsigned int multiIndex)
        override;

private:
    UsdMayaPointBasedDeformerNode();
    ~UsdMayaPointBasedDeformerNode() override;

    UsdMayaPointBasedDeformerNode(const UsdMayaPointBasedDeformerNode&);
    UsdMayaPointBasedDeformerNode& operator=(const UsdMayaPointBasedDeformerNode&);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
