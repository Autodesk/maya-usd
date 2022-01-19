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
#include "usdPreviewSurfacePlugin.h"

#include "cpvColor.h"
#include "usdPreviewSurface.h"
#include "usdPreviewSurfaceReader.h"
#include "usdPreviewSurfaceShadingNodeOverride.h"
#include "usdPreviewSurfaceWriter.h"

#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
TfToken::Set         _registeredTypeNames;
static const MString cpvColorShaderName("cpvColor");
static const MString cpvColorShaderUserClassification("texture/2d:");
static const MString cpvColorShaderDrawClassification("drawdb/shader/texture/2d/");
} // namespace
/* static */
MStatus PxrMayaUsdPreviewSurfacePlugin::initialize(
    MFnPlugin&     plugin,
    const MString& typeName,
    MTypeId        typeId,
    const MString& registrantId)
{
    TfToken typeNameToken(typeName.asChar());
    if (_registeredTypeNames.count(typeNameToken) > 0) {
        TF_CODING_ERROR("Trying to register typeName %s more than once", typeNameToken.GetText());
        return MStatus::kFailure;
    }
    _registeredTypeNames.insert(typeNameToken);

    MString drawDbClassification(
        TfStringPrintf("drawdb/shader/surface/%s", typeName.asChar()).c_str());
    MString fullClassification(
        TfStringPrintf("shader/surface:shader/displacement:%s", drawDbClassification.asChar())
            .c_str());

    MStatus status = plugin.registerNode(
        typeName,
        typeId,
        PxrMayaUsdPreviewSurface::creator,
        PxrMayaUsdPreviewSurface::initialize,
        MPxNode::kDependNode,
        &fullClassification);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(
        drawDbClassification, registrantId, PxrMayaUsdPreviewSurfaceShadingNodeOverride::creator);
    CHECK_MSTATUS(status);

    // Register CPV shader node
    const MString cpvDrawClassify(cpvColorShaderDrawClassification + cpvColorShaderName);
    MString       cpvUserClassify(cpvColorShaderUserClassification);
    cpvUserClassify += cpvDrawClassify;

    status = plugin.registerNode(
        cpvColorShaderName,
        CPVColor::id,
        CPVColor::creator,
        CPVColor::initialize,
        MPxNode::kDependNode,
        &cpvUserClassify);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::registerShadingNodeOverrideCreator(
        cpvDrawClassify, registrantId, CPVColorShadingNodeOverride::creator);
    CHECK_MSTATUS(status);

    return status;
}

/* static */
MStatus PxrMayaUsdPreviewSurfacePlugin::finalize(
    MFnPlugin&     plugin,
    const MString& typeName,
    MTypeId        typeId,
    const MString& registrantId)
{
    TfToken typeNameToken(typeName.asChar());
    if (_registeredTypeNames.count(typeNameToken) == 0) {
        TF_CODING_ERROR("TypeName %s is not currently registered", typeNameToken.GetText());
        return MStatus::kFailure;
    }
    _registeredTypeNames.erase(typeNameToken);

    MString drawDbClassification(
        TfStringPrintf("drawdb/shader/surface/%s", typeName.asChar()).c_str());

    deregisterFragments();

    const MString cpvDrawClassify(cpvColorShaderDrawClassification + cpvColorShaderName);
    MStatus status = plugin.deregisterNode(CPVColor::id);
    CHECK_MSTATUS(status);
    status = MHWRender::MDrawRegistry::deregisterShadingNodeOverrideCreator(
        cpvDrawClassify, registrantId);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(
        drawDbClassification, registrantId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(typeId);
    CHECK_MSTATUS(status);

    return status;
}

namespace {
bool _registered = false;
}

/* static */
MStatus PxrMayaUsdPreviewSurfacePlugin::registerFragments()
{
    if (_registered) {
        return MS::kSuccess;
    }

    MStatus status = HdVP2ShaderFragments::registerFragments();
    _registered = true;
    return status;
}

/* static */
MStatus PxrMayaUsdPreviewSurfacePlugin::deregisterFragments()
{
    if (!_registered) {
        return MS::kSuccess;
    }

    MStatus status = HdVP2ShaderFragments::deregisterFragments();
    _registered = false;
    return status;
}

/* static */
void PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceReader(const MString& typeName)
{
    TfToken typeNameToken(typeName.asChar());

    // There is obvious ambiguity here as soon as two plugins register a UsdPreviewSurface node.
    // First registered will be the one used for import.
    UsdMayaShaderReaderRegistry::Register(
        UsdImagingTokens->UsdPreviewSurface,
        PxrMayaUsdPreviewSurface_Reader::CanImport,
        [typeNameToken](const UsdMayaPrimReaderArgs& readerArgs) {
            return std::make_shared<PxrMayaUsdPreviewSurface_Reader>(readerArgs, typeNameToken);
        });
}

/* static */
void PxrMayaUsdPreviewSurfacePlugin::RegisterPreviewSurfaceWriter(const MString& typeName)
{
    TfToken typeNameToken(typeName.asChar());

    UsdMayaShaderWriterRegistry::Register(
        typeNameToken,
        &PxrMayaUsdPreviewSurface_Writer::CanExport,
        [](const MFnDependencyNode& depNodeFn,
           const SdfPath&           usdPath,
           UsdMayaWriteJobContext&  jobCtx) {
            return std::make_shared<PxrMayaUsdPreviewSurface_Writer>(depNodeFn, usdPath, jobCtx);
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
