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

#ifndef USDUFE_USDLABELEDEDITUNDOABLECOMMAND_H
#define USDUFE_USDLABELEDEDITUNDOABLECOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <ufe/undoableCommand.h>

#include <functional>
#include <string>

namespace USDUFE_NS_DEF {

//! UFE undo command that runs a host-supplied edit lambda and exposes
//! a human-readable undo label through commandString().
class USDUFE_PUBLIC UsdLabeledEditUndoableCommand : public UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    UsdLabeledEditUndoableCommand(std::string label, std::function<void()> edit);

    void executeImplementation() override;

    UFE_V4(std::string commandString() const override { return _label; })

private:
    std::string           _label;
    std::function<void()> _edit;
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDLABELEDEDITUNDOABLECOMMAND_H
