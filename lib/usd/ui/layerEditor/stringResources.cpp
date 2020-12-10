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

#include "stringResources.h"

#include <maya/MQtUtil.h>

#include <QtCore/QString>

#include <vector>

namespace UsdLayerEditor {

namespace StringResources {

std::vector<MStringResourceId>& get_stringResourceIDRegister()
{
    static std::vector<MStringResourceId> s_stringResourceIDRegister {};
    return s_stringResourceIDRegister;
}

MStatus registerAll()
{
    MStatus status { MStatus::MStatusCode::kSuccess };
    for (auto& stringResourceID : get_stringResourceIDRegister()) {
        status = MStringResource::registerString(stringResourceID);
    }
    return status;
}

MStringResourceId create(const char* key, const char* value)
{
    MStringResourceId stringResourceID { "mayaUsdPlugin", key, value };
    get_stringResourceIDRegister().push_back(stringResourceID);
    return stringResourceID;
}

MString getAsMString(const MStringResourceId& stringResourceID)
{
    MStatus status { MStatus::MStatusCode::kSuccess };
    MString string = MStringResource::getString(stringResourceID, status);
    return string;
}

QString getAsQString(const MStringResourceId& stringResourceID)
{
    MString str = getAsMString(stringResourceID);
    return MQtUtil::toQString(str);
}

} // namespace StringResources

} // namespace UsdLayerEditor
