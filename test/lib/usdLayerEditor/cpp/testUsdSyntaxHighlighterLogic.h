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
#pragma once

#include "usdSyntaxHighlighter.h"

#include <ghc/fs_std.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtGui/QTextDocument>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace UsdLayerEditor {

TEST(UsdSyntaxHighlighterTest, HighlightBlock_DoesNotCrashOnUsdContent)
{
    QTextDocument        doc;
    UsdSyntaxHighlighter hl(&doc);
    EXPECT_NO_THROW(doc.setPlainText(
        "#usda 1.0\n"
        "def Sphere \"MySphere\" {\n"
        "    double radius = 1.0\n"
        "    # comment\n"
        "    string myAttr = \"hello\"\n"
        "}\n"));
}

TEST(UsdSyntaxHighlighterTest, HighlightBlock_EmptyTextDoesNotCrash)
{
    QTextDocument        doc;
    UsdSyntaxHighlighter hl(&doc);
    EXPECT_NO_THROW(doc.setPlainText(""));
}

TEST(UsdSyntaxHighlighterTest, HighlightBlock_NumericLiteralHandled)
{
    QTextDocument        doc;
    UsdSyntaxHighlighter hl(&doc);
    EXPECT_NO_THROW(doc.setPlainText("float3 pos = (1.0, 2.0, 3.0)\n"));
}

TEST(UsdSyntaxHighlighterTest, HighlightBlock_KeywordsHandled)
{
    QTextDocument        doc;
    UsdSyntaxHighlighter hl(&doc);
    EXPECT_NO_THROW(doc.setPlainText(
        "over \"Prim\" (\n"
        "    prepend references = @layer.usda@\n"
        "    variants = {string modelingVariant = \"Sphere\"}\n"
        ")\n"));
}

// Tests the MAYAUSD_USD_SYNTAX_HIGHLIGHTING_CONFIG env var path.
// loadConfigFromJson() checks this env var first; setting it before construction
// makes the highlighter load rules from the provided file.
TEST(UsdSyntaxHighlighterTest, LoadConfigFromJson_CustomEnvVarPath)
{
    namespace fss = fs::filesystem;
    const fss::path configFile = fss::temp_directory_path() / "le_test_syntax_config.json";

    struct EnvGuard {
        QByteArray name;
        QByteArray original;
        explicit EnvGuard(const char* n) : name(n), original(qgetenv(n)) {}
        ~EnvGuard() { qputenv(name.constData(), original); }
    } envGuard("MAYAUSD_USD_SYNTAX_HIGHLIGHTING_CONFIG");

    // Minimal valid config: one specifier category with a word pattern.
    const char* jsonContent = R"({
  "syntaxHighlighting": {
    "specifiers": {
      "color": "#E98FE6",
      "fontWeight": "normal",
      "wordPatterns": ["def", "over"]
    },
    "numbers": {
      "color": "#C3DCB5",
      "fontWeight": "normal",
      "patterns": ["\\b\\d+(\\.\\d+)?\\b"]
    }
  }
})";

    {
        QFile f(QString::fromStdString(configFile.string()));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(jsonContent));
    }

    qputenv("MAYAUSD_USD_SYNTAX_HIGHLIGHTING_CONFIG", configFile.string().c_str());

    {
        QTextDocument        doc;
        UsdSyntaxHighlighter hl(&doc);
        // Exercise highlightBlock with content that matches the loaded rules.
        EXPECT_NO_THROW(doc.setPlainText(
            "def Sphere \"S\" {\n"
            "    double r = 1.0\n"
            "}\n"));
    }

    fss::remove(configFile);
}

} // namespace UsdLayerEditor
