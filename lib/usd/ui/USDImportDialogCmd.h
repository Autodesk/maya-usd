//
// Copyright 2019 Autodesk
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

#ifndef MAYAUSDUI_USD_IMPORT_DIALOG_CMD_H
#define MAYAUSDUI_USD_IMPORT_DIALOG_CMD_H

#include <mayaUsd/mayaUsd.h>

#include <maya/MPxCommand.h>

#include <mayaUsdUI/ui/api.h>

namespace MAYAUSD_NS_DEF {

class MAYAUSD_UI_PUBLIC USDImportDialogCmd : public MPxCommand
{
public:
    USDImportDialogCmd() = default;
    ~USDImportDialogCmd() override = default;

    static MStatus initialize(MFnPlugin&);
    static MStatus finalize(MFnPlugin&);

    static const MString fsName;

    static void*   creator();
    static MSyntax createSyntax();

    MStatus doIt(const MArgList& args) override;

private:
    MStatus applyToProxy(const MString& proxyPath);
};

} // namespace MAYAUSD_NS_DEF

#endif
