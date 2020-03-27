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

#include "pxr/pxr.h"

#include "../../base/api.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

#include <boost/functional/hash.hpp>


PXR_NAMESPACE_OPEN_SCOPE


struct PxrMayaHdRenderParams
{
    // Raster Params
    //
    bool enableLighting = true;

    // Color Params
    //
    GfVec4f wireframeColor = GfVec4f(0.0f);

    /// Custom bucketing on top of the regular bucketing based on render params.
    /// Leave this as the empty token if you want to use the default bucket for
    /// these params, along with its associated Hydra tasks.
    /// Set this to a non-empty token if you want to render with separate
    /// Hydra tasks, since these are allocated on a per-bucket basis.
    TfToken customBucketName;

    /// Helper function to find a batch key for the render params
    size_t Hash() const
    {
        size_t hash = size_t(enableLighting);
        boost::hash_combine(hash, wireframeColor);
        boost::hash_combine(hash, customBucketName);

        return hash;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
