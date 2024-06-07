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
#ifndef MATERIALUPDATER_H
#define MATERIALUPDATER_H

#include <mayaUsd/fileio/primUpdater.h>

PXR_NAMESPACE_OPEN_SCOPE

class MaterialUpdater : public UsdMayaPrimUpdater
{
public:
    MAYAUSD_CORE_PUBLIC
    MaterialUpdater(
        const UsdMayaPrimUpdaterContext& context,
        const MFnDependencyNode&         depNodeFn,
        const Ufe::Path&                 path);

    /// As of 16-Sep-2022, prims of type Material cannot be pulled by
    /// themselves, so this method returns false.  Materials can only be edited
    /// as Maya when associated with pulled Dag nodes.  This may change in
    /// future versions of maya-usd.
    MAYAUSD_CORE_PUBLIC
    bool canEditAsMaya() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MATERIALUPDATER_H
