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
#ifndef MAYAUSD_VP2_SHADOW_ONLY_SHADER_H
#define MAYAUSD_VP2_SHADOW_ONLY_SHADER_H

#include <mayaUsd/base/api.h>

// MApiNamespace forward-declares the Maya types; including MFnPlugin.h here
// would define MApiVersion in every translation unit that pulls this in.
#include <maya/MApiNamespace.h>

namespace MAYAUSD_NS_DEF {

/// Shading node whose MPxShaderOverride declines shadow passes and silently
/// consumes every other pass.
///
/// This is how cameraVisibility is emulated. VP2 has no per-render-item way to
/// say "not visible to camera but still casts shadows": drawMode, transparency
/// and the shape-level render stats were all tried and none separate the beauty
/// pass from the shadow pass. MPxShaderOverride::handlesDraw() is the one API
/// that is pass-aware -- returning false hands the pass back to Maya, so
/// declining only the shadow semantics leaves Maya to render the shadow while
/// we swallow the colour pass without drawing.
///
/// Reaching handlesDraw() at all requires the render item's shader to come from
/// a shading node (MRenderItem::setShaderFromNode2), which is why this node type
/// exists; maya-usd otherwise assigns MShaderInstances directly.
class ShadowOnlyShader
{
public:
    MAYAUSD_CORE_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_CORE_PUBLIC
    static const char* typeName;
    /// Must match the classification the shader override is registered against.
    MAYAUSD_CORE_PUBLIC
    static const char* classification;

    /// Passed to MFnPlugin::registerNode by the plugin. Registration itself has
    /// to happen there: MFnPlugin.h defines MApiVersion, so exactly one
    /// translation unit in the library may include it, and that is already taken.
    MAYAUSD_CORE_PUBLIC
    static void* nodeCreator();
    MAYAUSD_CORE_PUBLIC
    static MStatus nodeInitialize();

    /// Registers the MPxShaderOverride against `classification`. Call after the
    /// node type is registered.
    MAYAUSD_CORE_PUBLIC
    static MStatus registerOverride();

    MAYAUSD_CORE_PUBLIC
    static MStatus deregisterOverride();

    /// Creates the single shared node instance if it does not already exist.
    ///
    /// MUST be called from the main thread: it modifies the dependency graph,
    /// and render items are built on TBB worker threads during Hydra sync.
    /// The node does not survive a new scene, so this re-creates as needed.
    MAYAUSD_CORE_PUBLIC
    static MStatus ensureSharedNode();

    /// Accessor only, safe from worker threads. Returns a null MObject if
    /// ensureSharedNode() has not run or the node type was never registered, in
    /// which case callers should leave the render item's shader alone.
    MAYAUSD_CORE_PUBLIC
    static MObject getSharedNode();
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_VP2_SHADOW_ONLY_SHADER_H
