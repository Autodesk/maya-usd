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

#include <cstdlib>
#include <mutex>
#include <string>
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
    // Owning proxy shape; used to skip holdouts whose proxy node is hidden in
    // Maya (hiding the proxy does not re-Sync the prims, so we check at draw).
    MDagPath     proxyDagPath;
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
GLint  gMatteOriginLoc { -1 };
GLint  gMatteBandLoc { -1 };
GLint  gMatteHScaleLoc { -1 };
GLint  gMatteDebugLoc { -1 };

// Debug: 0 = plate, 1 = solid red (depth-mask test), 2 = UV visualization
// (red = U, green = V) to see the plate-mapping directly.
const int kMatteDebugMode = 0;

std::unordered_map<std::string, MHWRender::MTexture*> gTextureCache;
// Plates are loaded per resolved frame (image planes use frame extension) as
// linear float (~4 bytes/channel), so cap the cache to bound memory; cleared
// wholesale when exceeded.
const size_t kMaxCachedPlates = 4;

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
                                "uniform vec2 viewportOrigin;\n" // glViewport (vx, vy)
                                "uniform float bandHalf;\n" // plate NDC y half-extent
                                "uniform float hscale;\n"   // horizontal sample scale (anamorphic)
                                "uniform int debugMode;\n"
                                "void main() {\n"
                                "    if (debugMode == 1) { fragColor = vec4(1.0, 0.0, 0.0, 1.0); return; }\n"
                                "    // gl_FragCoord is in window coords spanning [origin, origin+size];\n"
                                "    // make it panel-relative so an offset (non-maximized) panel maps right.\n"
                                "    float sx = (gl_FragCoord.x - viewportOrigin.x) / viewportSize.x;\n"
                                "    float sy = (gl_FragCoord.y - viewportOrigin.y) / viewportSize.y;\n"
                                "    // Anamorphic 'To Size': plate is stretched to the film gate, so it\n"
                                "    // fills the frame width and spans an NDC vertical band of +/-bandHalf\n"
                                "    // (derived from the projection so cameraScale/overscan are included).\n"
                                "    // hscale samples the center fraction horizontally to undo the squeeze.\n"
                                "    float u = 0.5 + (sx - 0.5) * hscale;\n"
                                "    float ndcy = sy * 2.0 - 1.0;\n"
                                "    float v = (ndcy / bandHalf + 1.0) * 0.5;\n"
                                "    if (debugMode == 2) { fragColor = vec4(clamp(u,0.0,1.0), clamp(v,0.0,1.0), 0.0, 1.0); return; }\n"
                                "    vec2 uv = clamp(vec2(u, 1.0 - v), 0.0, 1.0);\n"
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
    gMatteOriginLoc = glGetUniformLocation(gMatteProgram, "viewportOrigin");
    gMatteBandLoc = glGetUniformLocation(gMatteProgram, "bandHalf");
    gMatteHScaleLoc = glGetUniformLocation(gMatteProgram, "hscale");
    gMatteDebugLoc = glGetUniformLocation(gMatteProgram, "debugMode");
    glGenVertexArrays(1, &gVao); // core profile requires a VAO; attribs set per draw
    return true;
}

// Find the active camera's first image plane, load its image (cached by path),
// and return the GL texture name. 0 on failure.
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

GLuint acquirePlateGLTexture(
    M3dView& view,
    int&     outW,
    int&     outH,
    float&   outGateAspect,
    float&   outPixelAspect)
{
    outW = 0;
    outH = 0;
    outGateAspect = 1.0f;
    outPixelAspect = 1.0f;
    MStatus  st;
    MDagPath camPath;
    if (view.getCamera(camPath) != MS::kSuccess)
        return 0;
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

    MString    plateFile;
    MString    ipNodeName;
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
        ipNodeName = ipFn.name();
        MPlug             p = ipFn.findPlug("imageName", false, &st);
        if (st == MS::kSuccess)
            p.getValue(plateFile);

        // Pixel aspect ratio stamped by the load pipeline (square = 1.0).
        // Anamorphic plates are squeezed; PAR is needed to unsqueeze horizontally.
        MStatus parSt;
        MPlug   parPlug = ipFn.findPlug("hh_image_par", false, &parSt);
        if (parSt == MS::kSuccess && !parPlug.isNull()) {
            double par = 0.0;
            if (parPlug.getValue(par) == MS::kSuccess && par > 1e-6)
                outPixelAspect = static_cast<float>(par);
        }

        // Resolve the sequence frame. The plane uses frame extension driven by
        // the time node, so imageName holds only the base frame; replace its
        // frame number with the current frameExtension (evaluated at draw time).
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

        static bool sLoggedPlane = false;
        if (!sLoggedPlane) {
            sLoggedPlane = true;
            auto dbl = [&](const char* n) {
                double v = 0.0;
                MPlug  pp = ipFn.findPlug(n, false);
                if (!pp.isNull())
                    pp.getValue(v);
                return v;
            };
            auto integ = [&](const char* n) {
                int   v = 0;
                MPlug pp = ipFn.findPlug(n, false);
                if (!pp.isNull())
                    pp.getValue(v);
                return v;
            };
            MFnDependencyNode camN(camPath.node());
            auto              cdbl = [&](const char* n) {
                double v = 0.0;
                MPlug  pp = camN.findPlug(n, false);
                if (!pp.isNull())
                    pp.getValue(v);
                return v;
            };
            auto cinteg = [&](const char* n) {
                int   v = 0;
                MPlug pp = camN.findPlug(n, false);
                if (!pp.isNull())
                    pp.getValue(v);
                return v;
            };
            MGlobal::displayInfo(
                MString("[holdoutDepthPass] PLANE fit=") + integ("fit") + " size=("
                + dbl("sizeX") + "," + dbl("sizeY") + ") offset=(" + dbl("offsetX") + ","
                + dbl("offsetY") + ") depth=" + dbl("depth") + " maintainRatio="
                + integ("maintainRatio") + " coverage=(" + integ("coverageX") + ","
                + integ("coverageY") + ") covOrigin=(" + integ("coverageOriginX") + ","
                + integ("coverageOriginY") + ")");
            MGlobal::displayInfo(
                MString("[holdoutDepthPass] CAM hAperture=") + cdbl("horizontalFilmAperture")
                + " vAperture=" + cdbl("verticalFilmAperture") + " focal=" + cdbl("focalLength")
                + " filmFit=" + cinteg("filmFit") + " ortho=" + cdbl("orthographic")
                + " orthoWidth=" + cdbl("orthographicWidth"));
        }
    }

    if (plateFile.length() == 0)
        return 0;

    const std::string    key(plateFile.asChar());
    MHWRender::MTexture* tex = nullptr;
    auto                 it = gTextureCache.find(key);
    if (it != gTextureCache.end())
        tex = it->second;

    if (!tex) {
        MHWRender::MRenderer*       renderer = MHWRender::MRenderer::theRenderer();
        MHWRender::MTextureManager* texMgr = renderer ? renderer->getTextureManager() : nullptr;
        if (!texMgr)
            return 0;

        // Load via the texture manager's file path API: it decodes EXR as float
        // (no 8-bit clamp, unlike MImage::readFromFile here) and ingests it
        // through the image plane's color management (input colorSpace ->
        // rendering space), matching how Maya brings in the native plate.
        tex = texMgr->acquireTexture(
            plateFile, ipNodeName, /*mipmapLevels*/ 0, /*useExposureControl*/ false);

        if (!tex) {
            // Fallback: CPU decode (8-bit) so we at least show something, and
            // make the limitation visible.
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

        static bool sLoggedFmt = false;
        if (!sLoggedFmt) {
            sLoggedFmt = true;
            MHWRender::MTextureDescription ld;
            tex->textureDescription(ld);
            MGlobal::displayInfo(
                MString("[holdoutDepthPass] PLATE load ") + ld.fWidth + "x" + ld.fHeight
                + " texFormat=" + static_cast<int>(ld.fFormat) + " (16/19/22=float-ish, 1/2=8-bit)");
        }

        // Bound memory: per-frame loads accumulate, so clear when over the cap.
        if (gTextureCache.size() >= kMaxCachedPlates) {
            for (auto& kv : gTextureCache) {
                if (kv.second)
                    texMgr->releaseTexture(kv.second);
            }
            gTextureCache.clear();
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
    int          imgW = 0, imgH = 0;
    float        gateAspect = 1.0f;
    float        pixelAspect = 1.0f;
    const GLuint texName = acquirePlateGLTexture(view, imgW, imgH, gateAspect, pixelAspect);
    const bool   havePlate = (texName != 0 && imgW > 0 && imgH > 0);

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

    // filmFit is horizontal (the pipeline forces it): the horizontal projection
    // scale is aspect-independent and correct, but the vertical scale (element
    // 1,1) follows the render aspect. Under playblast the M3dView still reports
    // the interactive panel's aspect, so the holdout mesh would project
    // vertically compressed into the (wider) output. Rebuild (1,1) from the
    // actual render dims so the mesh matches the output. This is a no-op
    // interactively, where panel aspect == render aspect.
    MMatrix renderProjection = projection;
    if (vh != 0 && projection(0, 0) != 0.0) {
        double pm[4][4];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                pm[r][c] = projection(r, c);
        pm[1][1] = pm[0][0] * static_cast<double>(vw) / static_cast<double>(vh);
        renderProjection = MMatrix(pm);
    }

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
    // Vertical band half-extent in NDC. The plate fills the frame width and has
    // the gate display aspect, so the band fraction is just aFrame/gateAspect.
    // Derive aFrame from the render-target dims (correct under playblast) rather
    // than proj11/proj00: during playblast the M3dView projection can still carry
    // the interactive panel's aspect, which sized the band wrongly. (proj00/proj11
    // are kept only for the diagnostic.)
    const double p00 = projection(0, 0);
    const double p11 = projection(1, 1);
    float        bandHalf = 1.0f;
    if (vh != 0 && gateAspect > 1e-6f)
        bandHalf = static_cast<float>((static_cast<double>(vw) / static_cast<double>(vh))
                                      / static_cast<double>(gateAspect));

    // Horizontal anamorphic squeeze. The plate is unsqueezed by its pixel aspect
    // ratio and covers the gate, so the visible horizontal fraction of the stored
    // image is gateAspect / (imageAspect * PAR). Sampling that center fraction
    // magnifies the content to match the native image plane.
    const float imageAspect = (imgH > 0) ? static_cast<float>(imgW) / static_cast<float>(imgH) : 1.0f;
    float       hscale = 1.0f;
    if (imageAspect > 1e-6f && pixelAspect > 1e-6f)
        hscale = gateAspect / (imageAspect * pixelAspect);
    // Optional manual override for debugging (live via os.environ).
    if (const char* hs = std::getenv("MAYAUSD_HOLDOUT_HSCALE")) {
        const float v = static_cast<float>(std::atof(hs));
        if (v > 0.0f)
            hscale = v;
    }

    glUniform2f(gMatteViewportLoc, static_cast<float>(vw), static_cast<float>(vh));
    glUniform2f(gMatteOriginLoc, static_cast<float>(vx), static_cast<float>(vy));
    glUniform1f(gMatteBandLoc, bandHalf);
    glUniform1f(gMatteHScaleLoc, hscale);
    glUniform1i(gMatteDebugLoc, kMatteDebugMode);

    if (havePlate) {
        static bool sLoggedAspect = false;
        if (!sLoggedAspect) {
            sLoggedAspect = true;
            MGlobal::displayInfo(
                MString("[holdoutDepthPass] BAND vw=") + vw + " vh=" + vh + " vx=" + vx + " vy="
                + vy + " glVP=(" + prevViewport[0] + "," + prevViewport[1] + "," + prevViewport[2]
                + "," + prevViewport[3] + ") gateAspect=" + gateAspect + " proj00=" + p00
                + " proj11=" + p11 + " bandHalf=" + bandHalf + " PAR=" + pixelAspect
                + " imageAspect=" + imageAspect + " hscale=" + hscale);
        }
    }
    glBindVertexArray(gVao);

    for (const Entry& e : entries) {
        if (e.posHandle == 0 || e.idxHandle == 0 || e.indexCount == 0)
            continue;

        // Skip if the owning proxy node is hidden in Maya (visibility, parent
        // visibility, display layer, etc.). Hiding the proxy does not re-Sync the
        // prims, so the entry stays published; we gate it here at draw time.
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
    const MMatrix&                worldMatrix,
    const MDagPath&               proxyDagPath)
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
    e.proxyDagPath = proxyDagPath;

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

} // namespace HdVP2HoldoutDepthPass

PXR_NAMESPACE_CLOSE_SCOPE