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

#ifndef MAYAUSD_UNDO_OPUNDOINFOVALIDATOR_H
#define MAYAUSD_UNDO_OPUNDOINFOVALIDATOR_H

#include <mayaUsd/base/api.h>

namespace MAYAUSD_NS_DEF {

class OpUndoItem;

//! \brief Validate that the global undo item list is in the correct state.
//
// Only turne don in debug build to help catch incorrect usage of OpUndoItem.
class MAYAUSD_CORE_PUBLIC OpUndoItemValidator
{
public:
    //! \brief Constructor validates the global item list.
    OpUndoItemValidator()
    {
        OpUndoItemValidator::increaseValidatorCount();
#ifdef WANT_VALIDATE_UNDO_ITEM
        OpUndoItemValidator::validateEmpty();
#endif
    }

    //! \brief Destructor.
    ~OpUndoItemValidator() { OpUndoItemValidator::decreaseValidatorCount(); }

    static bool validateEmpty();
    static bool validateItem(const OpUndoItem& item);

protected:
    // Verify if the validator is nested in another.
    // That is, if the validator count is greater than one.
    static bool isNested();

private:
    static void increaseValidatorCount();
    static void decreaseValidatorCount();
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_OPUNDOINFOVALIDATOR_H
