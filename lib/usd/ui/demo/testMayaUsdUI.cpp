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
#include <mayaUsd/fileio/importData.h>

#include <pxr/usd/usd/stagePopulationMask.h>

#include <QtCore/QProcessEnvironment>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <mayaUsdUI/ui/IMayaMQtUtil.h>
#include <mayaUsdUI/ui/USDImportDialog.h>

#include <iostream>
#include <string>

class TestUIQtUtil : public MayaUsd::IMayaMQtUtil
{
public:
    ~TestUIQtUtil() override = default;

    int      dpiScale(int size) const override;
    float    dpiScale(float size) const override;
    QPixmap* createPixmap(const std::string& imageName) const override;
};

int TestUIQtUtil::dpiScale(int size) const { return size; }

float TestUIQtUtil::dpiScale(float size) const { return size; }

QPixmap* TestUIQtUtil::createPixmap(const std::string& imageName) const
{
    QString resName(":/");
    resName += imageName.c_str();
    return new QPixmap(resName);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <filename> <rootPrimPath> <primVarSelection>"
                  << std::endl;
        std::cout << std::endl;
        std::cout
            << "  Ex: " << argv[0]
            << "\"/Kitchen_set/Props_grp/DiningTable_grp/ChairB_2\" "
               "\"/Kitchen_set/Props_grp/North_grp/NorthWall_grp/NailA_1:modelingVariant=NailB\""
            << std::endl;
        exit(EXIT_FAILURE);
    }

    if (QProcessEnvironment::systemEnvironment().contains("MAYA_LOCATION")) {
        QString mayaLoc = QProcessEnvironment::systemEnvironment().value("MAYA_LOCATION");
        QCoreApplication::addLibraryPath(mayaLoc + "\\plugins");
    }

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Create the Qt Application
    QApplication app(argc, argv);
    app.setStyle("adskdarkflatui");

    std::string usdFile = argv[1];

    // Set some import data for testing.
    MayaUsd::ImportData importData(usdFile);
    if (argc >= 3) {
        std::string strRootPrimPath = argv[2];
        importData.setRootPrimPath(strRootPrimPath);
    }

    if (argc >= 4) {
        std::string            strPrimVarSel = argv[3];
        std::string::size_type pos1 = strPrimVarSel.find(':');
        if (pos1 != std::string::npos) {
            std::string            strPrimPath = strPrimVarSel.substr(0, pos1);
            std::string::size_type pos2 = strPrimVarSel.find('=', pos1);
            if (pos2 != std::string::npos) {
                std::string strVarName = strPrimVarSel.substr(pos1 + 1, pos2 - pos1 - 1);
                std::string strVarSel = strPrimVarSel.substr(pos2 + 1);

                MayaUsd::ImportData::PrimVariantSelections primVarSels;
                SdfVariantSelectionMap                     varSel;
                varSel[strVarName] = strVarSel;
                primVarSels[SdfPath(strPrimPath)] = varSel;
                importData.setPrimVariantSelections(primVarSels);
            }
        }
    }

    // Create and show the ImportUI
    TestUIQtUtil             uiQtUtil;
    MayaUsd::USDImportDialog usdImportDialog(usdFile, &importData, uiQtUtil);

    // Give the dialog the Maya dark style.
    QStyle* adsk = app.style();
    usdImportDialog.setStyle(adsk);
    QPalette newPal(adsk->standardPalette());
    newPal.setBrush(
        QPalette::Active,
        QPalette::AlternateBase,
        newPal.color(QPalette::Active, QPalette::Base).lighter(130));
    usdImportDialog.setPalette(newPal);
    usdImportDialog.show();

    auto                                       ret = app.exec();
    std::string                                rootPrimPath = usdImportDialog.rootPrimPath();
    UsdStagePopulationMask                     popMask = usdImportDialog.stagePopulationMask();
    MayaUsd::ImportData::PrimVariantSelections varSel = usdImportDialog.primVariantSelections();

    return ret;
}