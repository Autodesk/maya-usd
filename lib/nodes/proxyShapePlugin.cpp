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
#include "proxyShapePlugin.h"

#include "../render/pxrUsdMayaGL/hdImagingShapeDrawOverride.h"
#include "../render/pxrUsdMayaGL/hdImagingShapeUI.h"
#include "../render/pxrUsdMayaGL/proxyDrawOverride.h"
#include "../render/pxrUsdMayaGL/proxyShapeUI.h"
#include "../render/vp2RenderDelegate/proxyRenderDelegate.h"
#include "../render/vp2ShaderFragments/shaderFragments.h"

#include "stageData.h"
#include "hdImagingShape.h"
#include "proxyShapeBase.h"

#include "pxr/base/tf/envSetting.h"

#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
const MString _RegistrantId("mayaUsd");
int _registrationCount = 0;

// Name of the plugin registering the proxy shape base class.
MString _registrantPluginName;

bool _useVP2RenderDelegate = false;

TF_DEFINE_ENV_SETTING(VP2_RENDER_DELEGATE_PROXY, false,
    "Switch proxy shape rendering to VP2 render delegate.");
}

PXR_NAMESPACE_OPEN_SCOPE

/* static */
MStatus
MayaUsdProxyShapePlugin::initialize(MFnPlugin& plugin)
{
    // If we're already registered, do nothing.
    if (_registrationCount++ > 0) {
        return MS::kSuccess;
    }

    _registrantPluginName = plugin.name();

    _useVP2RenderDelegate = TfGetEnvSetting(VP2_RENDER_DELEGATE_PROXY);

    MStatus status;

    // Proxy shape initialization.
    status = plugin.registerData(
        MayaUsdStageData::typeName,
        MayaUsdStageData::mayaTypeId,
        MayaUsdStageData::creator);
    CHECK_MSTATUS(status);

    status = plugin.registerShape(
        MayaUsdProxyShapeBase::typeName,
        MayaUsdProxyShapeBase::typeId,
        MayaUsdProxyShapeBase::creator,
        MayaUsdProxyShapeBase::initialize,
        nullptr,
        getProxyShapeClassification());
    CHECK_MSTATUS(status);

    // Hybrid Hydra / VP2 rendering uses a draw override to draw the proxy
    // shape.  The Pixar and MayaUsd plugins use the UsdMayaProxyDrawOverride,
    // so register it here.  Native USD VP2 rendering uses a sub-scene override.
    if (_useVP2RenderDelegate) {
        status = MHWRender::MDrawRegistry::registerSubSceneOverrideCreator(
            ProxyRenderDelegate::drawDbClassification,
            _RegistrantId,
            ProxyRenderDelegate::Creator);
        CHECK_MSTATUS(status);
    } else {
        status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
            UsdMayaProxyDrawOverride::drawDbClassification,
            _RegistrantId,
            UsdMayaProxyDrawOverride::Creator);
        CHECK_MSTATUS(status);

        status = plugin.registerDisplayFilter(
            MayaUsdProxyShapeBase::displayFilterName,
            MayaUsdProxyShapeBase::displayFilterLabel,
            UsdMayaProxyDrawOverride::drawDbClassification);
        CHECK_MSTATUS(status);

        // Hybrid Hydra / VP2 rendering uses the PxrMayaHdImagingShape for draw
        // aggregation of all proxy shapes.
        status = plugin.registerShape(
            PxrMayaHdImagingShape::typeName,
            PxrMayaHdImagingShape::typeId,
            PxrMayaHdImagingShape::creator,
            PxrMayaHdImagingShape::initialize,
            PxrMayaHdImagingShapeUI::creator,
            &PxrMayaHdImagingShapeDrawOverride::drawDbClassification);
        CHECK_MSTATUS(status);

        status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
            PxrMayaHdImagingShapeDrawOverride::drawDbClassification,
            _RegistrantId,
            PxrMayaHdImagingShapeDrawOverride::creator);
        CHECK_MSTATUS(status);
    }

    status = HdVP2ShaderFragments::registerFragments();
    CHECK_MSTATUS(status);

    return status;
}

/* static */
MStatus
MayaUsdProxyShapePlugin::finalize(MFnPlugin& plugin)
{
    // If more than one plugin still has us registered, do nothing.
    if (_registrationCount-- > 1) {
        return MS::kSuccess;
    }

    // Maya requires deregistration to be done by the same plugin that
    // performed the registration.  If this isn't possible, warn and don't
    // deregister.
    if (plugin.name() != _registrantPluginName) {
        MGlobal::displayWarning(
            "USD proxy shape base cannot be deregistered, registering plugin "
            + _registrantPluginName + " is unloaded.");
        return MS::kSuccess;
    }

    MStatus status = HdVP2ShaderFragments::deregisterFragments();
    CHECK_MSTATUS(status);
    
    if (_useVP2RenderDelegate) {
        status = MHWRender::MDrawRegistry::deregisterSubSceneOverrideCreator(
            ProxyRenderDelegate::drawDbClassification,
            _RegistrantId);
        CHECK_MSTATUS(status);
    }
    else
    {
        status = plugin.deregisterDisplayFilter(
            MayaUsdProxyShapeBase::displayFilterName);
        CHECK_MSTATUS(status);

        status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
            UsdMayaProxyDrawOverride::drawDbClassification,
            _RegistrantId);
        CHECK_MSTATUS(status);

        status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
            PxrMayaHdImagingShapeDrawOverride::drawDbClassification,
            _RegistrantId);
        CHECK_MSTATUS(status);

        status = plugin.deregisterNode(PxrMayaHdImagingShape::typeId);
        CHECK_MSTATUS(status);
    }

    status = plugin.deregisterNode(MayaUsdProxyShapeBase::typeId);
    CHECK_MSTATUS(status);
    
    status = plugin.deregisterData(MayaUsdStageData::mayaTypeId);
    CHECK_MSTATUS(status);

    return status;
}

const MString* MayaUsdProxyShapePlugin::getProxyShapeClassification()
{
    return _useVP2RenderDelegate ? &ProxyRenderDelegate::drawDbClassification : 
        &UsdMayaProxyDrawOverride::drawDbClassification;
}

bool MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()
{
    return _useVP2RenderDelegate;
}

PXR_NAMESPACE_CLOSE_SCOPE
