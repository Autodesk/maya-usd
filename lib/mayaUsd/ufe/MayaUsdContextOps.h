//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSD_MAYAUSDCONTEXTOPS_H
#define MAYAUSD_MAYAUSDCONTEXTOPS_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdContextOps.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for Maya scene item context operations.
/*!
    This class defines the interface that USD run-time implements to
    provide contextual operation support (example Outliner context menu).

    This class is not copy-able, nor move-able.

    \see UFE ContextOps class documentation for more details
*/
class MAYAUSD_CORE_PUBLIC MayaUsdContextOps : public UsdUfe::UsdContextOps
{
public:
    typedef UsdUfe::UsdContextOps              Parent;
    typedef std::shared_ptr<MayaUsdContextOps> Ptr;

    MayaUsdContextOps(const UsdUfe::UsdSceneItem::Ptr& item);

    //! Create a MayaUsdContextOps.
    static MayaUsdContextOps::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

    // UsdUfe::UsdContextOps overrides
    Items                     getItems(const ItemPath& itemPath) const override;
    Ufe::UndoableCommand::Ptr doOpCmd(const ItemPath& itemPath) override;

    Items                     getBulkItems(const ItemPath& itemPath) const override;
    Ufe::UndoableCommand::Ptr doBulkOpCmd(const ItemPath& itemPath) override;

    UsdUfe::UsdContextOps::SchemaNameMap getSchemaPluginNiceNames() const override;

private:
    void enableOutlinerClassPrims() const;

}; // MayaUsdContextOps

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYAUSDCONTEXTOPS_H
