//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "maya/MArgDatabase.h"
#include "maya/MFnDagNode.h"
#include "maya/MGlobal.h"
#include "maya/MSelectionList.h"
#include "maya/MStatus.h"
#include "maya/MStringArray.h"
#include "maya/MSyntax.h"
#include "maya/MDagPath.h"
#include "maya/MArgList.h"

#include <sstream>
#include <algorithm>

#include <AL/usdmaya/cmds/ProxyShapeSelectCommands.h>
#include <AL/usdmaya/SelectabilityDB.h>
#include "AL/usdmaya/utils/Utils.h"

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
} // cmds
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
