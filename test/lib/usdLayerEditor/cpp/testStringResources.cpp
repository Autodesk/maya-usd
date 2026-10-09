//
// Copyright 2026 Autodesk
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
#include "scopedLayerEditorDCCFunctions.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTranslator>
#include <gtest/gtest.h>

#include <layerEditorDCCFunctions.h>
#include <stringResources.h>
#include <utilString.h>

using namespace UsdLayerEditor;

namespace {

// One canned translation, to prove the Qt fallback really goes through QCoreApplication.
class StubTranslator : public QTranslator
{
public:
    StubTranslator(QByteArray context, QByteArray sourceText, QString translated)
        : _context(std::move(context))
        , _sourceText(std::move(sourceText))
        , _translated(std::move(translated))
    {
    }

    QString translate(
        const char* context,
        const char* sourceText,
        const char* disambiguation = nullptr,
        int         n = -1) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);
        if (_context == context && _sourceText == sourceText)
            return _translated;
        return QString();
    }

    bool isEmpty() const override { return false; }

private:
    QByteArray _context;
    QByteArray _sourceText;
    QString    _translated;
};

} // namespace

TEST(StringResources, allResourcesIsConsistent)
{
    const auto& resources = StringResources::allResources();
    ASSERT_FALSE(resources.empty());

    for (const auto& entry : resources) {
        const auto& key = entry.first;
        const auto& resource = entry.second;
        EXPECT_EQ(key, resource.key);
        EXPECT_FALSE(resource.value.empty()) << "empty value for " << key;
        // A context disagreeing with the QT_TRANSLATE_NOOP literal compiles and runs, but
        // never finds a translation.
        EXPECT_EQ("UsdLayerEditor", resource.module) << "wrong context for " << key;
    }
}

TEST(StringResources, contextMenuAndComponentKeysArePresent)
{
    const auto& resources = StringResources::allResources();
    for (const char* key : { "kMenuRemove",
                             "kMenuSaveAs",
                             "kMenuSaveEdits",
                             "kMenuReload",
                             "kMenuAddSublayer",
                             "kMenuAddParentLayer",
                             "kMenuLoadSublayers",
                             "kMenuMergeWithSublayers",
                             "kMenuMute",
                             "kMenuUnmute",
                             "kMenuLock",
                             "kMenuUnlock",
                             "kMenuLockLayerAndSublayers",
                             "kMenuUnlockLayerAndSublayers",
                             "kMenuPrintToScriptEditor",
                             "kMenuSelectPrimsWithSpec",
                             "kMenuClear",
                             "kComponentName",
                             "kComponentLocation",
                             "kComponentBrowseForFolder",
                             "kComponentFileStructure",
                             "kComponentNothingToPreview" }) {
        EXPECT_EQ(1u, resources.count(key)) << "missing resource " << key;
    }
}

TEST(StringResources, fallsBackToEnglishWithoutTranslatorOrHook)
{
    ScopedLayerEditorDCCFunctions scopedFns;
    setLocalizationFns(LocalizationFns {});

    EXPECT_EQ(
        StringResources::kAddNewLayer.value,
        StringResources::getAsString(StringResources::kAddNewLayer));
}

TEST(StringResources, qtTranslatorIsUsedWhenNoDCCHookIsSet)
{
    ScopedLayerEditorDCCFunctions scopedFns;
    setLocalizationFns(LocalizationFns {});

    StubTranslator translator(
        "UsdLayerEditor", StringResources::kAddNewLayer.value.c_str(), "Ajouter un calque");
    ASSERT_TRUE(QCoreApplication::installTranslator(&translator));

    EXPECT_EQ("Ajouter un calque", StringResources::getAsString(StringResources::kAddNewLayer));

    ASSERT_TRUE(QCoreApplication::removeTranslator(&translator));
    EXPECT_EQ(
        StringResources::kAddNewLayer.value,
        StringResources::getAsString(StringResources::kAddNewLayer));
}

TEST(StringResources, dccHookTakesPrecedenceAndReceivesTheKey)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    // A translator is installed to prove the hook wins rather than merely filling a gap.
    StubTranslator translator(
        "UsdLayerEditor", StringResources::kAddNewLayer.value.c_str(), "from Qt");
    ASSERT_TRUE(QCoreApplication::installTranslator(&translator));

    std::string     seenKey;
    std::string     seenSourceText;
    LocalizationFns localization;
    localization.translate
        = [&](const std::string& key, const std::string& sourceText) -> std::string {
        seenKey = key;
        seenSourceText = sourceText;
        return "from DCC";
    };
    setLocalizationFns(localization);

    EXPECT_EQ("from DCC", StringResources::getAsString(StringResources::kAddNewLayer));
    EXPECT_EQ("kAddNewLayer", seenKey);
    EXPECT_EQ(StringResources::kAddNewLayer.value, seenSourceText);

    ASSERT_TRUE(QCoreApplication::removeTranslator(&translator));
}

TEST(StringResources, placeholdersSurviveTranslation)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    LocalizationFns localization;
    localization.translate
        = [](const std::string&, const std::string&) -> std::string { return "Recharger ^1s"; };
    setLocalizationFns(localization);

    const std::string translated = StringResources::getAsString(StringResources::kReloadTitle);
    EXPECT_EQ("Recharger layer.usda", String::format(translated, std::string("layer.usda")));
}
