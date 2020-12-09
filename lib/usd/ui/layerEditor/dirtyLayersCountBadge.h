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

#ifndef DIRTYLAYERSCOUNTBADGE_H
#define DIRTYLAYERSCOUNTBADGE_H

#include <QtWidgets/QtWidgets>

namespace UsdLayerEditor {
/**
 * @brief Widget that appears on top of the Save Layer button, to show how many layers need to be
 *saved
 *
 **/

class DirtyLayersCountBadge : public QWidget
{
    Q_OBJECT
public:
    DirtyLayersCountBadge(QWidget* in_parent);
    // API for parent widget
    void updateCount(int newCount);

    // QWidgets overrides
    virtual void paintEvent(QPaintEvent* event) override;

protected:
    int _dirtyCount = 0;
};

} // namespace UsdLayerEditor

#endif // DIRTYLAYERSCOUNTBADGE_H
