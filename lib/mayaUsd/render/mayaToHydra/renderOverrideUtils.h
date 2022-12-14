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

#include <mayaHydraLib/utils.h>

#include <maya/MViewport2Renderer.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraPreRender : public MHWRender::MSceneRender
{
public:
    explicit MayaHydraPreRender(const MString& name)
        : MHWRender::MSceneRender(name)
    {
        // To keep the colors always sync'ed, reuse same clear colors as global ones instead of set the same colors explicitly.
        mClearOperation.setOverridesColors(false);
    }

    MUint64 getObjectTypeExclusions() override
    {
        // To skip the generation of some unwanted render lists even the kRenderPreSceneUIItems filter is specified.
        return MFrameContext::kExcludeManipulators | MFrameContext::kExcludeHUD;
    }

    MSceneFilterOption renderFilterOverride() override { return kRenderPreSceneUIItems; }

    MHWRender::MClearOperation& clearOperation() override { return mClearOperation; }
};

class MayaHydraPostRender : public MHWRender::MSceneRender
{
public:
    explicit MayaHydraPostRender(const MString& name)
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
        return MFrameContext::kExcludeMeshes | MFrameContext::kExcludePluginShapes | kExcludeGrid;
    }

    MSceneFilterOption renderFilterOverride() override
    {
        return MSceneFilterOption(kRenderPostSceneUIItems);
    }

    MHWRender::MClearOperation& clearOperation() override { return mClearOperation; }
};

//! \brief Serves to synchronize maya viewport data with the scene delegate before scene update is called
//	when requiresSceneUpdate=false, subtype=kDataServerRemovals and after scene update is called
//  when requiresSceneUpdate=true, subtype=kDataServer
//

class MayaHydraRender : public MHWRender::MDataServerOperation
{
public:
	MayaHydraRender(const MString& name, MtohRenderOverride* override)
	: MDataServerOperation(name)
	, _override(override)
	{
	}
    
    MStatus execute(const MDrawContext & drawContext, const MHWRender::MDataServerOperation::MViewportScene& scene) override
    {
		return _override->Render(drawContext, scene);
    }

private:
    MtohRenderOverride* _override;
};

struct MayaHydraGLBackup
{
    GLint RestoreFramebuffer = 0;
    GLint RestoreDrawFramebuffer = 0;
    GLint RestoreReadFramebuffer = 0;
};

class MayaHydraBackupGLStateTask : public HdTask
{
    using base = HdTask;
    static const SdfPath& _Id()
    {
        static SdfPath path = SdfPath("MayaHydraBackupGLStateTask");
        return path;
    }

public:
    MayaHydraGLBackup& _backup;

    MayaHydraBackupGLStateTask(MayaHydraGLBackup& backup)
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

class MayaHydraRestoreGLStateTask : public HdTask
{
    using base = HdTask;
    static const SdfPath& _Id()
    {
        static SdfPath path = SdfPath("MayaHydraRestoreGLStateTask");
        return path;
    }

public:
    MayaHydraGLBackup& _backup;

    MayaHydraRestoreGLStateTask(MayaHydraGLBackup& backup)
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

class MayaHydraSetRenderGLState
{
public:
    MayaHydraSetRenderGLState()
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

    ~MayaHydraSetRenderGLState()
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
