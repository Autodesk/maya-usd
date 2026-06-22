//-
// holdoutRenderOverrideSpike.cpp
//
// PURPOSE (throwaway spike, increment 2):
//   Increment 1 proved the physics: a custom MUserRenderOperation can write
//   DEPTH while masking ALL color via raw glColorMask, and that depth occludes
//   a later scene render sharing the same depth target. (Confirmed: teal
//   rectangle punched through the scene.)
//
//   This increment proves the TRANSFORM WIRING: instead of a hardcoded
//   screen-space quad, we draw a real 3D box positioned in world space, using
//   the camera's actual view-projection matrix pulled from the MDrawContext.
//   The box is drawn depth-only (color masked) with proper depth testing.
//
//   This is the prerequisite for drawing real prim geometry: we confirm that
//   geometry placed in the scene via the live camera matrices stamps depth in
//   the correct place, so occlusion is perspective-correct and world-anchored.
//
// HOW TO TEST:
//   Create any object near the world origin (a default poly sphere is ideal),
//   load the plugin, choose "Holdout Depth Spike" in the viewport Renderer
//   menu, then ORBIT the camera.
//
// HOW TO READ THE RESULT:
//   * A teal box-silhouette hole that occludes the object behind it AND tracks
//     the scene in 3D as you orbit (its shape/size change with view angle like
//     a real cube, perspective-correct, locked to world space) -> SUCCESS.
//     Transform wiring is correct; ready to feed real prim geometry.
//   * A hole that ignores the camera (flat, screen-locked, wrong place, or
//     inside-out) -> the view-projection / matrix convention is wrong.
//   * Geometry that should be IN FRONT of the box getting occluded, or vice
//     versa -> depth ordering wrong.
//
// GL-only by design (Linux / OpenGL Core Profile).
//+

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MMatrix.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>

// Modern GL entry points, loaded exactly like the rest of maya-usd does it.
#include <pxr/imaging/garch/glApi.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const MString kOverrideName("holdoutSpike");
const MString kOverrideUIName("Holdout Depth Spike");

// Vertex shader: real 3D positions transformed by the camera's view-projection
// (combined with a model matrix that stands in for a prim's world transform).
const char* kVertexSrc =
    "#version 330\n"
    "layout(location = 0) in vec3 position;\n"
    "uniform mat4 mvp;\n"
    "void main() { gl_Position = mvp * vec4(position, 1.0); }\n";

// Bright magenta: only visible if color masking is (unexpectedly) ignored.
const char* kFragmentSrc =
    "#version 330\n"
    "out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 1.0, 1.0); }\n";

// A cube spanning [-1, 1] on each axis, centered at the world origin.
// 36 vertices (6 faces x 2 triangles x 3 verts). Winding is irrelevant: we do
// not cull and only care about depth, so the nearest surface wins per pixel.
const GLfloat kCubeVerts[] = {
    // -Z
    -1, -1, -1,   1,  1, -1,   1, -1, -1,
    -1, -1, -1,  -1,  1, -1,   1,  1, -1,
    // +Z
    -1, -1,  1,   1, -1,  1,   1,  1,  1,
    -1, -1,  1,   1,  1,  1,  -1,  1,  1,
    // -X
    -1, -1, -1,  -1, -1,  1,  -1,  1,  1,
    -1, -1, -1,  -1,  1,  1,  -1,  1, -1,
    // +X
     1, -1, -1,   1,  1, -1,   1,  1,  1,
     1, -1, -1,   1,  1,  1,   1, -1,  1,
    // -Y
    -1, -1, -1,   1, -1, -1,   1, -1,  1,
    -1, -1, -1,   1, -1,  1,  -1, -1,  1,
    // +Y
    -1,  1, -1,  -1,  1,  1,   1,  1,  1,
    -1,  1, -1,   1,  1,  1,   1,  1, -1,
};
const GLsizei kCubeVertCount = 36;

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

//============================================================================
// Operation 1: the depth-only pass -- now drawing a world-space 3D box.
//============================================================================
class HoldoutDepthOp : public MHWRender::MUserRenderOperation
{
public:
    HoldoutDepthOp(const MString& name)
        : MHWRender::MUserRenderOperation(name)
    {
    }

    ~HoldoutDepthOp() override = default;

    MStatus execute(const MHWRender::MDrawContext& drawContext) override
    {
        if (!ensureGLResources())
            return MS::kFailure;

        // --- save the GL state we are about to disturb -----------------
        GLint     prevProgram = 0, prevVao = 0, prevDepthFunc = GL_LESS;
        GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
        GLboolean prevDepthMask = GL_TRUE;
        GLboolean prevColorMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
        glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
        glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

        // --- (a) establish the shared buffers (this op runs first) -----
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glClearColor(0.10f, 0.15f, 0.20f, 1.0f); // teal -> the "hole" color
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- (b) compute MVP from the live camera ----------------------
        // Maya uses row-vector convention: clip_row = world_row * viewProj.
        // The model matrix is a stand-in for a prim's world transform; here
        // identity (the cube verts are already authored around the origin).
        MStatus mstat;
        MMatrix viewProj = drawContext.getMatrix(
            MHWRender::MFrameContext::kViewProjMtx, &mstat);
        if (!mstat) {
            MGlobal::displayError("[holdoutSpike] failed to get viewProj matrix.");
            return MS::kFailure;
        }
        MMatrix model;                  // identity == prim-world stand-in
        MMatrix mvp = model * viewProj; // row-vector compose

        // Row-major fill + transpose=GL_FALSE uploads the column-vector
        // equivalent (mvp^T), which is what the column-major shader expects.
        GLfloat mvpData[16];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                mvpData[r * 4 + c] = static_cast<GLfloat>(mvp(r, c));

        // --- (c) THE PASS: write depth (real ordering), mask all color --
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);                          // true depth ordering
        glDepthMask(GL_TRUE);                          // depth write ON
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // color write OFF

        glUseProgram(mProgram);
        glUniformMatrix4fv(mMvpLoc, 1, GL_FALSE, mvpData);
        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, kCubeVertCount);
        glBindVertexArray(0);
        glUseProgram(0);

        // --- restore state so the beauty scene render starts clean -----
        glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
        glDepthMask(prevDepthMask);
        glDepthFunc(prevDepthFunc);
        if (prevDepthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glUseProgram(static_cast<GLuint>(prevProgram));
        glBindVertexArray(static_cast<GLuint>(prevVao));

        return MS::kSuccess;
    }

private:
    // Built lazily on first execute (live GL context). Intentionally not
    // deleted -- this is a spike; objects are reclaimed on plugin unload.
    bool ensureGLResources()
    {
        if (mInitialized)
            return mProgram != 0;
        mInitialized = true;

        GarchGLApiLoad();

        GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
        if (!vs || !fs)
            return false;

        mProgram = glCreateProgram();
        glAttachShader(mProgram, vs);
        glAttachShader(mProgram, fs);
        glLinkProgram(mProgram);
        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint linked = GL_FALSE;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[1024] = { 0 };
            glGetProgramInfoLog(mProgram, sizeof(log), nullptr, log);
            MGlobal::displayError(MString("[holdoutSpike] program link failed: ") + log);
            glDeleteProgram(mProgram);
            mProgram = 0;
            return false;
        }

        mMvpLoc = glGetUniformLocation(mProgram, "mvp");

        // VAO + VBO for the cube (position attribute at location 0).
        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);
        glGenBuffers(1, &mVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        return true;
    }

    bool   mInitialized = false;
    GLuint mProgram = 0;
    GLuint mVao = 0;
    GLuint mVbo = 0;
    GLint  mMvpLoc = -1;
};

//============================================================================
// Operation 2: the beauty scene render -- draws everything, does NOT clear,
// so it depth-tests against the depth our user op just stamped.
//============================================================================
class BeautySceneRender : public MHWRender::MSceneRender
{
public:
    BeautySceneRender(const MString& name)
        : MHWRender::MSceneRender(name)
    {
    }

    MHWRender::MClearOperation& clearOperation() override
    {
        mClearOperation.setMask(
            static_cast<unsigned int>(MHWRender::MClearOperation::kClearNone));
        return mClearOperation;
    }
};

//============================================================================
// The override: [ depth-only user op ] -> [ beauty scene ] -> [ present ]
//============================================================================
class HoldoutSpikeOverride : public MHWRender::MRenderOverride
{
public:
    HoldoutSpikeOverride(const MString& name)
        : MHWRender::MRenderOverride(name)
    {
        mOps[0] = new HoldoutDepthOp("holdoutSpike_DepthOnly");
        mOps[1] = new BeautySceneRender("holdoutSpike_Beauty");
        mOps[2] = new MHWRender::MPresentTarget("holdoutSpike_Present");
    }

    ~HoldoutSpikeOverride() override
    {
        for (auto*& op : mOps) {
            delete op;
            op = nullptr;
        }
    }

    MHWRender::DrawAPI supportedDrawAPIs() const override
    {
        // GL only: the depth pass uses raw OpenGL. (Linux / Core Profile.)
        return static_cast<MHWRender::DrawAPI>(
            MHWRender::kOpenGL | MHWRender::kOpenGLCoreProfile);
    }

    MString uiName() const override { return kOverrideUIName; }

    bool startOperationIterator() override
    {
        mCurrentOp = 0;
        return true;
    }

    MHWRender::MRenderOperation* renderOperation() override
    {
        if (mCurrentOp >= 0 && mCurrentOp < kNumOps)
            return mOps[mCurrentOp];
        return nullptr;
    }

    bool nextRenderOperation() override
    {
        ++mCurrentOp;
        return mCurrentOp < kNumOps;
    }

private:
    static const int             kNumOps = 3;
    MHWRender::MRenderOperation*  mOps[kNumOps] = { nullptr, nullptr, nullptr };
    int                          mCurrentOp = 0;
};

HoldoutSpikeOverride* gOverride = nullptr;

} // anonymous namespace

//============================================================================
// Plugin entry points
//============================================================================
MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "HoldoutSpike", "0.2", "Any");

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        MGlobal::displayError("[holdoutSpike] VP2 renderer not available.");
        return MS::kFailure;
    }

    if (!gOverride) {
        gOverride = new HoldoutSpikeOverride(kOverrideName);
        renderer->registerOverride(gOverride);
    }

    MGlobal::displayInfo(
        "[holdoutSpike] Registered. Pick 'Holdout Depth Spike' in the "
        "viewport Renderer menu, then orbit the camera.");
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer && gOverride) {
        renderer->deregisterOverride(gOverride);
        delete gOverride;
        gOverride = nullptr;
    }
    return MS::kSuccess;
}