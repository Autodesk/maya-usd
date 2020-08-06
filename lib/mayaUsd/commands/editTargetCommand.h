//
// Copyright 2020 Autodesk
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

#include <mayaUsd/base/api.h>
#include <mayaUsd/mayaUsd.h>

#include <maya/MPxCommand.h>
#include <maya/MString.h>

#include <memory>
#include <string>

namespace MAYAUSD_NS {

namespace Impl {
class SetEditTarget;
}

class MAYAUSD_CORE_PUBLIC EditTargetCommand : public MPxCommand {
public:
    // plugin registration requirements
    static const char commandName[];
    static void*      creator();
    static MSyntax    createSyntax();

    // MPxCommand callbacks
    MStatus doIt(const MArgList& argList) override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    bool    isUndoable() const override;

private:
    MStatus parseArgs(const MArgList& argList);

    enum class Mode { kCreate, kEdit, kQuery } _cmdMode = Mode::kCreate;
    bool isEdit() const { return _cmdMode == Mode::kEdit; }
    bool isQuery() const { return _cmdMode == Mode::kQuery; }

    MString                              _proxyShapePath;
    std::unique_ptr<Impl::SetEditTarget> _setEditTarget;
};

} // namespace MAYAUSD_NS
