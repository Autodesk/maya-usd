//
// Copyright 2022 Autodesk
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
#ifndef PXRUSDMAYA_MAYAUSDGEOMNODE_H
#define PXRUSDMAYA_MAYAUSDGEOMNODE_H

#include "base/api.h"

#include <mayaUsd/base/api.h>

#include <maya/MObject.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

// Maya node used to extract geometry from a USD file to use it in a scene. For example
// this can be used to apply GPU deformers to USD geometry without importing to Maya.
//
// Note, in its current state, this node can be used for exploration and maybe to debug
// some situations but it would require some enhancements to be ready to be used in
// production projects. (i.e. better cache management, file change detection, etc.)
//
// Input attributes:
//  File Path: Path to the usd file to import.
//  Root Primitive: Root primitive from which to start the import, this can be used to
//      limit the imported geometry from large USD files. The base root "/" is used if
//      attribute is not set.
//
// Output attributes:
//  Geometry: Array of mesh objects representing each imported primitive.
//  Matrix: Array of matrices corresponding to each imported primitive's transformation
//      in the same order as the geometry.
class MayaUsdGeomNode : public MPxNode
{
public:
    MAYAUSD_PLUGIN_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_PLUGIN_PUBLIC
    static const MString typeName;

    // Attributes
    MAYAUSD_PLUGIN_PUBLIC
    static MObject filePathAttr;

    MAYAUSD_PLUGIN_PUBLIC
    static MObject rootPrimAttr;

    // Output attributes
    MAYAUSD_PLUGIN_PUBLIC
    static MObject outGeomAttr;

    MAYAUSD_PLUGIN_PUBLIC
    static MObject outGeomMatrixAttr;

    MAYAUSD_PLUGIN_PUBLIC
    static void* creator();

    MAYAUSD_PLUGIN_PUBLIC
    static MStatus initialize();

    MayaUsdGeomNode();
    virtual ~MayaUsdGeomNode();

    virtual MStatus compute(const MPlug&, MDataBlock&);

private:
    struct CacheData;
    std::unique_ptr<CacheData> _cache;
};

} // namespace MAYAUSD_NS_DEF

#endif
