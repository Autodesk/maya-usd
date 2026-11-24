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
#include <usdLayerEditor/lib/layerEditorDCCFunctions.h>
#include <usdLayerEditor/ui/stringResources.h>

#include <usdUfe/base/stringResources.h>
#include <usdUfe/ufe/Utils.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTranslator>
#include <gtest/gtest.h>

#include <map>
#include <set>
#include <string>

#include <scopedLayerEditorDCCFunctions.h>

using namespace UsdLayerEditor;

namespace {

// Number of entries in the list. Counting the list itself, rather than the map built from it, is
// what exposes duplicate keys.
size_t layerEditorListSize()
{
    size_t count = 0;
#define COUNT_LE_STR(key, value) ++count;
    USDLAYEREDITOR_STRING_LIST(COUNT_LE_STR)
#undef COUNT_LE_STR
    return count;
}

// The UsdUfe register hook is a plain function pointer, so what it sees is recorded in a static.
std::map<std::string, std::string> gRegistered;

bool registerAndSucceed(const std::string& key, const std::string& value)
{
    gRegistered[key] = value;
    return true;
}

bool registerButFailAddNewLayer(const std::string& key, const std::string& value)
{
    gRegistered[key] = value;
    return key != "kAddNewLayer";
}

// The numbers N of every ^Ns placeholder in the text.
std::set<int> placeholderNumbers(const std::string& text)
{
    std::set<int> numbers;
    for (size_t i = 0; i + 2 < text.size(); ++i) {
        if (text[i] == '^' && text[i + 1] >= '0' && text[i + 1] <= '9' && text[i + 2] == 's')
            numbers.insert(text[i + 1] - '0');
    }
    return numbers;
}

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
        EXPECT_EQ(key, resource._key);
        EXPECT_FALSE(resource._value.empty()) << "empty value for " << key;
        // A context disagreeing with the QT_TRANSLATE_NOOP literal compiles and runs, but
        // never finds a translation.
        EXPECT_EQ("UsdLayerEditor", resource._module) << "wrong context for " << key;
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

    EXPECT_EQ(StringResources::kAddNewLayer._value, GET_LE_STR(kAddNewLayer));
}

TEST(StringResources, qtTranslatorIsUsedWhenNoDCCHookIsSet)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    StubTranslator translator(
        "UsdLayerEditor", StringResources::kAddNewLayer._value.c_str(), "Ajouter un calque");
    ASSERT_TRUE(QCoreApplication::installTranslator(&translator));

    EXPECT_EQ("Ajouter un calque", GET_LE_STR(kAddNewLayer));

    ASSERT_TRUE(QCoreApplication::removeTranslator(&translator));
    EXPECT_EQ(StringResources::kAddNewLayer._value, GET_LE_STR(kAddNewLayer));
}

TEST(StringResources, dccHookTakesPrecedenceAndReceivesTheKey)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    // A translator is installed to prove the hook wins rather than merely filling a gap.
    StubTranslator translator(
        "UsdLayerEditor", StringResources::kAddNewLayer._value.c_str(), "from Qt");
    ASSERT_TRUE(QCoreApplication::installTranslator(&translator));

    // UsdUfe::TranslateStringFn is a plain function pointer, so the hook cannot be a capturing
    // lambda. Record what it sees through file-scope statics instead.
    static std::string seenKey;
    static std::string seenSourceText;
    seenKey.clear();
    seenSourceText.clear();

    auto dccTranslateString
        = [](const std::string& key, const std::string& sourceText) -> std::string {
        seenKey = key;
        seenSourceText = sourceText;
        return "from DCC";
    };
    UsdUfe::setTranslateStringFn(dccTranslateString);

    EXPECT_EQ("from DCC", GET_LE_STR(kAddNewLayer));
    EXPECT_EQ("kAddNewLayer", seenKey);
    EXPECT_EQ(StringResources::kAddNewLayer._value, seenSourceText);

    ASSERT_TRUE(QCoreApplication::removeTranslator(&translator));
}

TEST(StringResources, placeholdersSurviveTranslation)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    auto dccTranslateString
        = [](const std::string&, const std::string&) -> std::string { return "Recharger ^1s"; };
    UsdUfe::setTranslateStringFn(dccTranslateString);

    const std::string translated = GET_LE_STR(kReloadTitle);
    EXPECT_EQ(
        "Recharger layer.usda", StringResourceBase::format(translated, std::string("layer.usda")));
}

TEST(StringResources, allResourcesMatchesTheList)
{
    // A duplicate key would collapse two list entries into one map entry.
    EXPECT_EQ(layerEditorListSize(), StringResources::allResources().size());
}

TEST(StringResources, keysDoNotCollideWithUsdUfeKeys)
{
    // Both libraries register under the same DCC catalog, so a shared key would make one library
    // show the other's text.
    const auto& usdUfe = UsdUfe::StringResources::allResources();
    for (const auto& entry : StringResources::allResources())
        EXPECT_EQ(0u, usdUfe.count(entry.first)) << "key in both lists: " << entry.first;
}

TEST(StringResources, placeholdersAreNumberedFromOneWithoutGaps)
{
    // A source text using ^2s but not ^1s cannot be filled in by format() or MString::format().
    for (const auto& entry : StringResources::allResources()) {
        const std::set<int> numbers = placeholderNumbers(entry.second._value);
        if (numbers.empty())
            continue;
        EXPECT_EQ(1, *numbers.begin()) << "placeholders do not start at 1 in " << entry.first;
        EXPECT_EQ(static_cast<int>(numbers.size()), *numbers.rbegin())
            << "gap in placeholders of " << entry.first;
    }
}

TEST(StringResources, placeholderStringsHaveTheirPlaceholders)
{
    EXPECT_EQ(std::set<int> { 1 }, placeholderNumbers(StringResources::kReloadTitle._value));
    EXPECT_EQ(
        (std::set<int> { 1, 2 }),
        placeholderNumbers(StringResources::kSaveLayerSaveNestedAnonymLayer._value));
    EXPECT_TRUE(placeholderNumbers(StringResources::kAddNewLayer._value).empty());
}

TEST(StringResources, getAsQStringFallsBackToEnglishWithoutTranslatorOrHook)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    EXPECT_EQ(
        QString::fromStdString(StringResources::kAddNewLayer._value), GET_LE_QSTR(kAddNewLayer));
}

TEST(StringResources, getAsQStringUsesTheQtTranslatorWhenNoDCCHookIsSet)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    StubTranslator translator(
        "UsdLayerEditor", StringResources::kAddNewLayer._value.c_str(), "Ajouter un calque");
    ASSERT_TRUE(QCoreApplication::installTranslator(&translator));

    EXPECT_EQ(QString("Ajouter un calque"), GET_LE_QSTR(kAddNewLayer));

    ASSERT_TRUE(QCoreApplication::removeTranslator(&translator));
}

TEST(StringResources, getAsQStringUsesTheDCCHook)
{
    ScopedLayerEditorDCCFunctions scopedFns;

    auto dccTranslateString
        = [](const std::string&, const std::string&) -> std::string { return "from DCC"; };
    UsdUfe::setTranslateStringFn(dccTranslateString);

    EXPECT_EQ(QString("from DCC"), GET_LE_QSTR(kAddNewLayer));
}

TEST(StringResources, registerStringsRegistersEveryResource)
{
    ScopedLayerEditorDCCFunctions scopedFns;
    gRegistered.clear();
    UsdUfe::setRegisterStringFn(registerAndSucceed);

    EXPECT_TRUE(StringResources::registerStrings());

    const auto& resources = StringResources::allResources();
    ASSERT_EQ(resources.size(), gRegistered.size());
    for (const auto& entry : resources) {
        const auto found = gRegistered.find(entry.first);
        ASSERT_NE(gRegistered.end(), found) << "not registered: " << entry.first;
        EXPECT_EQ(entry.second._value, found->second);
    }
}

TEST(StringResources, registerStringsFailsWithoutARegisterHook)
{
    ScopedLayerEditorDCCFunctions scopedFns;
    UsdUfe::setRegisterStringFn(nullptr);

    EXPECT_FALSE(StringResources::registerStrings());
}

TEST(StringResources, registerStringsKeepsGoingAfterAFailure)
{
    ScopedLayerEditorDCCFunctions scopedFns;
    gRegistered.clear();
    UsdUfe::setRegisterStringFn(registerButFailAddNewLayer);

    EXPECT_FALSE(StringResources::registerStrings());
    EXPECT_EQ(StringResources::allResources().size(), gRegistered.size());
}
