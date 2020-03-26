//
// Copyright 2019 Luma Pictures
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
#ifndef MTOH_VIEW_OVERRIDE_UTILS_H
#define MTOH_VIEW_OVERRIDE_UTILS_H

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaSceneRender : public MHWRender::MSceneRender {
public:
    explicit HdMayaSceneRender(const MString& name, bool drawSelectionOverlay)
        : MHWRender::MSceneRender(name),
          _drawSelectionOverlay(drawSelectionOverlay) {}

    MUint64 getObjectTypeExclusions() override {
        return _drawSelectionOverlay
                   ? MHWRender::MSceneRender::getObjectTypeExclusions()
                   : ~(MHWRender::MFrameContext::kExcludeSelectHandles |
                       MHWRender::MFrameContext::kExcludeCameras |
                       MHWRender::MFrameContext::kExcludeCVs |
                       MHWRender::MFrameContext::kExcludeDimensions |
                       MHWRender::MFrameContext::kExcludeLights |
                       MHWRender::MFrameContext::kExcludeLocators |
                       MHWRender::MFrameContext::kExcludeGrid);
    }

    MSceneFilterOption renderFilterOverride() override {
        return _drawSelectionOverlay
                   ? kRenderUIItems
                   : MHWRender::MSceneRender::renderFilterOverride();
    }

    MHWRender::MClearOperation& clearOperation() override {
        auto* renderer = MHWRender::MRenderer::theRenderer();
        const auto gradient = renderer->useGradient();
        const auto color1 = renderer->clearColor();
        const auto color2 = renderer->clearColor2();

        float c1[4] = {color1[0], color1[1], color1[2], 1.0f};
        float c2[4] = {color2[0], color2[1], color2[2], 1.0f};

        mClearOperation.setClearColor(c1);
        mClearOperation.setClearColor2(c2);
        mClearOperation.setClearGradient(gradient);
        return mClearOperation;
    }

    bool _drawSelectionOverlay = false;
};

class HdMayaManipulatorRender : public MHWRender::MSceneRender {
public:
    explicit HdMayaManipulatorRender(const MString& name)
        : MHWRender::MSceneRender(name) {}

    MUint64 getObjectTypeExclusions() override {
        return ~MHWRender::MFrameContext::kExcludeManipulators;
    }

    MHWRender::MClearOperation& clearOperation() override {
        mClearOperation.setMask(MHWRender::MClearOperation::kClearNone);
        return mClearOperation;
    }
};

class HdMayaRender : public MHWRender::MUserRenderOperation {
public:
    HdMayaRender(const MString& name, MtohRenderOverride* override)
        : MHWRender::MUserRenderOperation(name), _override(override) {}

    MStatus execute(const MHWRender::MDrawContext& drawContext) override {
        return _override->Render(drawContext);
    }

private:
    MtohRenderOverride* _override;
};

class HdMayaSetRenderGLState {
public:
    HdMayaSetRenderGLState() {
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &_oldBlendFunc);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &_oldBlendEquation);
        glGetBooleanv(GL_BLEND, &_oldBlend);
        glGetBooleanv(GL_CULL_FACE, &_oldCullFace);

        if (_oldBlendFunc != BLEND_FUNC) {
            glBlendFunc(GL_SRC_ALPHA, BLEND_FUNC);
        }

        if (_oldBlendEquation != BLEND_EQUATION) {
            glBlendEquation(BLEND_EQUATION);
        }

        if (_oldBlend != BLEND) { glEnable(GL_BLEND); }

        if (_oldCullFace != CULL_FACE) { glDisable(GL_CULL_FACE); }
    }

    ~HdMayaSetRenderGLState() {
        if (_oldBlend != BLEND) { glDisable(GL_BLEND); }

        if (_oldBlendFunc != BLEND_FUNC) {
            glBlendFunc(GL_SRC_ALPHA, _oldBlendFunc);
        }

        if (_oldBlendEquation != BLEND_EQUATION) {
            glBlendEquation(_oldBlendEquation);
        }

        if (_oldCullFace != CULL_FACE) { glEnable(GL_CULL_FACE); }
    }

private:
    // non-odr
    constexpr static int BLEND_FUNC = GL_ONE_MINUS_SRC_ALPHA;
    constexpr static int BLEND_EQUATION = GL_FUNC_ADD;
    constexpr static GLboolean BLEND = GL_TRUE;
    constexpr static GLboolean CULL_FACE = GL_FALSE;

    int _oldBlendFunc = BLEND_FUNC;
    int _oldBlendEquation = BLEND_EQUATION;
    GLboolean _oldBlend = BLEND;
    GLboolean _oldCullFace = CULL_FACE;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_VIEW_OVERRIDE_UTILS_H
