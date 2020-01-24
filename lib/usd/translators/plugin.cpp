//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"

#include "api.h"

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>


PXR_NAMESPACE_USING_DIRECTIVE


PXRUSDTRANSLATORS_API
MStatus
initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Pixar", "1.0", "Any");
    return MStatus::kSuccess;
}

PXRUSDTRANSLATORS_API
MStatus
uninitializePlugin(MObject /*obj*/)
{
    return MStatus::kSuccess;
}
