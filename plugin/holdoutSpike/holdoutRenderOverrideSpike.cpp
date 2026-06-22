//-
// holdoutRenderOverrideSpike.cpp
//
// PURPOSE (throwaway spike, increment 5 -- full path inside stock VP2):
//   Proven so far:
//     1. depth-write-with-color-masked survives a custom pass (raw glColorMask)
//     2. a world-space 3D cube via the live camera occludes correctly
//     3. depth written from a kBeginSceneRenderSemantic notification on stock
//        VP2 PERSISTS into the opaque draw (rectangle punched through sphere)
//     4. BUT getMatrix(kViewProjMtx) at that hook returns a placeholder (camera
//        not loaded yet) -- it logged as an identity+depth-remap, not a camera.
//
//   This increment closes the loop: bring back the real 3D cube, but source
//   its MVP from M3dView (active 3D view) -- which derives the camera from the
//   DAG and does NOT depend on the frame context being warm -- and stamp it
//   depth-only from the beginSceneRender notification.
//
//   M3dView convention is Maya row-vector (same as increment 2's working
//   path): clip = world * modelView * projection. So:
//       mvp = model * modelView * projection
//   uploaded row-major with transpose = GL_FALSE for the column-major shader.
//
// HOW TO READ THE RESULT (stock Viewport 2.0, sphere at origin):
//   * A correct, perspective cube-silhouette holdout: invisible occluder that
//     bites the sphere, world-anchored, tracking as you orbit -> the ENTIRE
//     render path is proven inside stock VP2. Only thing left: swap the
//     hardcoded cube for the holdout prim's real geometry + transform.
//   * Cube misplaced / smeared / inside-out -> M3dView matrix convention is
//     off; the one-time matrix log tells us how to correct it.
//
// GL-only by design (Linux / OpenGL Core Profile).
//+

// Include ORDER matters: the GL loader / pxr headers must come BEFORE the Maya
// headers. This mirrors lib/mayaUsd/render/px_vp20/utils.cpp, which includes
// both garch and <maya/M3dView.h> in this order. M3dView.h is the legacy GL
// viewport header and drags in system GL; if garch is included after it, the
// transitive <pxr/pxr.h> that defines PXR_NAMESPACE_USING_DIRECTIVE gets
// skipped and the macro fails to expand. (Increments 1-4 didn't hit this
// because none of them included M3dView.h.)
#include <pxr/imaging/garch/glApi.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MMatrix.h>
#include <maya/M3dView.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MStringArray.h>

#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const MString kNotificationName("holdoutSpike_DepthStamp");

const char* kVertexSrc =
    "#version 330\n"
    "layout(location = 0) in vec3 position;\n"
    "uniform mat4 mvp;\n"
    "void main() { gl_Position = mvp * vec4(position, 1.0); }\n";

const char* kFragmentSrc =
    "#version 330\n"
    "out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 1.0, 1.0); }\n";

// Cube spanning [-1, 1] on each axis, centered at the world origin.
const GLfloat kCubeVerts[] = {
    -1, -1, -1,   1,  1, -1,   1, -1, -1,
    -1, -1, -1,  -1,  1, -1,   1,  1, -1,
    -1, -1,  1,   1, -1,  1,   1,  1,  1,
    -1, -1,  1,   1,  1,  1,  -1,  1,  1,
    -1, -1, -1,  -1, -1,  1,  -1,  1,  1,
    -1, -1, -1,  -1,  1,  1,  -1,  1, -1,
     1, -1, -1,   1,  1, -1,   1,  1,  1,
     1, -1, -1,   1,  1,  1,   1, -1,  1,
    -1, -1, -1,   1, -1, -1,   1, -1,  1,
    -1, -1, -1,   1, -1,  1,  -1, -1,  1,
    -1,  1, -1,  -1,  1,  1,   1,  1,  1,
    -1,  1, -1,   1,  1,  1,   1,  1, -1,
};
const GLsizei kCubeVertCount = 36;

struct GLState {
    bool   initialized = false;
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint  mvpLoc = -1;
};
GLState gGL;

std::set<std::string> gLoggedPasses;
bool gLoggedMatrix = false;

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
        MGlobal::displayError(MString("[holdoutSpike] shader compile failed: ") + log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ensureGLResources()
{
    if (gGL.initialized)
        return gGL.program != 0;
    gGL.initialized = true;

    GarchGLApiLoad();

    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
    if (!vs || !fs)
        return false;

    gGL.program = glCreateProgram();
    glAttachShader(gGL.program, vs);
    glAttachShader(gGL.program, fs);
    glLinkProgram(gGL.program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = GL_FALSE;
    glGetProgramiv(gGL.program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = { 0 };
        glGetProgramInfoLog(gGL.program, sizeof(log), nullptr, log);
        MGlobal::displayError(MString("[holdoutSpike] program link failed: ") + log);
        glDeleteProgram(gGL.program);
        gGL.program = 0;
        return false;
    }

    gGL.mvpLoc = glGetUniformLocation(gGL.program, "mvp");

    glGenVertexArrays(1, &gGL.vao);
    glBindVertexArray(gGL.vao);
    glGenBuffers(1, &gGL.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gGL.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

void logMatrixOnce(const char* label, const MMatrix& m)
{
    for (int r = 0; r < 4; ++r) {
        MString row("[holdoutSpike] ");
        row += label; row += " row "; row += r; row += ": ";
        for (int c = 0; c < 4; ++c) { row += m(r, c); row += "  "; }
        MGlobal::displayInfo(row);
    }
}

//============================================================================
// Pre-scene-render notification: real 3D cube, MVP from M3dView.
//============================================================================
void holdoutDepthNotify(MHWRender::MDrawContext& context, void* /*clientData*/)
{
    if (!ensureGLResources())
        return;

    // --- log the pass (once per distinct pass) -------------------------
    const MHWRender::MPassContext& pass = context.getPassContext();
    const MString                  passId = pass.passIdentifier();
    MStringArray                   sems = pass.passSemantics();
    MString semJoined;
    for (unsigned int i = 0; i < sems.length(); ++i) {
        if (i) semJoined += ",";
        semJoined += sems[i];
    }
    std::string key = std::string(passId.asChar()) + "|" + semJoined.asChar();
    if (gLoggedPasses.find(key) == gLoggedPasses.end()) {
        gLoggedPasses.insert(key);
        MGlobal::displayInfo(
            MString("[holdoutSpike] callback fired -- pass id='") + passId
            + "' semantics='" + semJoined + "'");
    }

    // --- camera from M3dView (DAG-derived, not the cold frame context) -
    MStatus mstat;
    M3dView view = M3dView::active3dView(&mstat);
    if (!mstat) {
        MGlobal::displayError("[holdoutSpike] active3dView() failed.");
        return;
    }
    MMatrix modelView, projection;
    if (view.modelViewMatrix(modelView) != MS::kSuccess ||
        view.projectionMatrix(projection) != MS::kSuccess) {
        MGlobal::displayError("[holdoutSpike] failed to read M3dView matrices.");
        return;
    }

    MMatrix model;                                  // identity == prim stand-in
    MMatrix mvp = model * modelView * projection;   // row-vector compose

    if (!gLoggedMatrix) {
        gLoggedMatrix = true;
        logMatrixOnce("modelView", modelView);
        logMatrixOnce("projection", projection);
        logMatrixOnce("mvp", mvp);
    }

    GLfloat mvpData[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            mvpData[r * 4 + c] = static_cast<GLfloat>(mvp(r, c));

    // --- save the GL state we touch ------------------------------------
    GLint     prevProgram = 0, prevVao = 0, prevDepthFunc = GL_LESS;
    GLint     prevViewport[4] = { 0, 0, 0, 0 };
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevDepthMask = GL_TRUE;
    GLboolean prevColorMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

    int vx = 0, vy = 0, vw = 0, vh = 0;
    if (context.getViewportDimensions(vx, vy, vw, vh) == MS::kSuccess && vw > 0 && vh > 0)
        glViewport(vx, vy, vw, vh);

    // --- stamp: write depth (true ordering), mask all color ------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glUseProgram(gGL.program);
    glUniformMatrix4fv(gGL.mvpLoc, 1, GL_FALSE, mvpData);
    glBindVertexArray(gGL.vao);
    glDrawArrays(GL_TRIANGLES, 0, kCubeVertCount);
    glBindVertexArray(0);
    glUseProgram(0);

    // --- restore -------------------------------------------------------
    glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    glDepthMask(prevDepthMask);
    glDepthFunc(prevDepthFunc);
    if (prevDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glUseProgram(static_cast<GLuint>(prevProgram));
    glBindVertexArray(static_cast<GLuint>(prevVao));
}

} // anonymous namespace

//============================================================================
// Plugin entry points
//============================================================================
MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "HoldoutSpike", "0.5", "Any");

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        MGlobal::displayError("[holdoutSpike] VP2 renderer not available.");
        return MS::kFailure;
    }

    MStatus status = renderer->addNotification(
        holdoutDepthNotify,
        kNotificationName,
        MHWRender::MPassContext::kBeginSceneRenderSemantic,
        nullptr);
    if (!status) {
        MGlobal::displayError("[holdoutSpike] addNotification failed.");
        return status;
    }

    MGlobal::displayInfo(
        "[holdoutSpike] Full-path test registered on Viewport 2.0. "
        "Use the normal 'Viewport 2.0' renderer; sphere at origin; orbit.");
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        renderer->removeNotification(
            kNotificationName,
            MHWRender::MPassContext::kBeginSceneRenderSemantic);
    }
    return MS::kSuccess;
}