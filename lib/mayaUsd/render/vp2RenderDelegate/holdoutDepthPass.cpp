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
#include "holdoutDepthPass.h"

// GL loader / pxr headers MUST precede the Maya headers (M3dView.h drags in
// system GL); otherwise the transitive pxr/pxr.h gets skipped. Mirrors the
// include order in px_vp20/utils.cpp.
#include <pxr/imaging/garch/glApi.h>

#include <maya/M3dView.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MHWGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MViewport2Renderer.h>

#include <mutex>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdVP2HoldoutDepthPass {

namespace {

const MString kNotificationName("mayaUsd_HoldoutDepthStamp");

struct Entry
{
    // GL resource handles captured at commit time (while the buffers are alive).
    // We never dereference Maya buffer objects in the render callback.
    GLuint       posHandle { 0 };
    GLuint       idxHandle { 0 };
    unsigned int indexCount { 0 };
    MMatrix      world;
};

std::mutex                                               gMutex;
std::unordered_map<const MHWRender::MRenderItem*, Entry> gRegistry;
bool                                                     gRegistered { false };

// GL resources, built lazily on the live context during the first callback.
bool   gGLInit { false };
GLuint gProgram { 0 };
GLuint gVao { 0 };
GLint  gMvpLoc { -1 };

const char* kVertexSrc = "#version 330\n"
                         "layout(location = 0) in vec3 position;\n"
                         "uniform mat4 mvp;\n"
                         "void main() { gl_Position = mvp * vec4(position, 1.0); }\n";

// Magenta: masked out in normal operation; only visible if color masking fails.
const char* kFragmentSrc = "#version 330\n"
                           "out vec4 fragColor;\n"
                           "void main() { fragColor = vec4(1.0, 0.0, 1.0, 1.0); }\n";

GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = { 0 };
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        MGlobal::displayError(MString("[holdoutDepthPass] shader compile failed: ") + log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ensureGL()
{
    if (gGLInit)
        return gProgram != 0;
    gGLInit = true;

    GarchGLApiLoad();

    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
    if (!vs || !fs)
        return false;

    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, fs);
    glLinkProgram(gProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(gProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = { 0 };
        glGetProgramInfoLog(gProgram, sizeof(log), nullptr, log);
        MGlobal::displayError(MString("[holdoutDepthPass] program link failed: ") + log);
        glDeleteProgram(gProgram);
        gProgram = 0;
        return false;
    }

    gMvpLoc = glGetUniformLocation(gProgram, "mvp");
    glGenVertexArrays(1, &gVao); // required by core profile; attribs set per draw
    return true;
}

void depthNotify(MHWRender::MDrawContext& context, void* /*clientData*/)
{
    // Snapshot the registry under lock, then draw without holding it.
    std::vector<Entry> entries;
    {
        std::lock_guard<std::mutex> lock(gMutex);
        entries.reserve(gRegistry.size());
        for (const auto& kv : gRegistry)
            entries.push_back(kv.second);
    }
    {
        static int sLastCount = -1;
        if (static_cast<int>(entries.size()) != sLastCount) {
            sLastCount = static_cast<int>(entries.size());
            MGlobal::displayInfo(
                MString("[holdoutDepthPass] callback fired; entries=") + sLastCount);
        }
    }
    if (entries.empty())
        return;

    if (!ensureGL())
        return;

    // Camera from M3dView (DAG-derived; valid even though the frame context's
    // matrices are not yet warm at beginSceneRender).
    MStatus st;
    M3dView view = M3dView::active3dView(&st);
    if (!st)
        return;
    MMatrix modelView, projection;
    if (view.modelViewMatrix(modelView) != MS::kSuccess
        || view.projectionMatrix(projection) != MS::kSuccess)
        return;

    // --- save the GL state we touch ------------------------------------
    GLint     prevProgram = 0, prevVao = 0, prevArrayBuf = 0, prevDepthFunc = GL_LESS;
    GLint     prevViewport[4] = { 0, 0, 0, 0 };
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevDepthMask = GL_TRUE;
    GLboolean prevColorMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuf);
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

    int vx = 0, vy = 0, vw = 0, vh = 0;
    if (context.getViewportDimensions(vx, vy, vw, vh) == MS::kSuccess && vw > 0 && vh > 0)
        glViewport(vx, vy, vw, vh);

    // TEMP DIAGNOSTIC: when true, draw the stamped geometry in MAGENTA with
    // color writes ON and depth ignored, to positively confirm we are sourcing
    // the right buffers + transform. Set to false for the real depth-only
    // holdout stamp. (Holdout has no *visible* effect until the beauty-pass
    // exclusion increment, because the prim still draws normally in beauty;
    // this toggle lets us verify sourcing right now.)
    static const bool kDiagnosticShowColor = false;

    glEnable(GL_DEPTH_TEST);
    if (kDiagnosticShowColor) {
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    } else {
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

    glUseProgram(gProgram);
    glBindVertexArray(gVao);

    static bool sLoggedDraw = false;
    for (const Entry& e : entries) {
        if (e.posHandle == 0 || e.idxHandle == 0 || e.indexCount == 0)
            continue;

        if (!sLoggedDraw) {
            sLoggedDraw = true;
            MString msg("[holdoutDepthPass] draw inputs: posHandle=");
            msg += (int)e.posHandle;
            msg += " idxHandle=";
            msg += (int)e.idxHandle;
            msg += " indexCount=";
            msg += (int)e.indexCount;
            msg += " worldTranslate=(";
            msg += e.world(3, 0);
            msg += ", ";
            msg += e.world(3, 1);
            msg += ", ";
            msg += e.world(3, 2);
            msg += ")";
            MGlobal::displayInfo(msg);
        }

        MMatrix mvp;
        if (kDiagnosticShowColor) {
            // Shift the diagnostic geometry +3 in world X so the prim's own
            // beauty draw doesn't overpaint it.
            const double tv[4][4]
                = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 3, 0, 0, 1 } };
            mvp = e.world * MMatrix(tv) * modelView * projection;
        } else {
            mvp = e.world * modelView * projection; // row-vector compose
        }
        GLfloat m[16];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                m[r * 4 + c] = static_cast<GLfloat>(mvp(r, c));
        glUniformMatrix4fv(gMvpLoc, 1, GL_FALSE, m);

        glBindBuffer(GL_ARRAY_BUFFER, e.posHandle);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // tight float3
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, e.idxHandle);
        glDrawElements(GL_TRIANGLES, e.indexCount, GL_UNSIGNED_INT, nullptr);

        GLenum      err = glGetError();
        static bool sLoggedErr = false;
        if (err != GL_NO_ERROR && !sLoggedErr) {
            sLoggedErr = true;
            MGlobal::displayError(
                MString("[holdoutDepthPass] GL error after draw: ") + (int)err);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // --- restore -------------------------------------------------------
    glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    glDepthMask(prevDepthMask);
    glDepthFunc(prevDepthFunc);
    if (prevDepthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevArrayBuf));
    glUseProgram(static_cast<GLuint>(prevProgram));
    glBindVertexArray(static_cast<GLuint>(prevVao));
}

} // anonymous namespace

void Register()
{
    if (gRegistered)
        return;
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer)
        return;
    MStatus status = renderer->addNotification(
        depthNotify,
        kNotificationName,
        MHWRender::MPassContext::kBeginSceneRenderSemantic,
        nullptr);
    gRegistered = (status == MS::kSuccess);
    if (gRegistered)
        MGlobal::displayInfo("[holdoutDepthPass] notification registered.");
    else
        MGlobal::displayError("[holdoutDepthPass] addNotification failed.");
}

void Deregister()
{
    if (!gRegistered)
        return;
    if (MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer()) {
        renderer->removeNotification(
            kNotificationName, MHWRender::MPassContext::kBeginSceneRenderSemantic);
    }
    gRegistered = false;
    std::lock_guard<std::mutex> lock(gMutex);
    gRegistry.clear();
}

void Publish(
    const MHWRender::MRenderItem* key,
    MHWRender::MVertexBuffer*     positionBuffer,
    MHWRender::MIndexBuffer*      indexBuffer,
    unsigned int                 indexCount,
    const MMatrix&                worldMatrix)
{
    if (!key || !positionBuffer || !indexBuffer)
        return;

    // Read the GL resource handles NOW, at commit time, while the buffers are
    // guaranteed alive. The render callback later uses only these cached ints,
    // never calling resourceHandle() on a buffer that may have been freed --
    // that was the scene-clear crash inside OGSMayaVertexBuffer.
    const GLuint* ph = static_cast<const GLuint*>(positionBuffer->resourceHandle());
    const GLuint* ih = static_cast<const GLuint*>(indexBuffer->resourceHandle());

    Entry e;
    e.posHandle = ph ? *ph : 0;
    e.idxHandle = ih ? *ih : 0;
    e.indexCount = indexCount;
    e.world = worldMatrix;

    std::lock_guard<std::mutex> lock(gMutex);
    gRegistry[key] = e;

    static bool sFirstPublish = true;
    if (sFirstPublish) {
        sFirstPublish = false;
        MGlobal::displayInfo(
            MString("[holdoutDepthPass] first Publish; posHandle=") + (int)e.posHandle
            + " idxHandle=" + (int)e.idxHandle + " indexCount=" + (int)e.indexCount
            + " registry size=" + (int)gRegistry.size());
    }
    if (e.posHandle == 0 || e.idxHandle == 0) {
        static bool sWarnedHandle = false;
        if (!sWarnedHandle) {
            sWarnedHandle = true;
            MGlobal::displayWarning(
                "[holdoutDepthPass] buffer handle was 0 at commit; if holdout does "
                "not appear, the handle is not ready this early.");
        }
    }
}

void Unpublish(const MHWRender::MRenderItem* key)
{
    std::lock_guard<std::mutex> lock(gMutex);
    gRegistry.erase(key);
}

} // namespace HdVP2HoldoutDepthPass

PXR_NAMESPACE_CLOSE_SCOPE