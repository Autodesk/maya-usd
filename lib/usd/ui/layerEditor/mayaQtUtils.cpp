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

#include "mayaQtUtils.h"

#include <maya/MQtUtil.h>

namespace UsdLayerEditor {

double MayaQtUtils::dpiScale() { return MQtUtil::dpiScale(1.0f); }

QIcon MayaQtUtils::createIcon(const char* iconName)
{
    QIcon* icon = MQtUtil::createIcon(iconName);
    QIcon  copy;
    if (icon) {
        copy = QIcon(*icon);
    }
    delete icon;
    return copy;
}

QPixmap MayaQtUtils::createPixmap(QString const& pixmapName, int width, int height)
{
    QPixmap* pixmap = MQtUtil::createPixmap(pixmapName.toStdString().c_str());
    if (pixmap != nullptr) {
        QPixmap copy(*pixmap);
        delete pixmap;
        return copy;
    }
    return QPixmap();
}

} // namespace UsdLayerEditor
