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
#ifndef PXRUSDMAYAGL_SOFT_SELECT_HELPER_H
#define PXRUSDMAYAGL_SOFT_SELECT_HELPER_H

/// \file pxrUsdMayaGL/softSelectHelper.h

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/hash.h>
#include <pxr/pxr.h>

#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MRampAttribute.h>
#include <maya/MString.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaGLSoftSelectHelper
/// \brief Helper class to store soft ("rich") selection state while
/// computing render params for a frame.
///
/// When rendering, we want to be able to draw things that will be influenced by
/// soft selection with a different wireframe.  Querying this maya state is too
/// expensive do in the middle of the render loop so this class lets us compute
/// it once at the beginning of a frame render, and then query it later.
///
/// While this class doesn't have anything particular to rendering, it is only
/// used by the render and is therefore here.  We can move this to usdMaya if
/// we'd like to use it outside of the rendering.
class UsdMayaGLSoftSelectHelper
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaGLSoftSelectHelper();

    /// \brief Clears the saved soft selection state.
    MAYAUSD_CORE_PUBLIC
    void Reset();

    /// \brief Repopulates soft selection state
    MAYAUSD_CORE_PUBLIC
    void Populate();

    /// \brief Returns true if \p dagPath is in the softSelection.  Also returns
    /// the \p weight.
    ///
    /// NOTE: until MAYA-73448 (and MAYA-73513) is fixed, the \p weight value is
    /// arbitrary.
    MAYAUSD_CORE_PUBLIC
    bool GetWeight(const MDagPath& dagPath, float* weight) const;

    /// \brief Returns true if \p dagPath is in the softSelection.  Also returns
    /// the appropriate color based on the distance/weight and the current soft
    /// select color curve.  It will currently always return (0, 0, 1) at the
    /// moment.
    MAYAUSD_CORE_PUBLIC
    bool GetFalloffColor(const MDagPath& dagPath, MColor* falloffColor) const;

private:
    void _PopulateWeights();
    void _PopulateSoftSelectColorRamp();

    struct _MDagPathHash
    {
        inline size_t operator()(const MDagPath& dagPath) const
        {
            return TfHash()(std::string(dagPath.fullPathName().asChar()));
        }
    };
    typedef std::unordered_map<MDagPath, float, _MDagPathHash> _MDagPathsToWeights;

    _MDagPathsToWeights _dagPathsToWeight;
    MColor              _wireColor;
    bool                _populated;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYAGL_SOFT_SELECT_HELPER_H
