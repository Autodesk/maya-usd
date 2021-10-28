//
// Copyright 2021 Autodesk
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
#ifndef MAYAUSD_TRAVERSELAYER_H
#define MAYAUSD_TRAVERSELAYER_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/layer.h>

#include <stdexcept>

namespace MAYAUSD_NS_DEF {

//! \brief Exception class to signal traversal failure.
//
// Layer traversal functions can throw this exception to signal traversal
// failure, as it is caught by traverseLayer().

class TraversalFailure : public std::runtime_error
{
public:
    TraversalFailure(const std::string& reason, const PXR_NS::SdfPath& path)
        : std::runtime_error(path.GetString())
        , _reason(reason)
        , _path(path)
    {
    }
    TraversalFailure(const TraversalFailure&) = default;
    ~TraversalFailure() override { }

    std::string     reason() const { return _reason; }
    PXR_NS::SdfPath path() const { return _path; }

private:
    const std::string     _reason;
    const PXR_NS::SdfPath _path;
};

//! \brief Type definition for layer traversal function.
//
// A layer traversal function must return true to continue the traversal, and
// false to prune traversal to the children of the argument path.  The
// traversal function should use the TraversalFailure exception to report
// failure.
typedef std::function<bool(const PXR_NS::SdfPath&)> TraverseLayerFn;

/*! \brief Layer traversal utility.

  Pre-order depth-first traversal of layer, starting at path, so that parents
  are traversed before children.  SdfLayer::Traverse() is depth-first,
  post-order, in which case the parent is traversed after the children.

  Catches the TraversalFailure exception, and returns false on traversal
  failure.
 */
MAYAUSD_CORE_PUBLIC
bool traverseLayer(
    const PXR_NS::SdfLayerHandle& layer,
    const PXR_NS::SdfPath&        path,
    const TraverseLayerFn&        fn);

} // namespace MAYAUSD_NS_DEF

#endif
