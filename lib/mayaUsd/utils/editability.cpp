//
// Copyright 2021 Autodesk
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
#include "editability.h"

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  The tokens used in the editability metadata.
 */
TfToken Editability::MetadataToken("mayaEditability");
TfToken Editability::OnToken("on");
TfToken Editability::OffToken("off");

/*! \brief  Retrieve the editability of a property.
 */
bool Editability::isEditable(UsdProperty property)
{
    // The reason we treat invalid property as editable is because we don't want
    // to influence editability of things that are not property that are being
    // tested by accident.
    if (!property.IsValid())
        return true;

    TfToken editability;
    if (!property.GetMetadata(MetadataToken, &editability))
        return true;

    if (editability == OffToken) {
        return false;
    } else if (editability == OnToken) {
        return true;
    } else {
        TF_WARN(
            "Invalid token value for maya editability will be treated as on: %s",
            editability.data());
        return true;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE