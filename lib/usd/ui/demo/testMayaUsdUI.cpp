//
// Copyright 2019 Autodesk
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtCore/QProcessEnvironment>

#include <mayaUsd/ui/views/USDImportDialog.h>
#include <mayaUsd/fileio/importData.h>

#include <pxr/usd/usd/stagePopulationMask.h>

#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (QProcessEnvironment::systemEnvironment().contains("MAYA_LOCATION"))
    {
        QString mayaLoc = QProcessEnvironment::systemEnvironment().value("MAYA_LOCATION");
        QCoreApplication::addLibraryPath(mayaLoc + "\\plugins");
    }

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Create the Qt Application
    QApplication app(argc, argv);
    app.setStyle("adskdarkflatui");

    // Create and show the ImportUI
    std::string usdFile = argv[1];
    MAYAUSD_NS::USDImportDialog usdImportDialog(usdFile);

    // Give the dialog the Maya dark style.
    QStyle* adsk = app.style();
    usdImportDialog.setStyle(adsk);
    QPalette newPal(adsk->standardPalette());
    newPal.setBrush(QPalette::Active, QPalette::AlternateBase, newPal.color(QPalette::Active, QPalette::Base).lighter(130));
    usdImportDialog.setPalette(newPal);
    usdImportDialog.show();

    auto ret = app.exec();
    UsdStagePopulationMask popMask = usdImportDialog.stagePopulationMask();
    MayaUsd::ImportData::PrimVariantSelections varSel = usdImportDialog.primVariantSelections();

    return ret;
}