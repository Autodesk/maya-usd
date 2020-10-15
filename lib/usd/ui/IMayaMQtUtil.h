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

#ifndef MAYAUSDUI_I_MAYA_MQT_UTIL_H
#define MAYAUSDUI_I_MAYA_MQT_UTIL_H

#include <mayaUsd/base/api.h>

#include <mayaUsdUI/ui/api.h>

#include <string>

class QPixmap;

namespace MAYAUSD_NS_DEF {

/**
 * \class IMayaMQtUtil
 * \brief Class used to handle interfacing with Maya's MQtUtil class.
 */
class MAYAUSD_UI_PUBLIC IMayaMQtUtil
{
public:
    virtual ~IMayaMQtUtil() { }

    //! Get the scaled size for Maya interface scaling.
    virtual int   dpiScale(int size) const = 0;
    virtual float dpiScale(float size) const = 0;

    //! Loads the a pixmap for the given resource name.
    virtual QPixmap* createPixmap(const std::string& imageName) const = 0;
};

} // namespace MAYAUSD_NS_DEF

#endif
