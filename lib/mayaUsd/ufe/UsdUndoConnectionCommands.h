//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_UFE_USDUNDOCONNECTIONCOMMANDS_H
#define MAYAUSD_UFE_USDUNDOCONNECTIONCOMMANDS_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/attribute.h>
#include <ufe/undoableCommand.h>

#include <memory>

UFE_NS_DEF
{
    class AttributeInfo;
    class Connection;
}

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCreateConnectionCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCreateConnectionCommand
    : public Ufe::ConnectionResultUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateConnectionCommand> Ptr;

    UsdUndoCreateConnectionCommand(
        const Ufe::Attribute::Ptr& srcAttr,
        const Ufe::Attribute::Ptr& dstAttr);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoCreateConnectionCommand);

    //! Create a UsdUndoCreateConnectionCommand from two attributes.
    static Ptr create(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "CreateConnection"; })

    std::shared_ptr<Ufe::Connection> connection() const override;

private:
    UsdUfe::UsdUndoableItem             _undoableItem;
    std::unique_ptr<Ufe::AttributeInfo> _srcInfo;
    std::unique_ptr<Ufe::AttributeInfo> _dstInfo;
}; // UsdUndoCreateConnectionCommand

//! \brief UsdUndoDeleteConnectionCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDeleteConnectionCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoDeleteConnectionCommand> Ptr;

    UsdUndoDeleteConnectionCommand(
        const Ufe::Attribute::Ptr& srcAttr,
        const Ufe::Attribute::Ptr& dstAttr);
    ~UsdUndoDeleteConnectionCommand() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoDeleteConnectionCommand);

    //! Create a UsdUndoDeleteConnectionCommand from two attributes.
    static Ptr create(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdUfe::UsdUndoableItem             _undoableItem;
    std::unique_ptr<Ufe::AttributeInfo> _srcInfo;
    std::unique_ptr<Ufe::AttributeInfo> _dstInfo;
}; // UsdUndoDeleteConnectionCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
