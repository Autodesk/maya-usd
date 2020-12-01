//
// Copyright 2016 Pixar
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
// Modifications copyright (C) 2020 Autodesk
//
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorGprim.h>
#include <mayaUsd/fileio/translators/translatorMaterial.h>
#include <mayaUsd/fileio/translators/translatorMesh.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/nodes/stageNode.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFnBlendShapeDeformer.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
bool assignMaterial(
    const UsdGeomMesh&           mesh,
    const UsdMayaPrimReaderArgs& args,
    const MObject&               meshObj,
    UsdMayaPrimReaderContext*    context)
{
    // If a material is bound, create (or reuse if already present) and assign it
    // If no binding is present, assign the mesh to the default shader
    const UsdMayaJobImportArgs& jobArguments = args.GetJobArguments();
    return UsdMayaTranslatorMaterial::AssignMaterial(jobArguments, mesh, meshObj, context);
}
} // namespace

// prim reader for mesh
class MayaUsdPrimReaderMesh final : public UsdMayaPrimReader
{
public:
    MayaUsdPrimReaderMesh(const UsdMayaPrimReaderArgs& args)
        : UsdMayaPrimReader(args)
    {
    }

    ~MayaUsdPrimReaderMesh() override { }

    bool Read(UsdMayaPrimReaderContext* context) override;
};

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, UsdGeomMesh)
{
    UsdMayaPrimReaderRegistry::Register<UsdGeomMesh>([](const UsdMayaPrimReaderArgs& args) {
        return std::make_shared<MayaUsdPrimReaderMesh>(args);
    });
}

bool MayaUsdPrimReaderMesh::Read(UsdMayaPrimReaderContext* context)
{
    if (!context) {
        return false;
    }

    MStatus status { MS::kSuccess };

    const auto& prim = _GetArgs().GetUsdPrim();
    auto        mesh = UsdGeomMesh(prim);
    if (!mesh) {
        return false;
    }

    auto    parentNode = context->GetMayaNode(prim.GetPath().GetParentPath(), true);
    MObject transformObj;
    bool    retStatus = UsdMayaTranslatorUtil::CreateTransformNode(
        prim, parentNode, _GetArgs(), context, &status, &transformObj);
    if (!retStatus) {
        return false;
    }

    // get the USD stage node from the context's registry
    MObject stageNode;
    if (_GetArgs().GetUseAsAnimationCache()) {
        stageNode = context->GetMayaNode(
            SdfPath(UsdMayaStageNodeTokens->MayaTypeName.GetString()), false);
    }

    MayaUsd::TranslatorMeshRead meshRead(
        mesh,
        prim,
        transformObj,
        stageNode,
        _GetArgs().GetTimeInterval(),
        _GetArgs().GetUseAsAnimationCache(),
        &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // mesh is a shape, so read Gprim properties
    UsdMayaTranslatorGprim::Read(mesh, meshRead.meshObject(), context);

    // undo/redo mesh object
    context->RegisterNewMayaNode(meshRead.shapePath().GetString(), meshRead.meshObject());

    // undo/redo deformable mesh (blenshape, PointBasedDeformer)
    if (meshRead.pointsNumTimeSamples() > 0) {
        if (_GetArgs().GetUseAsAnimationCache()) {
            context->RegisterNewMayaNode(
                meshRead.pointBasedDeformerName().asChar(), meshRead.pointBasedDeformerNode());
        } else {
            if (meshRead.blendObject().apiType() == MFn::kBlend) {
                return false;
            }

            MFnBlendShapeDeformer blendFnSet(meshRead.blendObject());
            context->RegisterNewMayaNode(blendFnSet.name().asChar(), meshRead.blendObject());
        }
    }

    // assign primvars to mesh
    UsdMayaMeshReadUtils::assignPrimvarsToMesh(
        mesh, meshRead.meshObject(), _GetArgs().GetExcludePrimvarNames());

    // assign invisible faces
    UsdMayaMeshReadUtils::assignInvisibleFaces(mesh, meshRead.meshObject());

    // assign material
    assignMaterial(mesh, _GetArgs(), meshRead.meshObject(), context);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
