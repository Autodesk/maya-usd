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

/// \file usdMaya/pointBasedDeformerNode.h

#include "usdMaya/api.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"

#include <maya/MDataBlock.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS \
    ((MayaTypeName, "pxrUsdPointBasedDeformerNode"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaPointBasedDeformerNodeTokens,
                         PXRUSDMAYA_API,
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
        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        // Attributes
        PXRUSDMAYA_API
        static MObject inUsdStageAttr;
        PXRUSDMAYA_API
        static MObject primPathAttr;
        PXRUSDMAYA_API
        static MObject timeAttr;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        // MPxGeometryFilter overrides
        PXRUSDMAYA_API
        MStatus deform(
                MDataBlock& block,
                MItGeometry& iter,
                const MMatrix& mat,
                unsigned int multiIndex) override;

    private:
        UsdMayaPointBasedDeformerNode();
        ~UsdMayaPointBasedDeformerNode() override;

        UsdMayaPointBasedDeformerNode(const UsdMayaPointBasedDeformerNode&);
        UsdMayaPointBasedDeformerNode& operator=(
                const UsdMayaPointBasedDeformerNode&);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
