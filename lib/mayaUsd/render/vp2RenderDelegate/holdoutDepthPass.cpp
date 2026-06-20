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

#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdVP2HoldoutDepthPass {

namespace {

const MString kNotificationName("mayaUsd_HoldoutDepthStamp");

// Maximum image planes composited per camera. Must match MAX_PLANES in the
// matte fragment shader below.
const int kMaxPlanes = 8;

struct Entry
{
    // GL resource handles captured at commit time (while the buffers are alive).
    // We never dereference Maya buffer objects in the render callback.
    GLuint       posHandle { 0 };
    GLuint       idxHandle { 0 };
    unsigned int indexCount { 0 };
    MMatrix      world;
    // Owning proxy shape; used to skip holdouts whose proxy node is hidden in
    // Maya (hiding the proxy does not re-Sync the prims, so we check at draw).
    MDagPath     proxyDagPath;
    // USD prim visibility, pushed from Sync. A visibility-only change does not
    // run the geometry commit, so we keep the entry and gate drawing on this.
    bool         visible { true };
};

// One resolved image plane for the current frame: its GL texture, the
// horizontal sample scale that undoes its anamorphic squeeze, and its depth
// (used only to order the planes back-to-front).
struct PlaneTex
{
    GLuint texName { 0 };
    float  hscale { 1.0f };
    double depth { 0.0 };
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
// plate(s). Multiple image planes are composited back-to-front by depth using
// each plate's alpha.
//============================================================================

bool   gMatteInit { false };
GLuint gMatteProgram { 0 };
GLint  gMatteMvpLoc { -1 };
GLint  gMattePlatesLoc { -1 };     // sampler2D plates[MAX_PLANES]
GLint  gMatteHScalesLoc { -1 };    // float hscales[MAX_PLANES]
GLint  gMattePlaneDepthLoc { -1 }; // float planeDepth[MAX_PLANES]
GLint  gMattePlaneCountLoc { -1 }; // int planeCount
GLint  gMatteViewportLoc { -1 };
GLint  gMatteOriginLoc { -1 };
GLint  gMatteBandLoc { -1 };
GLint  gMatteDebugLoc { -1 };

// Debug: 0 = plate, 1 = solid red (depth-mask test), 2 = UV visualization
// (red = U, green = V) to see the plate-mapping directly.
const int kMatteDebugMode = 0;

// Cache of loaded plate textures, keyed by resolved (per-frame) file path. Each
// frame we keep exactly the plates in use and release the rest, so an image
// sequence does not accumulate one texture per frame.
std::unordered_map<std::string, MHWRender::MTexture*> gTextureCache;

// Vertex: transform holdout geometry by mvp (same convention as the depth pass).
const char* kMatteVertexSrc = "#version 330\n"
                              "layout(location = 0) in vec3 position;\n"
                              "uniform mat4 mvp;\n"
                              "void main() { gl_Position = mvp * vec4(position, 1.0); }\n";

// Fragment: sample the plate(s) in SCREEN space, so they read like a flat
// backdrop revealed through the holdout silhouette (not projected onto the 3D
// surface). Planes are composited back-to-front (index 0 = farthest) with a
// premultiplied 'over', and the result is emitted premultiplied so GL blends it
// over the viewport background (revealing it where the plates are transparent).
// gl_FragCoord is bottom-left origin; the plate is top-down, hence the V flip.
const char* kMatteFragmentSrc = "#version 330\n"
                                "out vec4 fragColor;\n"
                                "const int MAX_PLANES = 8;\n"
                                "uniform sampler2D plates[MAX_PLANES];\n"
                                "uniform float     hscales[MAX_PLANES];\n"
                                "uniform float     planeDepth[MAX_PLANES];\n" // window-space z per plane
                                "uniform int       planeCount;\n"
                                "uniform vec2      viewportSize;\n"
                                "uniform vec2      viewportOrigin;\n" // glViewport (vx, vy)
                                "uniform float     bandHalf;\n"       // plate NDC y half-extent
                                "uniform int       debugMode;\n"
                                "void main() {\n"
                                "    if (debugMode == 1) { fragColor = vec4(1.0, 0.0, 0.0, 1.0); return; }\n"
                                "    // gl_FragCoord is in window coords spanning [origin, origin+size];\n"
                                "    // make it panel-relative so an offset (non-maximized) panel maps right.\n"
                                "    float sx = (gl_FragCoord.x - viewportOrigin.x) / viewportSize.x;\n"
                                "    float sy = (gl_FragCoord.y - viewportOrigin.y) / viewportSize.y;\n"
                                "    float ndcy = sy * 2.0 - 1.0;\n"
                                "    float rv = 1.0 - ((ndcy / bandHalf + 1.0) * 0.5);\n" // V flip
                                "    // Composite planes back-to-front, premultiplied 'over'. hscale samples\n"
                                "    // the center fraction horizontally to undo each plate's squeeze.\n"
                                "    vec4 acc = vec4(0.0);\n"
                                "    for (int i = 0; i < MAX_PLANES; ++i) {\n"
                                "        if (i >= planeCount) break;\n"
                                "        // Only planes BEHIND the holdout surface are revealed here; planes\n"
                                "        // in front of it are drawn natively by Maya over our depth stamp.\n"
                                "        if (planeDepth[i] <= gl_FragCoord.z) continue;\n"
                                "        float ru = 0.5 + (sx - 0.5) * hscales[i];\n"
                                "        if (ru < 0.0 || ru > 1.0 || rv < 0.0 || rv > 1.0)\n"
                                "            continue;\n" // outside this plate -> transparent
                                "        vec4 c = texture(plates[i], vec2(ru, rv));\n" // straight alpha
                                "        vec4 cp = vec4(c.rgb * c.a, c.a);\n"          // premultiply
                                "        acc = cp + acc * (1.0 - c.a);\n"              // nearer over farther
                                "    }\n"
                                "    if (debugMode == 2) { fragColor = vec4(clamp(sx,0.0,1.0), clamp(rv,0.0,1.0), 0.0, 1.0); return; }\n"
                                "    fragColor = acc;\n" // premultiplied; GL blends over the background
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
    gMattePlatesLoc = glGetUniformLocation(gMatteProgram, "plates");
    gMatteHScalesLoc = glGetUniformLocation(gMatteProgram, "hscales");
    gMattePlaneDepthLoc = glGetUniformLocation(gMatteProgram, "planeDepth");
    gMattePlaneCountLoc = glGetUniformLocation(gMatteProgram, "planeCount");
    gMatteViewportLoc = glGetUniformLocation(gMatteProgram, "viewportSize");
    gMatteOriginLoc = glGetUniformLocation(gMatteProgram, "viewportOrigin");
    gMatteBandLoc = glGetUniformLocation(gMatteProgram, "bandHalf");
    gMatteDebugLoc = glGetUniformLocation(gMatteProgram, "debugMode");
    glGenVertexArrays(1, &gVao); // core profile requires a VAO; attribs set per draw
    return true;
}

// Replace the trailing numeric field of a file name (the frame number, e.g.
// the "1001" in ".../shot.1001.exr") with 'frame', preserving zero-padding.
// Scans backward from the end, skipping any extension, and stops at the file
// name boundary so directory digits are never touched.
std::string substituteFrame(const std::string& path, int frame)
{
    size_t digitBeg = std::string::npos, digitEnd = std::string::npos;
    bool   inRun = false;
    for (size_t i = path.size(); i-- > 0;) {
        const char c = path[i];
        if (c == '/' || c == '\\')
            break;
        if (c >= '0' && c <= '9') {
            if (!inRun) {
                digitEnd = i + 1;
                inRun = true;
            }
            digitBeg = i;
        } else if (inRun) {
            break;
        }
    }
    if (!inRun)
        return path; // no frame field

    const size_t width = digitEnd - digitBeg;
    std::string  num = std::to_string(frame);
    if (num.size() < width)
        num = std::string(width - num.size(), '0') + num;
    return path.substr(0, digitBeg) + num + path.substr(digitEnd);
}

// Acquire (or reuse from cache) the GL texture for one resolved plate path.
// Uses the file-path loader (float + Maya color management, including alpha)
// with an MImage 8-bit fallback. Returns 0 on failure and fills outW/outH.
GLuint acquireOnePlate(
    MHWRender::MTextureManager* texMgr,
    const MString&              plateFile,
    const MString&              ipNodeName,
    int&                        outW,
    int&                        outH)
{
    outW = 0;
    outH = 0;

    const std::string    key(plateFile.asChar());
    MHWRender::MTexture* tex = nullptr;
    auto                 it = gTextureCache.find(key);
    if (it != gTextureCache.end())
        tex = it->second;

    if (!tex) {
        // File-path loader: decodes EXR as float (no 8-bit clamp) and ingests it
        // through the image plane's color management, matching the native plate;
        // the EXR alpha comes through as the texture's alpha channel.
        tex = texMgr->acquireTexture(
            plateFile, ipNodeName, /*mipmapLevels*/ 0, /*useExposureControl*/ false);

        if (!tex) {
            // Fallback: CPU decode (8-bit) so we at least show something.
            MImage img;
            if (img.readFromFile(plateFile) != MS::kSuccess) {
                static bool sWarned = false;
                if (!sWarned) {
                    sWarned = true;
                    MGlobal::displayWarning(
                        MString("[holdoutDepthPass] could not read image: '") + plateFile + "'");
                }
                return 0;
            }
            unsigned int w = 0, h = 0;
            img.getSize(w, h);
            if (w == 0 || h == 0)
                return 0;

            MHWRender::MTextureDescription desc;
            desc.setToDefault2DTexture();
            desc.fWidth = w;
            desc.fHeight = h;
            desc.fDepth = 1;
            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            desc.fBytesPerRow = w * 4;
            desc.fBytesPerSlice = w * h * 4;
            tex = texMgr->acquireTexture(plateFile, desc, img.pixels());
            if (!tex)
                return 0;
        }
        gTextureCache[key] = tex;
    }

    MHWRender::MTextureDescription d;
    tex->textureDescription(d);
    outW = static_cast<int>(d.fWidth);
    outH = static_cast<int>(d.fHeight);

    const GLuint* th = static_cast<const GLuint*>(tex->resourceHandle());
    return th ? *th : 0;
}

// Gather ALL image planes on the active camera, resolve each to its current
// frame, load its texture, and return them ordered back-to-front (index 0 =
// farthest) for compositing. Also returns the camera's film-gate aspect.
void gatherPlates(M3dView& view, float& outGateAspect, std::vector<PlaneTex>& outPlanes)
{
    outGateAspect = 1.0f;
    outPlanes.clear();

    MStatus  st;
    MDagPath camPath;
    if (view.getCamera(camPath) != MS::kSuccess)
        return;
    camPath.extendToShape(); // camera transform -> camera shape (no-op if shape)

    {
        MFnDependencyNode camN(camPath.node());
        double            ha = 0.0, va = 0.0;
        MPlug             hp = camN.findPlug("horizontalFilmAperture", false);
        MPlug             vp = camN.findPlug("verticalFilmAperture", false);
        if (!hp.isNull())
            hp.getValue(ha);
        if (!vp.isNull())
            vp.getValue(va);
        if (va > 1e-6)
            outGateAspect = static_cast<float>(ha / va);
    }

    // Collect every image plane on the camera. Primary: image planes connect to
    // the camera shape's "imagePlane" array plug (imagePlaneShape.message ->
    // cameraShape.imagePlane[n]); they are usually NOT DAG children.
    std::vector<MObject> planeObjs;
    {
        MFnDependencyNode camDepFn(camPath.node());
        MPlug             ipArray = camDepFn.findPlug("imagePlane", false, &st);
        if (st == MS::kSuccess && !ipArray.isNull()) {
            const unsigned int numEl = ipArray.numElements();
            for (unsigned int e = 0; e < numEl; ++e) {
                MPlug      elem = ipArray.elementByPhysicalIndex(e, &st);
                MPlugArray srcs;
                if (elem.connectedTo(srcs, true, false) && srcs.length() > 0) {
                    MObject node = srcs[0].node();
                    if (node.hasFn(MFn::kImagePlane))
                        planeObjs.push_back(node);
                }
            }
        }
    }
    // Fallback: some rigs parent the image plane(s) under the camera shape.
    if (planeObjs.empty()) {
        MFnDagNode         camFn(camPath);
        const unsigned int nChildren = camFn.childCount();
        for (unsigned int i = 0; i < nChildren; ++i) {
            MObject child = camFn.child(i);
            if (child.hasFn(MFn::kImagePlane))
                planeObjs.push_back(child);
        }
    }
    if (planeObjs.empty())
        return;

    MHWRender::MRenderer*       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* texMgr = renderer ? renderer->getTextureManager() : nullptr;
    if (!texMgr)
        return;

    std::unordered_set<std::string> currentKeys;

    for (const MObject& planeObj : planeObjs) {
        MFnDependencyNode ipFn(planeObj);
        const MString     ipNodeName = ipFn.name();

        MString plateFile;
        MPlug   nameP = ipFn.findPlug("imageName", false, &st);
        if (st == MS::kSuccess)
            nameP.getValue(plateFile);

        // Pixel aspect ratio stamped by user/pipeline (square = 1.0); anamorphic
        // plates are squeezed and need it to unsqueeze horizontally.
        float pixelAspect = 1.0f;
        MPlug parPlug = ipFn.findPlug("image_par", false, &st);
        if (st == MS::kSuccess && !parPlug.isNull()) {
            double par = 0.0;
            if (parPlug.getValue(par) == MS::kSuccess && par > 1e-6)
                pixelAspect = static_cast<float>(par);
        }

        // Per-plane depth: only used to order the planes back-to-front.
        double depth = 0.0;
        MPlug  depthPlug = ipFn.findPlug("depth", false, &st);
        if (st == MS::kSuccess && !depthPlug.isNull())
            depthPlug.getValue(depth);

        // Resolve the sequence frame from the plane's frame extension.
        bool  useFrameExt = false;
        MPlug ufePlug = ipFn.findPlug("useFrameExtension", false);
        if (!ufePlug.isNull())
            ufePlug.getValue(useFrameExt);
        if (useFrameExt && plateFile.length() > 0) {
            int   frameExt = 0;
            MPlug fePlug = ipFn.findPlug("frameExtension", false);
            if (!fePlug.isNull() && fePlug.getValue(frameExt) == MS::kSuccess && frameExt > 0)
                plateFile = MString(substituteFrame(plateFile.asChar(), frameExt).c_str());
        }
        if (plateFile.length() == 0)
            continue;

        int          w = 0, h = 0;
        const GLuint texName = acquireOnePlate(texMgr, plateFile, ipNodeName, w, h);
        if (texName == 0 || w <= 0 || h <= 0)
            continue;

        currentKeys.insert(std::string(plateFile.asChar()));

        // Anamorphic squeeze: visible horizontal fraction of the stored image is
        // gateAspect / (imageAspect * PAR). Sampling that center fraction
        // magnifies the content to match the native image plane.
        const float imageAspect = static_cast<float>(w) / static_cast<float>(h);
        float       hscale = 1.0f;
        if (imageAspect > 1e-6f && pixelAspect > 1e-6f)
            hscale = outGateAspect / (imageAspect * pixelAspect);

        PlaneTex pt;
        pt.texName = texName;
        pt.hscale = hscale;
        pt.depth = depth;
        outPlanes.push_back(pt);
    }

    // Release cached plates not used this frame (all planes of the current frame
    // must stay resident, so evict by "not in the current set" rather than a
    // count cap; this also keeps an image sequence from accumulating textures).
    std::vector<std::string> stale;
    for (const auto& kv : gTextureCache) {
        if (currentKeys.find(kv.first) == currentKeys.end())
            stale.push_back(kv.first);
    }
    for (const std::string& k : stale) {
        auto it = gTextureCache.find(k);
        if (it != gTextureCache.end()) {
            if (it->second)
                texMgr->releaseTexture(it->second);
            gTextureCache.erase(it);
        }
    }

    // Order back-to-front: largest depth (farthest) first, so nearer planes
    // composite 'over' farther ones.
    std::sort(outPlanes.begin(), outPlanes.end(), [](const PlaneTex& a, const PlaneTex& b) {
        return a.depth > b.depth;
    });
    // Cap to what the shader can composite; keep the nearest planes.
    if (outPlanes.size() > static_cast<size_t>(kMaxPlanes)) {
        outPlanes.erase(
            outPlanes.begin(),
            outPlanes.begin() + (outPlanes.size() - static_cast<size_t>(kMaxPlanes)));
    }
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

    // All image planes on this camera, ordered back-to-front. Empty (e.g. persp
    // view with no image plane) -> depth-only stamp so the holdout still occludes
    // CG, revealing the viewport background in its silhouette.
    float                 gateAspect = 1.0f;
    std::vector<PlaneTex> planes;
    gatherPlates(view, gateAspect, planes);
    const int  planeCount = static_cast<int>(planes.size());
    const bool havePlanes = planeCount > 0;

    // Policy for viewports with no plate (e.g. persp): by default the holdout
    // still occludes CG (revealing the viewport background in its silhouette).
    // Set MAYAUSD_HOLDOUT_OCCLUDE_WITHOUT_PLATE=0 to make it inert there.
    static const bool sOccludeWithoutPlate
        = TfGetenvBool("MAYAUSD_HOLDOUT_OCCLUDE_WITHOUT_PLATE", true);
    if (!havePlanes && !sOccludeWithoutPlate)
        return;

    // --- save the GL state we touch ------------------------------------
    GLint     prevProgram = 0, prevVao = 0, prevArrayBuf = 0, prevDepthFunc = GL_LESS;
    GLint     prevActiveTex = GL_TEXTURE0, prevViewport[4] = { 0, 0, 0, 0 };
    GLint     prevBlendSrcRGB = GL_ONE, prevBlendDstRGB = GL_ZERO;
    GLint     prevBlendSrcA = GL_ONE, prevBlendDstA = GL_ZERO;
    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLboolean prevDepthMask = GL_TRUE;
    GLboolean prevColorMask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    GLint     prevUnitTex[kMaxPlanes];
    for (int i = 0; i < kMaxPlanes; ++i)
        prevUnitTex[i] = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuf);
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &prevBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrcA);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDstA);
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

    // filmFit is horizontal: the horizontal projection scale is aspect-independent
    // and correct, but the vertical scale (element 1,1) follows the render aspect.
    // Under playblast the M3dView still reports the interactive panel's aspect, so
    // the holdout mesh would project vertically compressed into the (wider)
    // output. Rebuild (1,1) from the actual render dims so the mesh matches the
    // output. No-op interactively, where panel aspect == render aspect.
    MMatrix renderProjection = projection;
    if (vh != 0 && projection(0, 0) != 0.0) {
        double pm[4][4];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                pm[r][c] = projection(r, c);
        pm[1][1] = pm[0][0] * static_cast<double>(vw) / static_cast<double>(vh);
        renderProjection = MMatrix(pm);
    }

    // Vertical band half-extent in NDC: the plates fill the frame width and have
    // the gate display aspect, so the band fraction is aFrame/gateAspect, derived
    // from the render dims (correct under playblast).
    float bandHalf = 1.0f;
    if (vh != 0 && gateAspect > 1e-6f)
        bandHalf = static_cast<float>((static_cast<double>(vw) / static_cast<double>(vh))
                                      / static_cast<double>(gateAspect));

    float hscales[kMaxPlanes];
    for (int i = 0; i < kMaxPlanes; ++i)
        hscales[i] = 1.0f;
    for (int i = 0; i < planeCount; ++i)
        hscales[i] = planes[i].hscale;

    // Per-plane window-space depth, so the shader can tell (per pixel) which
    // planes are behind the holdout surface. Project a camera-space point at the
    // plane's depth (camera looks down -Z) through the same projection used for
    // the mesh; window z = ndc.z * 0.5 + 0.5, matching gl_FragCoord.z.
    float planeDepth[kMaxPlanes];
    for (int i = 0; i < kMaxPlanes; ++i)
        planeDepth[i] = 1.0f;
    for (int i = 0; i < planeCount; ++i) {
        const double D = planes[i].depth;
        const double cz = -D * renderProjection(2, 2) + renderProjection(3, 2);
        const double cw = -D * renderProjection(2, 3) + renderProjection(3, 3);
        planeDepth[i] = (cw != 0.0) ? static_cast<float>((cz / cw) * 0.5 + 0.5) : 1.0f;
    }

    // Common shader state (mvp is set per entry).
    glUseProgram(gMatteProgram);
    const GLint sUnits[kMaxPlanes] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    glUniform1iv(gMattePlatesLoc, kMaxPlanes, sUnits);
    glUniform1fv(gMatteHScalesLoc, kMaxPlanes, hscales);
    glUniform1fv(gMattePlaneDepthLoc, kMaxPlanes, planeDepth);
    glUniform2f(gMatteViewportLoc, static_cast<float>(vw), static_cast<float>(vh));
    glUniform2f(gMatteOriginLoc, static_cast<float>(vx), static_cast<float>(vy));
    glUniform1f(gMatteBandLoc, bandHalf);
    glUniform1i(gMatteDebugLoc, kMatteDebugMode);
    glBindVertexArray(gVao);

    // Draw all published holdout entries (honouring prim and proxy visibility).
    auto drawEntries = [&]() {
        for (const Entry& e : entries) {
            if (e.posHandle == 0 || e.idxHandle == 0 || e.indexCount == 0)
                continue;
            if (!e.visible)
                continue;
            if (e.proxyDagPath.isValid() && !e.proxyDagPath.isVisible())
                continue;

            const MMatrix mvp = e.world * modelView * renderProjection; // row-vector compose
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
    };

    // PASS 1 -- depth stamp only. LESS + depth write records the nearest holdout
    // surface per pixel so the scene occludes CG behind it. No color, no sampling
    // (planeCount = 0). This is the whole stamp when there is no plate.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glUniform1i(gMattePlaneCountLoc, 0);
    drawEntries();

    // PASS 2 -- composite the plates as color, exactly once per pixel. EQUAL
    // (against the depth just stamped) admits only the nearest fragment, so a
    // concave mesh does not composite the plates multiple times. Depth writes are
    // off here; the premultiplied result blends over the viewport background.
    if (havePlanes) {
        for (int i = 0; i < planeCount; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevUnitTex[i]);
            glBindTexture(GL_TEXTURE_2D, planes[i].texName);
        }
        glUniform1i(gMattePlaneCountLoc, planeCount);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied 'over'
        drawEntries();
    }

    // --- restore -------------------------------------------------------
    for (int i = 0; i < planeCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevUnitTex[i]));
    }
    glActiveTexture(static_cast<GLenum>(prevActiveTex));
    if (prevBlend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    glBlendFuncSeparate(static_cast<GLenum>(prevBlendSrcRGB),
                        static_cast<GLenum>(prevBlendDstRGB),
                        static_cast<GLenum>(prevBlendSrcA),
                        static_cast<GLenum>(prevBlendDstA));
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
    if (!gRegistered)
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
    unsigned int                  indexCount,
    const MMatrix&                worldMatrix,
    const MDagPath&               proxyDagPath,
    bool                          visible)
{
    if (!key || !positionBuffer || !indexBuffer)
        return;

    // Read the GL resource handles NOW, at commit time, while the buffers are
    // guaranteed alive. The render callback later uses only these cached ints,
    // never calling resourceHandle() on a buffer that may have been freed --
    // that is to fix the crash when clearing Maya scene (OGSMayaVertexBuffer).
    const GLuint* ph = static_cast<const GLuint*>(positionBuffer->resourceHandle());
    const GLuint* ih = static_cast<const GLuint*>(indexBuffer->resourceHandle());

    Entry e;
    e.posHandle = ph ? *ph : 0;
    e.idxHandle = ih ? *ih : 0;
    e.indexCount = indexCount;
    e.world = worldMatrix;
    e.proxyDagPath = proxyDagPath;
    e.visible = visible;

    std::lock_guard<std::mutex> lock(gMutex);
    gRegistry[key] = e;

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

void SetVisible(const MHWRender::MRenderItem* key, bool visible)
{
    // Lightweight visibility toggle used on DirtyVisibility, when the geometry
    // commit (and thus Publish/Unpublish) does not run. No-op if not published.
    std::lock_guard<std::mutex> lock(gMutex);
    auto                        it = gRegistry.find(key);
    if (it != gRegistry.end())
        it->second.visible = visible;
}

} // namespace HdVP2HoldoutDepthPass

PXR_NAMESPACE_CLOSE_SCOPE