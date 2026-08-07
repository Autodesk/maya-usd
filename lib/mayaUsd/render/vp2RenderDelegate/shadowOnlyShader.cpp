//
// Copyright 2026 Autodesk
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
#include "shadowOnlyShader.h"

#include "debugCodes.h"

#include <mayaUsd/utils/blockSceneModificationContext.h>

#include <pxr/base/tf/debug.h>

#include <maya/MDGModifier.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawRegistry.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxNode.h>
#include <maya/MPxShaderOverride.h>
#include <maya/MStringArray.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

const MString kRegistrantId("mayaUsdShadowOnlyShader");

//! The node itself carries no state; all the behaviour is in the override.
class ShadowOnlyShaderNode : public MPxNode
{
public:
    static void*   creator() { return new ShadowOnlyShaderNode(); }
    static MStatus initialize() { return MS::kSuccess; }

    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }
};

bool _HasSemantic(const MStringArray& semantics, const MString& wanted)
{
    for (unsigned int i = 0; i < semantics.length(); ++i) {
        if (semantics[i] == wanted) {
            return true;
        }
    }
    return false;
}

class ShadowOnlyShaderOverride : public MHWRender::MPxShaderOverride
{
public:
    static MHWRender::MPxShaderOverride* creator(const MObject& obj)
    {
        return new ShadowOnlyShaderOverride(obj);
    }

    MHWRender::DrawAPI supportedDrawAPIs() const override
    {
        return MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11 | MHWRender::kOpenGL;
    }

    /// Decline shadow passes so Maya renders the item into the shadow map with
    /// its own pass shader; claim everything else so nothing is drawn to camera.
    bool handlesDraw(MHWRender::MDrawContext& context) override
    {
        const MStringArray& semantics = context.getPassContext().passSemantics();

        const bool shadowPass
            = _HasSemantic(semantics, MHWRender::MPassContext::kShadowPassSemantic)
            || _HasSemantic(semantics, MHWRender::MPassContext::kPointLightShadowPassSemantic);

        TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
            .Msg(
                "ShadowOnlyShaderOverride::handlesDraw: %u semantic(s), first '%s', shadowPass=%d "
                "-> handles=%d\n",
                semantics.length(),
                semantics.length() > 0 ? semantics[0].asChar() : "",
                static_cast<int>(shadowPass),
                static_cast<int>(!shadowPass));

        return !shadowPass;
    }

    /// Claim the draw without emitting geometry. Returning true stops Maya from
    /// falling back to its unsupported-material drawing.
    bool draw(MHWRender::MDrawContext&, const MHWRender::MRenderItemList&) const override
    {
        return true;
    }

    // Opaque on purpose: transparent items are excluded from the shadow map.
    bool isTransparent() override { return false; }

    bool overridesDrawState() override { return false; }

private:
    explicit ShadowOnlyShaderOverride(const MObject& obj)
        : MHWRender::MPxShaderOverride(obj)
    {
    }
};

MObjectHandle _sharedNode;

} // namespace

const MTypeId ShadowOnlyShader::typeId(0x580000A7);
const char*   ShadowOnlyShader::typeName = "mayaUsdShadowOnlyShader";
const char*   ShadowOnlyShader::classification = "drawdb/shader/surface/mayaUsdShadowOnly";

/* static */
void* ShadowOnlyShader::nodeCreator() { return ShadowOnlyShaderNode::creator(); }

/* static */
MStatus ShadowOnlyShader::nodeInitialize() { return ShadowOnlyShaderNode::initialize(); }

/* static */
MStatus ShadowOnlyShader::registerOverride()
{
    return MHWRender::MDrawRegistry::registerShaderOverrideCreator(
        classification, kRegistrantId, ShadowOnlyShaderOverride::creator);
}

/* static */
MStatus ShadowOnlyShader::deregisterOverride()
{
    _sharedNode = MObjectHandle();
    return MHWRender::MDrawRegistry::deregisterShaderOverrideCreator(
        classification, kRegistrantId);
}

/* static */
MStatus ShadowOnlyShader::ensureSharedNode()
{
    if (_sharedNode.isValid()) {
        return MS::kSuccess;
    }

    // This node is an implementation detail of cameraVisibility, created lazily
    // from the draw path. Without this the mere act of drawing a USD stage would
    // flag the Maya scene as having unsaved changes. Same reasoning as
    // PxrMayaHdImagingShape; see hdImagingShape.cpp.
    const MayaUsd::utils::BlockSceneModificationContext blockModContext;

    MStatus       status;
    MDGModifier   modifier;
    const MObject node = modifier.createNode(typeId, &status);
    if (!status || node.isNull()) {
        TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
            .Msg("ShadowOnlyShader: could not create the shared node; is the type registered?\n");
        return status ? MStatus(MS::kFailure) : status;
    }

    status = modifier.doIt();
    if (!status) {
        return status;
    }

    MFnDependencyNode depNodeFn(node);
    depNodeFn.setName("mayaUsdShadowOnlyShader1");
    // Never write it to the scene file: it is recreated on demand, and a saved
    // copy would reload as an unknown node wherever the plugin is not loaded.
    CHECK_MSTATUS(depNodeFn.setDoNotWrite(true));

    _sharedNode = MObjectHandle(node);

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS).Msg("ShadowOnlyShader: created the shared node\n");
    return MS::kSuccess;
}

/* static */
MObject ShadowOnlyShader::getSharedNode()
{
    return _sharedNode.isValid() ? _sharedNode.object() : MObject::kNullObj;
}

} // namespace MAYAUSD_NS_DEF
