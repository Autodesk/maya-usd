//
// Copyright 2017 Animal Logic
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
#pragma once
#include "AL/maya/utils/Api.h"
#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"

/*
#include "AL/usdmaya/fileio/ImportParams.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MDagModifier.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPxCommand.h>
PXR_NAMESPACE_USING_DIRECTIVE

*/

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command to force a variant switch - just a maya convenience wrapper around USD
/// functionality \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ChangeVariant : public ProxyShapeCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command to activate / deactivate a prim - just a maya convenience wrapper around USD
/// functionality \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ActivatePrim : public ProxyShapeCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
