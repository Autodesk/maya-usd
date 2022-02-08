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
