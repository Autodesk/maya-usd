//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mayaHydraDataSource.h"

#include <mayaHydraLib/sceneIndex/mayaHydraDisplayStyleDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraPrimvarDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraCameraDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraLightDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraSceneIndex.h>
#include <mayaHydraLib/adapters/adapter.h>

#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/basisCurvesSchema.h>
#include <pxr/imaging/hd/basisCurvesTopologySchema.h>
#include <pxr/imaging/hd/cameraSchema.h>
#include <pxr/imaging/hd/legacyDisplayStyleSchema.h>
#include <pxr/imaging/hd/lightSchema.h>
#include <pxr/imaging/hd/materialBindingSchema.h>
#if PXR_VERSION >= 2308
#include <pxr/imaging/hd/materialBindingsSchema.h>
#endif
#include <pxr/imaging/hd/materialConnectionSchema.h>
#include <pxr/imaging/hd/materialNetworkSchema.h>
#include <pxr/imaging/hd/materialNodeSchema.h>
#include <pxr/imaging/hd/materialSchema.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/meshTopologySchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/visibilitySchema.h>
#include <pxr/imaging/hd/volumeFieldSchema.h>
#include <pxr/imaging/hd/xformSchema.h>

PXR_NAMESPACE_OPEN_SCOPE


MayaHydraDataSource::MayaHydraDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraSceneIndex* sceneIndex,
    MayaHydraAdapter* adapter)
    : _id(id)
    , _type(type)
    , _sceneIndex(sceneIndex)
    , _adapter(adapter)
{
}

TfTokenVector
MayaHydraDataSource::GetNames()
{
    TfTokenVector result;

    if (_type == HdPrimTypeTokens->mesh) {
        result.push_back(HdMeshSchemaTokens->mesh);
    }

    if (_type == HdPrimTypeTokens->basisCurves) {
        result.push_back(HdBasisCurvesSchemaTokens->basisCurves);
    }

    result.push_back(HdPrimvarsSchemaTokens->primvars);

    if (HdPrimTypeIsGprim(_type)) {
#if PXR_VERSION < 2308
        result.push_back(HdMaterialBindingSchemaTokens->materialBinding);
#else
        result.push_back(HdMaterialBindingsSchema::GetSchemaToken());
#endif
        result.push_back(HdLegacyDisplayStyleSchemaTokens->displayStyle);
        result.push_back(HdVisibilitySchemaTokens->visibility);
        result.push_back(HdXformSchemaTokens->xform);
    }

    if (HdPrimTypeIsLight(_type)) {
        result.push_back(HdMaterialSchemaTokens->material);
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdLightSchemaTokens->light);
    }

    if (_type == HdPrimTypeTokens->material) {
        result.push_back(HdMaterialSchemaTokens->material);
    }

    if (_type == HdPrimTypeTokens->camera) {
        result.push_back(HdCameraSchemaTokens->camera);
        result.push_back(HdXformSchemaTokens->xform);
    }

    return result;
}

HdDataSourceBaseHandle
MayaHydraDataSource::Get(const TfToken& name)
{
    if (name == HdMeshSchemaTokens->mesh) {
        if (_type == HdPrimTypeTokens->mesh) {
            auto topology = _adapter->GetMeshTopology();
            return HdMeshSchema::Builder()
                .SetTopology(
                    HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
                .SetSubdivisionScheme(
                    HdRetainedTypedSampledDataSource<TfToken>::New(topology.GetScheme()))
                .SetDoubleSided(
                    HdRetainedTypedSampledDataSource<bool>::New(_adapter->GetDoubleSided()))
                .Build();
        }
    }
    else if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        if (_type == HdPrimTypeTokens->basisCurves) {
            auto topology = _adapter->GetBasisCurvesTopology();
            return HdBasisCurvesSchema::Builder()
                .SetTopology(
                    HdBasisCurvesTopologySchema::Builder()
                    .SetCurveVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetCurveVertexCounts()))
                    .SetCurveIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetCurveIndices()))
                    .SetBasis(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveBasis()))
                    .SetType(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveType()))
                    .SetWrap(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveWrap()))
                    .Build())
                .Build();
        }
    }
    else if (name == HdPrimvarsSchemaTokens->primvars) {
        return _GetPrimvarsDataSource();
    }
    else if (name ==
#if PXR_VERSION < 2308
             HdMaterialBindingSchemaTokens->materialBinding
#else
             HdMaterialBindingsSchema::GetSchemaToken()
#endif             
             ) {
       return _GetMaterialBindingDataSource();
    }
    else if (name == HdXformSchemaTokens->xform) {
        auto xform = _adapter->GetTransform();
        return HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(xform))
            .Build();
    }
    else if (name == HdMaterialSchemaTokens->material) {
       return _GetMaterialDataSource();
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
        return MayaHydraDisplayStyleDataSource::New(_id, _type, _sceneIndex, _adapter);
    }
    else if (name == HdVisibilitySchemaTokens->visibility) {
        return _GetVisibilityDataSource();
    }
    else if (name == HdCameraSchemaTokens->camera) {
        return MayaHydraCameraDataSource::New(_id, _type, _adapter);
    }
    else if (name == HdLightSchemaTokens->light) {
        return MayaHydraLightDataSource::New(_id, _type, _adapter);
    }

    return nullptr;
}

HdDataSourceBaseHandle MayaHydraDataSource::_GetVisibilityDataSource()
{
    bool vis = _adapter->GetVisible();
    if (vis) {
        static const HdContainerDataSourceHandle visOn =
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(true));
        return visOn;
    }
    else {
        static const HdContainerDataSourceHandle visOff =
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(false));
        return visOff;
    }
}

HdDataSourceBaseHandle MayaHydraDataSource::_GetPrimvarsDataSource()
{
    if (_primvarsBuilt.load()) {
        return HdContainerDataSource::AtomicLoad(_primvars);
    }

    MayaHydraPrimvarsDataSourceHandle primvarsDs;

    for (size_t interpolation = HdInterpolationConstant;
        interpolation < HdInterpolationCount; ++interpolation) {

        HdPrimvarDescriptorVector v = _adapter->GetPrimvarDescriptors((HdInterpolation)interpolation);

        TfToken interpolationToken = _InterpolationAsToken(
            (HdInterpolation)interpolation);

        for (const auto& primvarDesc : v) {
            if (!primvarsDs) {
                primvarsDs = MayaHydraPrimvarsDataSource::New(_adapter);
            }
            primvarsDs->AddDesc(
                primvarDesc.name, interpolationToken, primvarDesc.role,
                primvarDesc.indexed);
        }
    }

    HdContainerDataSourceHandle ds = primvarsDs;
    HdContainerDataSource::AtomicStore(_primvars, ds);
    _primvarsBuilt.store(true);

    return primvarsDs;
}

TfToken MayaHydraDataSource::_InterpolationAsToken(HdInterpolation interpolation)
{
    switch (interpolation) {
    case HdInterpolationConstant:
        return HdPrimvarSchemaTokens->constant;
    case HdInterpolationUniform:
        return HdPrimvarSchemaTokens->uniform;
    case HdInterpolationVarying:
        return HdPrimvarSchemaTokens->varying;
    case HdInterpolationVertex:
        return HdPrimvarSchemaTokens->vertex;
    case HdInterpolationFaceVarying:
        return HdPrimvarSchemaTokens->faceVarying;
    case HdInterpolationInstance:
        return HdPrimvarSchemaTokens->instance;

    default:
        return HdPrimvarSchemaTokens->constant;
    }
}

HdDataSourceBaseHandle
MayaHydraDataSource::_GetMaterialBindingDataSource()
{
    SdfPath path = _sceneIndex->GetMaterialId(_id);
    if (path.IsEmpty()) {
        return nullptr;
    }
    HdDataSourceBaseHandle bindingPath =
        HdRetainedTypedSampledDataSource<SdfPath>::New(path);

    TfToken t = 
#if PXR_VERSION < 2308
        HdMaterialBindingSchemaTokens->allPurpose;
#else
        HdMaterialBindingsSchemaTokens->allPurpose;
#endif
    HdContainerDataSourceHandle binding =
#if PXR_VERSION < 2308
        HdMaterialBindingSchema::BuildRetained(1, &t, &bindingPath);
#else
        HdMaterialBindingsSchema::BuildRetained(1, &t, &bindingPath);
#endif
    return binding;
}

static bool
_ConvertHdMaterialNetworkToHdDataSources(
    const HdMaterialNetworkMap &hdNetworkMap,
    HdContainerDataSourceHandle *result)
{
    HD_TRACE_FUNCTION();

    TfTokenVector terminalsNames;
    std::vector<HdDataSourceBaseHandle> terminalsValues;
    std::vector<TfToken> nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;

    for (auto const &iter: hdNetworkMap.map) {
        const TfToken &terminalName = iter.first;
        const HdMaterialNetwork &hdNetwork = iter.second;

        if (hdNetwork.nodes.empty()) {
            continue;
        }

        terminalsNames.push_back(terminalName);

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
        for (const HdMaterialNode &node : hdNetwork.nodes) {
            std::vector<TfToken> paramsNames;
            std::vector<HdDataSourceBaseHandle> paramsValues;

            for (const auto &p : node.parameters) {
                paramsNames.push_back(p.first);
                paramsValues.push_back(
                    HdRetainedTypedSampledDataSource<VtValue>::New(p.second)
                );
            }

            // Accumulate array connections to the same input
            TfDenseHashMap<TfToken,
                TfSmallVector<HdDataSourceBaseHandle, 8>, TfToken::HashFunctor> 
                    connectionsMap;

            TfSmallVector<TfToken, 8> cNames;
            TfSmallVector<HdDataSourceBaseHandle, 8> cValues;

            for (const HdMaterialRelationship &rel : hdNetwork.relationships) {
                if (rel.outputId == node.path) {                    
                    TfToken outputPath = rel.inputId.GetToken(); 
                    TfToken outputName = TfToken(rel.inputName.GetString());

                    HdDataSourceBaseHandle c = 
                        HdMaterialConnectionSchema::BuildRetained(
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                outputPath),
                            HdRetainedTypedSampledDataSource<TfToken>::New(
                                outputName));

                    connectionsMap[
                        TfToken(rel.outputName.GetString())].push_back(c);
                }
            }

            cNames.reserve(connectionsMap.size());
            cValues.reserve(connectionsMap.size());

            // NOTE: not const because HdRetainedSmallVectorDataSource needs
            //       a non-const HdDataSourceBaseHandle*
            for (auto &entryPair : connectionsMap) {
                cNames.push_back(entryPair.first);
                cValues.push_back(
                    HdRetainedSmallVectorDataSource::New(
                        entryPair.second.size(), entryPair.second.data()));
            }

            nodeNames.push_back(node.path.GetToken());
            nodeValues.push_back(
                HdMaterialNodeSchema::BuildRetained(
                    HdRetainedContainerDataSource::New(
                        paramsNames.size(), 
                        paramsNames.data(),
                        paramsValues.data()),
                    HdRetainedContainerDataSource::New(
                        cNames.size(), 
                        cNames.data(),
                        cValues.data()),
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        node.identifier),
                    nullptr /*renderContextNodeIdentifiers*/
#if PXR_VERSION >= 2308
                    , nullptr /* nodeTypeInfo */ 
#endif
            ));
        }

        terminalsValues.push_back(
            HdMaterialConnectionSchema::BuildRetained(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    hdNetwork.nodes.back().path.GetToken()),
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    terminalsNames.back()))
                );
    }

    HdContainerDataSourceHandle nodesDefaultContext = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(), 
            nodeNames.data(),
            nodeValues.data());

    HdContainerDataSourceHandle terminalsDefaultContext = 
        HdRetainedContainerDataSource::New(
            terminalsNames.size(), 
            terminalsNames.data(),
            terminalsValues.data());

    // Create the material network, potentially one per network selector
    HdDataSourceBaseHandle network = HdMaterialNetworkSchema::BuildRetained(
        nodesDefaultContext,
        terminalsDefaultContext);
        
    TfToken defaultContext = HdMaterialSchemaTokens->universalRenderContext;
    *result = HdMaterialSchema::BuildRetained(
        1, 
        &defaultContext, 
        &network);
    
    return true;
}

HdDataSourceBaseHandle
MayaHydraDataSource::_GetMaterialDataSource()
{    
    VtValue materialContainer = _sceneIndex->GetMaterialResource(_id);

    if (!materialContainer.IsHolding<HdMaterialNetworkMap>()) {
        return nullptr;
    }

    HdMaterialNetworkMap hdNetworkMap = 
        materialContainer.UncheckedGet<HdMaterialNetworkMap>();
    HdContainerDataSourceHandle materialDS = nullptr;    
    if (!_ConvertHdMaterialNetworkToHdDataSources(
        hdNetworkMap,
        &materialDS) ) {
        return nullptr;
    }
    return materialDS;
}
PXR_NAMESPACE_CLOSE_SCOPE
