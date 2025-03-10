//
// Copyright 2024 Autodesk
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

#ifndef MAYAUSD_PROXYACCESSORUTIL_H
#define MAYAUSD_PROXYACCESSORUTIL_H

#include <mayaUsd/undo/OpUndoItemList.h>

#include <maya/MDGModifier.h>
#include <maya/MDagPath.h>
#include <maya/MStatus.h>
#include <ufe/path.h>

#include <string>

namespace MAYAUSD_NS_DEF {
namespace utils {

/// \brief OpUndoItem for proxyAccessor parenting.
class ProxyAccessorUndoItem : public OpUndoItem
{
public:
    /// \brief create proxy Accessor recorder and keep track of it in the global
    /// undo item list. Parents the pulled maya object at \p pulledDagPath under
    /// the UFE USD item at \p ufeParentPath.
    /// If \p force is \c true, the pulled object will first be un-parented from
    /// its current USD parent. Otherwise an error will occur if the child is
    /// already parented to USD.
    /// Returns \c MStatus::kSuccess if the parenting succeeded.
    static MStatus parentPulledObject(
        const std::string name,
        const MDagPath&   pulledDagPath,
        const Ufe::Path&  ufeParentPath,
        bool              force = false);

    /// \brief construct a proxy accessor recorder.
    ProxyAccessorUndoItem(std::string name)
        : OpUndoItem(std::move(name))
    {
    }

    ~ProxyAccessorUndoItem() override;

    /// \brief undo a single sub-operation.
    bool undo() override;

    /// \brief redo a single sub-operation.
    bool redo() override;

private:
    MDGModifier _modifier;
};

} // namespace utils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_PROXYACCESSORUTIL_H
