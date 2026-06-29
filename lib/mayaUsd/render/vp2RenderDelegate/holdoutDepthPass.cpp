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
#include <pxr/base/tf/getenv.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MHWGeometry.h>
#include <maya/MImage.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTextureManager.h>
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
GLuint gVao { 0 };

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

//============================================================================
// Plate matte: load the camera's image-plane footage via MTextureManager and
// sample it where the holdout geometry is stamped, so the holdout reveals the
// plate. (Sampled in screen space for now; placement + color management TODO.)
//============================================================================

bool   gMatteInit { false };
GLuint gMatteProgram { 0 };
GLint  gMatteMvpLoc { -1 };
GLint  gMatteSamplerLoc { -1 };
GLint  gMatteViewportLoc { -1 };
GLint  gMatteDebugLoc { -1 };

// Diagnostic: when true, the matte paints solid red instead of the plate, to
// test the depth masking independent of texture sampling.
const bool kMatteDebugSolid = false;

std::unordered_map<std::string, MHWRender::MTexture*> gTextureCache;

// Vertex: transform holdout geometry by mvp (same convention as the depth pass).
const char* kMatteVertexSrc = "#version 330\n"
                              "layout(location = 0) in vec3 position;\n"
                              "uniform mat4 mvp;\n"
                              "void main() { gl_Position = mvp * vec4(position, 1.0); }\n";

// Fragment: sample the plate in SCREEN space, so it reads like a flat backdrop
// revealed through the holdout silhouette (not projected onto the 3D surface).
// gl_FragCoord is bottom-left origin; the plate is top-down, hence the V flip.
const char* kMatteFragmentSrc = "#version 330\n"
                                "out vec4 fragColor;\n"
                                "uniform sampler2D plate;\n"
                                "uniform vec2 viewportSize;\n"
                                "uniform int debugMode;\n"
                                "void main() {\n"
                                "    if (debugMode == 1) { fragColor = vec4(1.0, 0.0, 0.0, 1.0); return; }\n"
                                "    vec2 uv = vec2(gl_FragCoord.x / viewportSize.x,\n"
                                "                   1.0 - gl_FragCoord.y / viewportSize.y);\n"
                                "    fragColor = texture(plate, uv);\n"
                                "}\n";

bool ensureMatteGL()
{
    if (gMatteInit)
        return gMatteProgram != 0;
    gMatteInit = true;

    GarchGLApiLoad();

    GLuint vs = compileShader(GL_VERTEX_SHADER, kMatteVertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kMatteFragmentSrc);
    if (!vs || !fs)
        return false;

    gMatteProgram = glCreateProgram();
    glAttachShader(gMatteProgram, vs);
    glAttachShader(gMatteProgram, fs);
    glLinkProgram(gMatteProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(gMatteProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = { 0 };
        glGetProgramInfoLog(gMatteProgram, sizeof(log), nullptr, log);
        MGlobal::displayError(MString("[holdoutDepthPass] matte link failed: ") + log);
        glDeleteProgram(gMatteProgram);
        gMatteProgram = 0;
        return false;
    }

    gMatteMvpLoc = glGetUniformLocation(gMatteProgram, "mvp");
    gMatteSamplerLoc = glGetUniformLocation(gMatteProgram, "plate");
    gMatteViewportLoc = glGetUniformLocation(gMatteProgram, "viewportSize");
    gMatteDebugLoc = glGetUniformLocation(gMatteProgram, "debugMode");
    glGenVertexArrays(1, &gVao); // core profile requires a VAO; attribs set per draw
    return true;
}

// Find the active camera's first image plane, load its image (cached by path),
// and return the GL texture name. 0 on failure.
GLuint acquirePlateGLTexture(M3dView& view)
{
    MStatus  st;
    MDagPath camPath;
    if (view.getCamera(camPath) != MS::kSuccess)
        return 0;
    camPath.extendToShape(); // camera transform -> camera shape (no-op if shape)

    MString    plateFile;
    bool       foundPlane = false;
    MObject    imagePlaneObj;

    // Primary: image planes are connected to the camera shape's "imagePlane"
    // array plug (imagePlaneShape.message -> cameraShape.imagePlane[n]); they
    // are usually NOT DAG children. Walk that plug to the source node.
    {
        MFnDependencyNode camDepFn(camPath.node());
        MPlug             ipArray = camDepFn.findPlug("imagePlane", false, &st);
        if (st == MS::kSuccess && !ipArray.isNull()) {
            const unsigned int numEl = ipArray.numElements();
            for (unsigned int e = 0; e < numEl; ++e) {
                MPlug         elem = ipArray.elementByPhysicalIndex(e, &st);
                MPlugArray    srcs;
                if (elem.connectedTo(srcs, true, false) && srcs.length() > 0) {
                    MObject node = srcs[0].node();
                    if (node.hasFn(MFn::kImagePlane)) {
                        imagePlaneObj = node;
                        foundPlane = true;
                        break;
                    }
                }
            }
        }
    }

    // Fallback: some rigs DO parent the image plane under the camera shape.
    MFnDagNode         camFn(camPath);
    const unsigned int nChildren = camFn.childCount();
    if (!foundPlane) {
        for (unsigned int i = 0; i < nChildren; ++i) {
            MObject child = camFn.child(i);
            if (child.hasFn(MFn::kImagePlane)) {
                imagePlaneObj = child;
                foundPlane = true;
                break;
            }
        }
    }

    if (foundPlane) {
        MFnDependencyNode ipFn(imagePlaneObj);
        MPlug             p = ipFn.findPlug("imageName", false, &st);
        if (st == MS::kSuccess)
            p.getValue(plateFile);
    }

    if (plateFile.length() == 0)
        return 0;

    const std::string key(plateFile.asChar());
    auto              it = gTextureCache.find(key);
    if (it != gTextureCache.end() && it->second) {
        const GLuint* th = static_cast<const GLuint*>(it->second->resourceHandle());
        return th ? *th : 0;
    }

    MImage img;
    if (img.readFromFile(plateFile) != MS::kSuccess) {
        static bool sWarned = false;
        if (!sWarned) {
            sWarned = true;
            MGlobal::displayWarning(
                MString("[holdoutDepthPass] could not read image: '") + plateFile
                + "' -- a sequence path likely needs frame resolution.");
        }
        return 0;
    }
    unsigned int w = 0, h = 0;
    img.getSize(w, h);

    MHWRender::MRenderer*       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* texMgr = renderer ? renderer->getTextureManager() : nullptr;
    if (!texMgr || w == 0 || h == 0)
        return 0;

    MHWRender::MTextureDescription desc;
    desc.setToDefault2DTexture();
    desc.fWidth = w;
    desc.fHeight = h;
    desc.fDepth = 1;
    desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
    desc.fBytesPerRow = w * 4;
    desc.fBytesPerSlice = w * h * 4;

    MHWRender::MTexture* tex = texMgr->acquireTexture(plateFile, desc, img.pixels());
    if (!tex)
        return 0;
    gTextureCache[key] = tex;

    const GLuint* th = static_cast<const GLuint*>(tex->resourceHandle());
    return th ? *th : 0;
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
    if (entries.empty())
        return;

    if (!ensureMatteGL())
        return;

    // Resolve the view for the panel being DRAWN (not the focused/active one),
    // so each viewport stamps with its own camera and plate. Fall back to the
    // active view for non-3d-viewport destinations (e.g. some playblast paths).
    MStatus st;
    M3dView view;
    MString panelName;
    bool    gotView = false;
    if (context.renderingDestination(panelName) == MHWRender::MFrameContext::k3dViewport)
        gotView = (M3dView::getM3dViewFromModelPanel(panelName, view) == MS::kSuccess);
    if (!gotView) {
        view = M3dView::active3dView(&st);
        if (!st)
            return;
    }

    MMatrix modelView, projection;
    if (view.modelViewMatrix(modelView) != MS::kSuccess
        || view.projectionMatrix(projection) != MS::kSuccess)
        return;

    // Plate texture for this view; 0 (e.g. persp view with no image plane) ->
    // depth-only stamp so the holdout still occludes CG, revealing the viewport
    // background in its silhouette.
    const GLuint texName = acquirePlateGLTexture(view);
    const bool   havePlate = (texName != 0);

    // Policy for viewports with no plate (e.g. persp): by default the holdout
    // still occludes CG (revealing the viewport background in its silhouette).
    // Set MAYAUSD_HOLDOUT_OCCLUDE_WITHOUT_PLATE=0 to make it inert there.
    static const bool sOccludeWithoutPlate
        = TfGetenvBool("MAYAUSD_HOLDOUT_OCCLUDE_WITHOUT_PLATE", true);
    if (!havePlate && !sOccludeWithoutPlate)
        return;

    // --- save the GL state we touch ------------------------------------
    GLint     prevProgram = 0, prevVao = 0, prevArrayBuf = 0, prevDepthFunc = GL_LESS;
    GLint     prevActiveTex = GL_TEXTURE0, prevTex2D = 0, prevViewport[4] = { 0, 0, 0, 0 };
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevDepthMask = GL_TRUE;
    GLboolean prevColorMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuf);
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

    int vx = 0, vy = 0, vw = 0, vh = 0;
    if (context.getViewportDimensions(vx, vy, vw, vh) != MS::kSuccess || vw <= 0 || vh <= 0) {
        vx = prevViewport[0];
        vy = prevViewport[1];
        vw = prevViewport[2];
        vh = prevViewport[3];
    }
    glViewport(vx, vy, vw, vh);

    // Combined begin-scene stamp: write the holdout's DEPTH (so the scene
    // occludes CG behind it) and -- where a plate exists -- its plate COLOR in
    // screen space. The scene then draws on top: the background plane and any CG
    // behind the holdout fail the depth test and leave the plate intact, while
    // CG in front draws over it. The masking is done by WRITING at begin-scene
    // (depth persists into the scene draw) rather than testing at end-scene
    // (where the scene depth is already gone).
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(havePlate, havePlate, havePlate, havePlate);

    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex2D);
    if (havePlate)
        glBindTexture(GL_TEXTURE_2D, texName);

    glUseProgram(gMatteProgram);
    glUniform1i(gMatteSamplerLoc, 0);
    glUniform2f(gMatteViewportLoc, static_cast<float>(vw), static_cast<float>(vh));
    glUniform1i(gMatteDebugLoc, kMatteDebugSolid ? 1 : 0);
    glBindVertexArray(gVao);

    for (const Entry& e : entries) {
        if (e.posHandle == 0 || e.idxHandle == 0 || e.indexCount == 0)
            continue;

        const MMatrix mvp = e.world * modelView * projection; // row-vector compose
        GLfloat       m[16];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                m[r * 4 + c] = static_cast<GLfloat>(mvp(r, c));
        glUniformMatrix4fv(gMatteMvpLoc, 1, GL_FALSE, m);

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
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex2D));
    glActiveTexture(static_cast<GLuint>(prevActiveTex));
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

        if (MHWRender::MTextureManager* texMgr = renderer->getTextureManager()) {
            for (auto& kv : gTextureCache) {
                if (kv.second)
                    texMgr->releaseTexture(kv.second);
            }
        }
    }
    gTextureCache.clear();
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