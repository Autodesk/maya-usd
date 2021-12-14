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
#ifndef PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADER_OVERRIDE_H
#define PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_SHADER_OVERRIDE_H

#include "api.h"

#include <pxr/pxr.h>

#include <maya/MDrawContext.h>
#include <maya/MPxShaderOverride.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrMayaUsdPreviewSurfaceShaderOverride : public MPxShaderOverride
{
public:
    PXRUSDPREVIEWSURFACE_API
    static MPxShaderOverride* creator(const MObject& obj);

    PXRUSDPREVIEWSURFACE_API
    ~PxrMayaUsdPreviewSurfaceShaderOverride() override;

    PXRUSDPREVIEWSURFACE_API
    MHWRender::DrawAPI supportedDrawAPIs() const override;

    PXRUSDPREVIEWSURFACE_API
    bool rebuildAlways() override;

    PXRUSDPREVIEWSURFACE_API
#if MAYA_API_VERSION >= 20220000
    MString initialize(const MInitContext& initContext, MSharedPtr<MUserData>& userData) override;
#else
    MString initialize(const MInitContext& initContext, MInitFeedback& initFeedback) override;
#endif

    PXRUSDPREVIEWSURFACE_API
    MShaderInstance* nonTexturedShaderInstance(bool& monitorNode) const override;

    PXRUSDPREVIEWSURFACE_API
    void updateDG(MObject obj) override;

    PXRUSDPREVIEWSURFACE_API
    void updateDevice() override;

    PXRUSDPREVIEWSURFACE_API
    bool handlesDraw(MDrawContext& context) override;

    PXRUSDPREVIEWSURFACE_API
    MShaderInstance* shaderInstance(MDrawContext& context) const override;

    PXRUSDPREVIEWSURFACE_API
    void activateKey(MDrawContext& context, const MString& key) override;

    PXRUSDPREVIEWSURFACE_API
    bool draw(MDrawContext& context, const MRenderItemList& renderItemList) const override;

    PXRUSDPREVIEWSURFACE_API
    void terminateKey(MDrawContext& context, const MString& key) override;

private:
    PxrMayaUsdPreviewSurfaceShaderOverride(const MObject& obj);

    void setShaderParams(MShaderInstance*& shaderInstance, bool displayCVP = false);

    MShaderInstance* m_shaderInstance = nullptr;

    MShaderInstance* m_shaderInstanceNonTextured = nullptr;

    float m_diffuseColor[3];
    float m_emissiveColor[3];
    float m_occlusion;
    float m_opacity;
    float m_opacityThreshold;
    float m_ior;
    float m_metallic;
    float m_roughness;
    float m_specularColor[3];
    float m_clearcoat;
    float m_clearcoatRoughness;
    float m_displacement;
    float m_normal[3];
    bool  m_useSpecularWorkflow;
    bool  m_caching;
    bool  m_frozen;
    bool  m_displayCPV;
    bool  m_previousDisplayCPV;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
