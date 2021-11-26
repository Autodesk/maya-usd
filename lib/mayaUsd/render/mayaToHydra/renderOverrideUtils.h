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

#include <pxr/pxr.h>

#include <maya/MViewport2Renderer.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaPreRender : public MHWRender::MSceneRender
{
public:
    explicit HdMayaPreRender(const MString& name)
        : MHWRender::MSceneRender(name)
    {
        auto*      renderer = MHWRender::MRenderer::theRenderer();
        const auto gradient = renderer->useGradient();
        const auto color1 = renderer->clearColor();
        const auto color2 = renderer->clearColor2();

        float c1[4] = { color1[0], color1[1], color1[2], 1.0f };
        float c2[4] = { color2[0], color2[1], color2[2], 1.0f };

        mClearOperation.setClearColor(c1);
        mClearOperation.setClearColor2(c2);
        mClearOperation.setClearGradient(gradient);
    }

    MSceneFilterOption renderFilterOverride() override { return kRenderPreSceneUIItems; }

    MHWRender::MClearOperation& clearOperation() override { return mClearOperation; }
};

class HdMayaPostRender : public MHWRender::MSceneRender
{
public:
    explicit HdMayaPostRender(const MString& name)
        : MHWRender::MSceneRender(name)
    {
        mClearOperation.setMask(MHWRender::MClearOperation::kClearNone);
    }

    MUint64 getObjectTypeExclusions() override
    {
        // FIXME:
        //   1. kExcludePluginShapes is here so as to not re-draw UsdProxy shapes
        //      ...but that means no plugin shapes would be drawn.
        //   2. Curves as controls and curves as a renderitem need to be delineated
        //
        return MFrameContext::kExcludeMeshes | MFrameContext::kExcludePluginShapes;
    }

    MSceneFilterOption renderFilterOverride() override
    {
        return MSceneFilterOption(kRenderShadedItems | kRenderPostSceneUIItems);
    }

    MHWRender::MClearOperation& clearOperation() override { return mClearOperation; }
};

class HdMayaRender : public MHWRender::MUserRenderOperation
{
public:
    HdMayaRender(const MString& name, MtohRenderOverride* override)
        : MHWRender::MUserRenderOperation(name)
        , _override(override)
    {
    }

    MStatus execute(const MHWRender::MDrawContext& drawContext) override
    {
        return _override->Render(drawContext);
    }

private:
    MtohRenderOverride* _override;
};

struct HdMayaGLBackup
{
    GLint RestoreFramebuffer = 0;
    GLint RestoreDrawFramebuffer = 0;
    GLint RestoreReadFramebuffer = 0;
};

class HdMayaBackupGLStateTask : public HdTask
{
    using base = HdTask;
    static const SdfPath& _Id()
    {
        static SdfPath path = SdfPath("HdMayaBackupGLStateTask");
        return path;
    }

public:
    HdMayaGLBackup& _backup;

    HdMayaBackupGLStateTask(HdMayaGLBackup& backup)
        : base(_Id())
        , _backup(backup)
    {
    }
    /// Prepare the render pass resources
    virtual void Prepare(HdTaskContext* ctx, HdRenderIndex* renderIndex) override { }

    /// Execute the task
    virtual void Execute(HdTaskContext* ctx) override
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_backup.RestoreFramebuffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_backup.RestoreDrawFramebuffer);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_backup.RestoreReadFramebuffer);
    }
    virtual void Sync(HdSceneDelegate* del, HdTaskContext* ctx, HdDirtyBits* dirtyBits) override { }
};

class HdMayaRestoreGLStateTask : public HdTask
{
    using base = HdTask;
    static const SdfPath& _Id()
    {
        static SdfPath path = SdfPath("HdMayaRestoreGLStateTask");
        return path;
    }

public:
    HdMayaGLBackup& _backup;

    HdMayaRestoreGLStateTask(HdMayaGLBackup& backup)
        : base(_Id())
        , _backup(backup)
    {
    }
    /// Prepare the render pass resources
    virtual void Prepare(HdTaskContext* ctx, HdRenderIndex* renderIndex) override { }

    /// Execute the task
    virtual void Execute(HdTaskContext* ctx) override
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _backup.RestoreFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _backup.RestoreDrawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _backup.RestoreReadFramebuffer);
    }
    virtual void Sync(HdSceneDelegate* del, HdTaskContext* ctx, HdDirtyBits* dirtyBits) override { }
};

class HdMayaSetRenderGLState
{
public:
    HdMayaSetRenderGLState()
    {
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

        if (_oldBlend != BLEND) {
            glEnable(GL_BLEND);
        }

        if (_oldCullFace != CULL_FACE) {
            glDisable(GL_CULL_FACE);
        }
    }

    ~HdMayaSetRenderGLState()
    {
        if (_oldBlend != BLEND) {
            glDisable(GL_BLEND);
        }

        if (_oldBlendFunc != BLEND_FUNC) {
            glBlendFunc(GL_SRC_ALPHA, _oldBlendFunc);
        }

        if (_oldBlendEquation != BLEND_EQUATION) {
            glBlendEquation(_oldBlendEquation);
        }

        if (_oldCullFace != CULL_FACE) {
            glEnable(GL_CULL_FACE);
        }
    }

private:
    // non-odr
    constexpr static int       BLEND_FUNC = GL_ONE_MINUS_SRC_ALPHA;
    constexpr static int       BLEND_EQUATION = GL_FUNC_ADD;
    constexpr static GLboolean BLEND = GL_TRUE;
    constexpr static GLboolean CULL_FACE = GL_FALSE;

    int       _oldBlendFunc = BLEND_FUNC;
    int       _oldBlendEquation = BLEND_EQUATION;
    GLboolean _oldBlend = BLEND;
    GLboolean _oldCullFace = CULL_FACE;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_VIEW_OVERRIDE_UTILS_H
