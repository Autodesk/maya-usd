//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_RENDER_PARAMS_H
#define PXRUSDMAYAGL_RENDER_PARAMS_H

/// \file pxrUsdMayaGL/renderParams.h

#include <pxr/base/gf/vec4f.h>
#include <pxr/pxr.h>

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

struct PxrMayaHdRenderParams
{
    // Raster Params
    //
    bool enableLighting = true;

    // Color Params
    //
    bool    useWireframe = false;
    GfVec4f wireframeColor = GfVec4f(0.0f);

    /// Helper function to find a batch key for the render params
    size_t Hash() const
    {
        size_t hash = size_t(enableLighting);
        boost::hash_combine(hash, useWireframe);
        boost::hash_combine(hash, wireframeColor);

        return hash;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
