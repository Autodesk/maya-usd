//
// Copyright 2016 Pixar
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
#ifndef PXVP20_UTILS_H
#define PXVP20_UTILS_H

/// \file px_vp20/utils.h

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/imaging/garch/gl.h>
#include <pxr/imaging/glf/simpleLightingContext.h>
#include <pxr/pxr.h>

#include <maya/M3dView.h>
#include <maya/MBoundingBox.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MSelectionContext.h>

#include <array>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

class px_vp20Utils
{
public:
    /// Take VP2.0 lighting information and import it into opengl lights
    MAYAUSD_CORE_PUBLIC
    static bool setupLightingGL(const MHWRender::MDrawContext& context);
    MAYAUSD_CORE_PUBLIC
    static void unsetLightingGL(const MHWRender::MDrawContext& context);

    /// Translate a Maya MDrawContext into a GlfSimpleLightingContext.
    MAYAUSD_CORE_PUBLIC
    static GlfSimpleLightingContextRefPtr
    GetLightingContextFromDrawContext(const MHWRender::MDrawContext& context);

    /// Tries to get the viewport for the given draw context.
    ///
    /// Returns true if the viewport was found, in which case it is
    /// returned in the \p view parameter.
    /// Returns false if there's not a 3D viewport (e.g. we're drawing into
    /// a render view).
    MAYAUSD_CORE_PUBLIC
    static bool GetViewFromDrawContext(const MHWRender::MDrawContext& context, M3dView& view);

    /// Returns true if the given Maya display style indicates that a
    /// bounding box should be rendered.
    static bool ShouldRenderBoundingBox(unsigned int displayStyle)
    {
        const bool boundingBoxStyle
            = displayStyle & MHWRender::MFrameContext::DisplayStyle::kBoundingBox;
        return boundingBoxStyle;
    }

    /// Renders the given bounding box in the given \p color via OpenGL.
    MAYAUSD_CORE_PUBLIC
    static bool RenderBoundingBox(
        const MBoundingBox& bounds,
        const GfVec4f&      color,
        const MMatrix&      worldViewMat,
        const MMatrix&      projectionMat);

    /// Helper to draw multiple wireframe boxes, where \p cubeXforms is a
    /// list of transforms to apply to the unit cube centered around the
    /// origin.  Those transforms will all be concatenated with the
    /// \p worldViewMat and \p projectionMat.
    MAYAUSD_CORE_PUBLIC
    static bool RenderWireCubes(
        const std::vector<GfMatrix4f>& cubeXforms,
        const GfVec4f&                 color,
        const GfMatrix4d&              worldViewMat,
        const GfMatrix4d&              projectionMat);

    /// Gets the view and projection matrices based on a particular
    /// selection in the given draw context.
    MAYAUSD_CORE_PUBLIC
    static bool GetSelectionMatrices(
        const MHWRender::MSelectionInfo& selectionInfo,
        const MHWRender::MDrawContext&   context,
        GfMatrix4d&                      viewMatrix,
        GfMatrix4d&                      projectionMatrix);

    /// Outputs a human-readable form of the given \p displayStyle to
    /// \p stream for debugging.
    ///
    /// \p displayStyle should be a bitwise combination of
    /// MHWRender::MFrameContext::DisplayStyle values.
    MAYAUSD_CORE_PUBLIC
    static void OutputDisplayStyleToStream(const unsigned int displayStyle, std::ostream& stream);

    /// Outputs a human-readable form of the given \p displayStatus to
    /// \p stream for debugging.
    MAYAUSD_CORE_PUBLIC
    static void
    OutputDisplayStatusToStream(const MHWRender::DisplayStatus displayStatus, std::ostream& stream);

private:
    px_vp20Utils() = delete;
    ~px_vp20Utils() = delete;
};

/// Simple RAII class to save uniform buffer bindings, to deal with a Maya
/// issue.
///
/// XXX: When Maya is using OpenGL Core Profile as the rendering engine (in
/// either compatibility or strict mode), batch renders like those done in the
/// "Render View" window or through the ogsRender command do not properly track
/// uniform buffer binding state. This was causing issues where the first batch
/// render performed would look correct, but then all subsequent renders done
/// in that Maya session would be completely black (no alpha), even if the
/// frame contained only Maya-native geometry or if a new scene was
/// created/opened.
///
/// To avoid this problem, this object can be used to save and restore Maya's
/// uniform buffer bindings across Hydra/OpenGL calls. We try not to bog down
/// performance by saving and restoring *all* GL_MAX_UNIFORM_BUFFER_BINDINGS
/// possible bindings, so instead we only do just enough to avoid issues.
/// Empirically, the problematic binding has been the material binding at
/// index 4.
class GLUniformBufferBindingsSaver
{
public:
    MAYAUSD_CORE_PUBLIC
    GLUniformBufferBindingsSaver();

    MAYAUSD_CORE_PUBLIC
    ~GLUniformBufferBindingsSaver();

private:
    static constexpr size_t UNIFORM_BINDINGS_TO_SAVE = 5u;

    std::array<GLint, UNIFORM_BINDINGS_TO_SAVE> _uniformBufferBindings;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
