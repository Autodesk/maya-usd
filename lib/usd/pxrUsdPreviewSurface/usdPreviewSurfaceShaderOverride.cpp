//
// Copyright 2021 Animal Logic
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

#include "usdPreviewSurfaceShaderOverride.h"

#include "usdPreviewSurfacePlugin.h"

#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>

#include <pxr/base/tf/staticTokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MShaderManager.h>

PXR_NAMESPACE_OPEN_SCOPE

static MStatus populateShaderManager(const MShaderManager* shaderMgr)
{
    MRenderer* renderer = MRenderer::theRenderer();
    if (renderer) {
        shaderMgr = renderer->getShaderManager();
        if (shaderMgr)
            return MS::kSuccess;
    }
    TF_RUNTIME_ERROR("Failed to populate shader manager.");
    return MS::kFailure;
}

inline static MShaderInstance* getPreviewSurfaceShader(const MShaderManager* shaderMgr)
{
    return shaderMgr->getFragmentShader(
        HdVP2ShaderFragmentsTokens->SurfaceFragmentGraphName.GetText(), "outSurfaceFinal", true);
}

inline static MShaderInstance*
cloneOrCreateInstance(const MShaderManager* shaderMgr, MShaderInstance* instance)
{
    // Clone the shader instance or create a new one
    return instance ? instance->clone() : getPreviewSurfaceShader(shaderMgr);
}

MPxShaderOverride* PxrMayaUsdPreviewSurfaceShaderOverride::creator(const MObject& obj)
{
    // Make sure the shader fragments have been registered before instantiating the override
    PxrMayaUsdPreviewSurfacePlugin::registerFragments();
    return new PxrMayaUsdPreviewSurfaceShaderOverride(obj);
}

PxrMayaUsdPreviewSurfaceShaderOverride::PxrMayaUsdPreviewSurfaceShaderOverride(const MObject& obj)
    : MPxShaderOverride(obj)
{
}

PxrMayaUsdPreviewSurfaceShaderOverride::~PxrMayaUsdPreviewSurfaceShaderOverride()
{
    // Release and clear all held shader instances
    const MShaderManager* shaderMgr = nullptr;
    if (populateShaderManager(shaderMgr)) {
        if (m_shaderInstanceNonTextured) {
            shaderMgr->releaseShader(m_shaderInstanceNonTextured);
            m_shaderInstanceNonTextured = nullptr;
        }
        if (m_shaderInstance) {
            shaderMgr->releaseShader(m_shaderInstance);
            m_shaderInstance = nullptr;
        }
    }
}

MHWRender::DrawAPI PxrMayaUsdPreviewSurfaceShaderOverride::supportedDrawAPIs() const
{
    return MHWRender::kAllDevices;
}

bool PxrMayaUsdPreviewSurfaceShaderOverride::rebuildAlways() { return true; }

#if MAYA_API_VERSION >= 20220000
MString PxrMayaUsdPreviewSurfaceShaderOverride::initialize(
    const MInitContext&    initContext,
    MSharedPtr<MUserData>& data)
#else
MString PxrMayaUsdPreviewSurfaceShaderOverride::initialize(
    const MInitContext& initContext,
    MInitFeedback&      data)
#endif
{
    // Define the geometry requirements for the mayaCPVInput fragment
    addGeometryRequirement({ {}, MGeometry::kNormal, MGeometry::kFloat, 3 });
    addGeometryRequirement({ {}, MGeometry::kColor, MGeometry::kFloat, 4 });

    // Create instances of the USD preview surface fragment shader
    const MShaderManager* shaderMgr = nullptr;
    if (populateShaderManager(shaderMgr)) {
        if (!m_shaderInstanceNonTextured)
            m_shaderInstanceNonTextured = getPreviewSurfaceShader(shaderMgr);

        if (!m_shaderInstance) {
            m_shaderInstance = cloneOrCreateInstance(shaderMgr, m_shaderInstanceNonTextured);
        }
    }
    return MPxShaderOverride::initialize(initContext, data);
}

MShaderInstance*
PxrMayaUsdPreviewSurfaceShaderOverride::nonTexturedShaderInstance(bool& monitorNode) const
{
    monitorNode = true; // TODO why is this not running the update loop??
    return m_shaderInstanceNonTextured;
}

void PxrMayaUsdPreviewSurfaceShaderOverride::updateDG(MObject obj)
{
    // Update the cached attributes for the node
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (!status)
        return;

    node.findPlug("diffuseColorR", true).getValue(m_diffuseColor[0]);
    node.findPlug("diffuseColorG", true).getValue(m_diffuseColor[1]);
    node.findPlug("diffuseColorB", true).getValue(m_diffuseColor[2]);

    node.findPlug("emissiveColorR", true).getValue(m_emissiveColor[0]);
    node.findPlug("emissiveColorG", true).getValue(m_emissiveColor[1]);
    node.findPlug("emissiveColorB", true).getValue(m_emissiveColor[2]);

    node.findPlug("occlusion", true).getValue(m_occlusion);
    node.findPlug("opacity", true).getValue(m_opacity);
    node.findPlug("opacityThreshold", true).getValue(m_opacityThreshold);
    node.findPlug("ior", true).getValue(m_ior);
    node.findPlug("metallic", true).getValue(m_metallic);
    node.findPlug("roughness", true).getValue(m_roughness);

    node.findPlug("specularColorR", true).getValue(m_specularColor[0]);
    node.findPlug("specularColorG", true).getValue(m_specularColor[1]);
    node.findPlug("specularColorB", true).getValue(m_specularColor[2]);

    node.findPlug("clearcoat", true).getValue(m_clearcoat);
    node.findPlug("clearcoatRoughness", true).getValue(m_clearcoatRoughness);
    node.findPlug("displacement", true).getValue(m_displacement);

    node.findPlug("normal0", true).getValue(m_normal[0]);
    node.findPlug("normal1", true).getValue(m_normal[1]);
    node.findPlug("normal2", true).getValue(m_normal[2]);

    // Store previous display value
    m_previousDisplayCPV = m_displayCPV;
    node.findPlug("displayCPV", true).getValue(m_displayCPV);
    node.findPlug("useSpecularWorkflow", true).getValue(m_useSpecularWorkflow);
    node.findPlug("caching", true).getValue(m_caching);
    node.findPlug("frozen", true).getValue(m_frozen);
}

void PxrMayaUsdPreviewSurfaceShaderOverride::updateDevice()
{
    // Recreate the textured shader if the CPV display value changed or does not exist
    const MShaderManager* shaderMgr = nullptr;
    if ((m_displayCPV != m_previousDisplayCPV || !m_shaderInstance)
        && populateShaderManager(shaderMgr)) {
        if (m_shaderInstance) {
            shaderMgr->releaseShader(m_shaderInstance);
            m_shaderInstance = nullptr;
        }

        // Rebuild the textured shader
        m_shaderInstance = cloneOrCreateInstance(shaderMgr, m_shaderInstanceNonTextured);
        if (!m_shaderInstance)
            TF_RUNTIME_ERROR("Failed to recreate textured shader instance.");
    }

    // Update the parameters for each shader instance
    setShaderParams(m_shaderInstanceNonTextured);
    setShaderParams(m_shaderInstance, m_displayCPV);
}

void PxrMayaUsdPreviewSurfaceShaderOverride::setShaderParams(
    MShaderInstance*& shaderInstance,
    bool              displayCVP)
{
    if (!shaderInstance)
        return;

    if (displayCVP) {
        // Set the CPV inputs to the shader instance
        shaderInstance->addInputFragment("mayaCPVInput", "outColor", "diffuseColor");
        shaderInstance->addInputFragment("mayaCPVInput", "outTransparency", "dummyTransparency");
        shaderInstance->setIsTransparent(true);
    } else {
        shaderInstance->setParameter("diffuseColor", &m_diffuseColor[0]);
        shaderInstance->setParameter("dummyTransparency", 0.f);
        shaderInstance->setIsTransparent(false);
    }

    // Copy the cached attributes from the node to the shader instance
    shaderInstance->setParameter("emissiveColor", &m_emissiveColor[0]);
    shaderInstance->setParameter("occlusion", m_occlusion);
    shaderInstance->setParameter("opacity", m_opacity);
    shaderInstance->setParameter("opacityThreshold", m_opacityThreshold);
    shaderInstance->setParameter("ior", m_ior);
    shaderInstance->setParameter("metallic", m_metallic);
    shaderInstance->setParameter("roughness", m_roughness);
    shaderInstance->setParameter("specularColor", &m_specularColor[0]);
    shaderInstance->setParameter("clearcoat", m_clearcoat);
    shaderInstance->setParameter("clearcoatRoughness", m_clearcoatRoughness);
    shaderInstance->setParameter("displacement", m_displacement);
    shaderInstance->setParameter("normal", &m_normal[0]);
    shaderInstance->setParameter("useSpecularWorkflow", m_useSpecularWorkflow);
    shaderInstance->setParameter("caching", m_caching);
    shaderInstance->setParameter("frozen", m_frozen);
}

bool PxrMayaUsdPreviewSurfaceShaderOverride::handlesDraw(MDrawContext& context)
{
    // Handle draw on non-overriden color passes
    const MPassContext& passCtx = context.getPassContext();
    const MStringArray& passSemantics = passCtx.passSemantics();
    for (const MString& semantic : passSemantics) {
        if (semantic == MPassContext::kColorPassSemantic && !passCtx.hasShaderOverride()) {
            return true;
        }
    }
    return false;
}

MShaderInstance* PxrMayaUsdPreviewSurfaceShaderOverride::shaderInstance(MDrawContext& context) const
{
    return m_shaderInstance;
}

void PxrMayaUsdPreviewSurfaceShaderOverride::activateKey(MDrawContext& context, const MString& key)
{
    if (!m_shaderInstance)
        return;
    m_shaderInstance->updateParameters(context);
    m_shaderInstance->bind(context);
}

bool PxrMayaUsdPreviewSurfaceShaderOverride::draw(
    MDrawContext&          context,
    const MRenderItemList& renderItemList) const
{
    if (m_shaderInstance) {
        const unsigned int passCount = m_shaderInstance->getPassCount(context);
        for (unsigned int i = 0; i < passCount; i++) {
            m_shaderInstance->activatePass(context, i);
            MPxShaderOverride::drawGeometry(context);
        }
    }
    return true;
}

void PxrMayaUsdPreviewSurfaceShaderOverride::terminateKey(MDrawContext& context, const MString& key)
{
    if (m_shaderInstance)
        m_shaderInstance->unbind(context);
}

PXR_NAMESPACE_CLOSE_SCOPE
