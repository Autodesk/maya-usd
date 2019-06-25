// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#include "pxr/base/tf/envSetting.h"

#include "base/api.h"
#include "importTranslator.h"

#include <mayaUsdCore/renderers/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsdCore/nodes/proxyShapeBase.h>
#include <mayaUsdCore/nodes/stageData.h>

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>
#include <maya/MDrawRegistry.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    const MString _RegistrantId("mayaUsdCore");
    bool _useVP2RenderDelegate = false;

    TF_DEFINE_ENV_SETTING(VP2_RENDER_DELEGATE_PROXY, false,
        "Switch proxy shape rendering to VP2 render delegate.");
}

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MStatus status;
    
    MFnPlugin plugin(obj, "Autodesk", "1.0", "Any");

    status = plugin.registerFileTranslator(
        "mayaUsdImport",
        "",
        UsdMayaImportTranslator::creator,
        "usdTranslatorImport", // options script name
        const_cast<char*>(UsdMayaImportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register import translator.");
    }

    _useVP2RenderDelegate = TfGetEnvSetting(VP2_RENDER_DELEGATE_PROXY);

    status = plugin.registerData(
        UsdMayaStageData::typeName,
        UsdMayaStageData::mayaTypeId,
        UsdMayaStageData::creator);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register stage data.");
    }

    const MString* drawClassification = nullptr;
    
    // Hybrid Hydra / VP2 rendering uses a draw override to draw the proxy
    // shape.  The Pixar and MayaUsd plugins use the UsdMayaProxyDrawOverride,
    // so register it here.  Native USD VP2 rendering uses a sub-scene override.
    if (_useVP2RenderDelegate) {
        status = MHWRender::MDrawRegistry::registerSubSceneOverrideCreator(
            ProxyRenderDelegate::drawDbClassification,
            _RegistrantId,
            ProxyRenderDelegate::Creator);
        if (!status) {
            status.perror("mayaUsdPlugin: unable to register proxy render delegate.");
        }

        drawClassification = &ProxyRenderDelegate::drawDbClassification;
    }

    status = plugin.registerShape(
        MayaUsdProxyShapeBase::typeName,
        MayaUsdProxyShapeBase::typeId,
        MayaUsdProxyShapeBase::creator,
        MayaUsdProxyShapeBase::initialize,
        nullptr,
        drawClassification);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register proxy shape.");
    }

    return status;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MStatus status;

    status = plugin.deregisterFileTranslator("mayaUsdImport");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister import translator.");
    }


    if (_useVP2RenderDelegate) {
        status = MHWRender::MDrawRegistry::deregisterSubSceneOverrideCreator(
            ProxyRenderDelegate::drawDbClassification,
            _RegistrantId);
        CHECK_MSTATUS(status);
    }

    status = plugin.deregisterNode(MayaUsdProxyShapeBase::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterData(UsdMayaStageData::mayaTypeId);
    CHECK_MSTATUS(status);

    return status;
}
