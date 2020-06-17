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

#include "usdPreviewSurface.h"
#include "usdPreviewSurfaceShadingNodeOverride.h"

#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>

#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>

#include <pxr/base/tf/envSetting.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
const MString _RegistrantId("mayaUsd");
int _registrationCount = 0;

// Name of the plugin registering the preview surface class.
MString _registrantPluginName;

}

PXR_NAMESPACE_OPEN_SCOPE

/* static */
MStatus
PxrMayaUsdPreviewSurfacePlugin::initialize(MFnPlugin& plugin)
{
    // If we're already registered, do nothing.
    if (_registrationCount++ > 0) {
        return MS::kSuccess;
    }

    _registrantPluginName = plugin.name();

    MStatus status = plugin.registerNode(
        PxrMayaUsdPreviewSurface::typeName,
        PxrMayaUsdPreviewSurface::typeId,
        PxrMayaUsdPreviewSurface::creator,
        PxrMayaUsdPreviewSurface::initialize,
        MPxNode::kDependNode,
        &PxrMayaUsdPreviewSurface::fullClassification);
    CHECK_MSTATUS(status);

    status =
        MHWRender::MDrawRegistry::registerSurfaceShadingNodeOverrideCreator(
            PxrMayaUsdPreviewSurface::drawDbClassification,
            _RegistrantId,
            PxrMayaUsdPreviewSurfaceShadingNodeOverride::creator);
    CHECK_MSTATUS(status);

    return status;
}

/* static */
MStatus
PxrMayaUsdPreviewSurfacePlugin::finalize(MFnPlugin& plugin)
{
    // If more than one plugin still has us registered, do nothing.
    if (_registrationCount-- > 1) {
        return MS::kSuccess;
    }

    deregisterFragments();

    // Maya requires deregistration to be done by the same plugin that
    // performed the registration.  If this isn't possible, warn and don't
    // deregister.
    if (plugin.name() != _registrantPluginName) {
        MGlobal::displayWarning(
            "USD preview surface base cannot be deregistered, registering plugin "
            + _registrantPluginName + " is unloaded.");
        return MS::kSuccess;
    }

    MStatus status =
        MHWRender::MDrawRegistry::deregisterSurfaceShadingNodeOverrideCreator(
            PxrMayaUsdPreviewSurface::drawDbClassification,
            _RegistrantId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(PxrMayaUsdPreviewSurface::typeId);
    CHECK_MSTATUS(status);

    return status;
}

namespace {
    bool _registered = false;
}

/* static */
MStatus
PxrMayaUsdPreviewSurfacePlugin::registerFragments() {
    if (_registered) {
        return MS::kSuccess;
    }

    MStatus status = HdVP2ShaderFragments::registerFragments();
    _registered = true;
    return status;
}

/* static */
MStatus
PxrMayaUsdPreviewSurfacePlugin::deregisterFragments() {
    if (!_registered) {
        return MS::kSuccess;
    }

    MStatus status = HdVP2ShaderFragments::deregisterFragments();
    _registered = false;
    return status;
}

PXR_NAMESPACE_CLOSE_SCOPE
