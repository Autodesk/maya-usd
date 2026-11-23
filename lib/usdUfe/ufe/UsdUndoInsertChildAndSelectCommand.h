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
#ifndef USDUFE_USDUNDOINSERTCHILDANDSELECTCOMMAND_H
#define USDUFE_USDUNDOINSERTCHILDANDSELECTCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief Execute an insert child command, then select the inserted child.
//
// The selection is only changed if the insert child command succeeded.
class USDUFE_PUBLIC UsdUndoInsertChildAndSelectCommand : public Ufe::CompositeUndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoInsertChildAndSelectCommand>;

    UsdUndoInsertChildAndSelectCommand(const Ufe::InsertChildCommand::Ptr& creationCmd);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoInsertChildAndSelectCommand);

    void execute() override;

    UFE_V4(std::string commandString() const override;)
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOINSERTCHILDANDSELECTCOMMAND_H
