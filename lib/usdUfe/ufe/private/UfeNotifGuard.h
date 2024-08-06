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
#ifndef UFENOTIFGUARD_H
#define UFENOTIFGUARD_H

#include <usdUfe/base/api.h>

#include <ufe/ufe.h>

UFE_NS_DEF { class Path; }

namespace USDUFE_NS_DEF {

//! \brief Helper class to scope when we are in a path change operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.

// TEMP (UsdUfe) - export this class so it can be used by mayaUsd/ufe
class USDUFE_PUBLIC InPathChange
{
public:
    InPathChange() { inGuard = true; }
    ~InPathChange() { inGuard = false; }

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(InPathChange);

    static bool inPathChange() { return inGuard; }

private:
    static bool inGuard;
};

//! \brief Helper class to scope when we are in a set attribute operation.
//
//         It allows detecting that adding an attribute really was setting the
//         attribute. When a USD prim did not have an opinion about an attribute
//         value, it gets notified by USD as adding a property instead of setting
//         a property, which is very unfortunate. This allows detecting this
//         situation.
//
// This simple guard class can be used within a single scope.

// ***** TEMP ***** export this class so it can be used by mayaUsd/ufe
class USDUFE_PUBLIC InSetAttribute
{
public:
    InSetAttribute() { inGuard += 1; }
    ~InSetAttribute() { inGuard -= 1; }

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(InSetAttribute);

    static bool inSetAttribute() { return inGuard > 0; }

private:
    static int inGuard;
};

//! \brief Helper class to scope when we are in an add or delete operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.

// TEMP (UsdUfe) - export this class so it can be used by mayaUsd/ufe
class USDUFE_PUBLIC InAddOrDeleteOperation
{
public:
    InAddOrDeleteOperation() { inGuard = true; }
    ~InAddOrDeleteOperation() { inGuard = false; }

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(InAddOrDeleteOperation);

    static bool inAddOrDeleteOperation() { return inGuard; }

private:
    static bool inGuard;
};

//! \brief Helper class to scope when we are in a Transform3d change operation.
//
// This simple guard class can be used within a single scope, but does not have
// recursive scope capability.  On guard exit, will send a Transform3d
// notification.

// TEMP (UsdUfe) - export this class so it can be used by mayaUsd/ufe
class USDUFE_PUBLIC InTransform3dChange
{
public:
    InTransform3dChange(const Ufe::Path& path);
    ~InTransform3dChange();

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(InTransform3dChange);

    static bool inTransform3dChange();
};

} // namespace USDUFE_NS_DEF

#endif // UFENOTIFGUARD_H
