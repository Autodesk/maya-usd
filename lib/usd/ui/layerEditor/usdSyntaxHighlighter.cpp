//
// Copyright 2025 Autodesk
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
#include "usdSyntaxHighlighter.h"

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/schemaBase.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtGui/QColor>
#include <QtGui/QTextDocument>

namespace {

// Helper function to dynamically read all the concrete prim types.
PXR_NS::TfTokenVector getConcretePrimTypes()
{
    PXR_NS::TfTokenVector primTypes;

    // Query all the available types
    PXR_NS::PlugRegistry&    plugReg = PXR_NS::PlugRegistry::GetInstance();
    std::set<PXR_NS::TfType> schemaTypes;
    plugReg.GetAllDerivedTypes<PXR_NS::UsdSchemaBase>(&schemaTypes);

    PXR_NS::UsdSchemaRegistry& schemaReg = PXR_NS::UsdSchemaRegistry::GetInstance();
    for (auto t : schemaTypes) {
        if (!schemaReg.IsConcrete(t)) {
            continue;
        }

        auto plugin = plugReg.GetPluginForType(t);
        if (!plugin) {
            continue;
        }

        auto type_name = PXR_NS::UsdSchemaRegistry::GetConcreteSchemaTypeName(t);
        primTypes.push_back(type_name);
    }
    return primTypes;
}

} // namespace

namespace UsdLayerEditor {

UsdSyntaxHighlighter::UsdSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    setupHighlightingRules();
}

void UsdSyntaxHighlighter::setupHighlightingRules()
{
    // Clear existing rules
    _highlightingRules.clear();

    loadConfigFromJson();
}

QTextCharFormat UsdSyntaxHighlighter::createFormat(const QString& color, int fontWeight)
{
    QTextCharFormat format;
    format.setForeground(QColor(color));
    format.setFontWeight(fontWeight);
    return format;
}

void UsdSyntaxHighlighter::addRule(const QString& pattern, const QTextCharFormat& format)
{
    HighlightingRule rule;
    rule._pattern = QRegularExpression(pattern);
    rule._format = format;
    _highlightingRules.append(rule);
}

bool UsdSyntaxHighlighter::loadConfigFromJson(const QString& configPath)
{
    PXR_NAMESPACE_USING_DIRECTIVE

    QString jsonPath = configPath;
    if (jsonPath.isEmpty()) {
        // First check if the user is providing a custom config file.
        const std::string customConfigPath
            = PXR_NS::TfGetenv("MAYAUSD_USD_SYNTAX_HIGHLIGHTING_CONFIG");
        if (!customConfigPath.empty()) {
            jsonPath = QString::fromStdString(customConfigPath);
            if (!QFile::exists(jsonPath)) {
                TF_WARN(
                    "Custom USD syntax highlighting config file does not exist: [%s].",
                    qPrintable(jsonPath));
                jsonPath.clear();
            }
        }
    }

    if (jsonPath.isEmpty()) {
        // Default to the config file installed in MayaUsd library location.
        const std::string libLoc = PXR_NS::TfGetenv("MAYAUSD_LIB_LOCATION");
        if (!libLoc.empty()) {
            jsonPath = QString::fromStdString(libLoc) + "/syntaxHighlight/usdSyntaxConfig.json";
        }
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        TF_WARN("Could not open USD syntax highlighting config file: [%s].", qPrintable(jsonPath));
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        TF_WARN(
            "Error during USD syntax highlighting JSON config parsing: %s",
            qPrintable(parseError.errorString()));
        return false;
    }

    QJsonObject root = doc.object();
    QJsonObject syntaxHighlighting = root.value("syntaxHighlighting").toObject();

    // Load each category from JSON
    auto loadCategory = [this](const QString& categoryName, const QJsonObject& category) {
        QString         color = category.value("color").toString();
        QString         fontWeight = category.value("fontWeight").toString();
        int             weight = getFontWeight(fontWeight);
        QTextCharFormat format = createFormat(color, weight);

        // Handle word patterns array
        if (category.contains("wordPatterns")) {
            QJsonArray patterns = category.value("wordPatterns").toArray();
            for (const auto patternValue : patterns) {
                // Add in the word boundary to the pattern.
                QString pattern = QString("\\b%1\\b").arg(patternValue.toString());
                addRule(pattern, format);
            }
        }
        // Handle patterns array
        else if (category.contains("patterns")) {
            QJsonArray patterns = category.value("patterns").toArray();
            for (const auto patternValue : patterns) {
                QString pattern = patternValue.toString();
                addRule(pattern, format);
            }
        } else {
            TF_WARN(
                "Category '%s' does not contain 'wordPatterns' or 'patterns'.",
                qPrintable(categoryName));
        }
    };

    // Dynamically load concrete prim types
    auto loadPrimTypes = [this](const QJsonObject& category) {
        QString         color = category.value("color").toString();
        QString         fontWeight = category.value("fontWeight").toString();
        int             weight = getFontWeight(fontWeight);
        QTextCharFormat format = createFormat(color, weight);

        PXR_NS::TfTokenVector primTypes = getConcretePrimTypes();
        for (const auto& type : primTypes) {
            QString pattern = QString("\\b%1\\b").arg(QString::fromStdString(type.GetString()));
            addRule(pattern, format);
        }
    };

    // Load all categories
    const QStringList categories
        = { "specifiers",     "storageModifier", "geomTokens",   "keywords", "sdfTypes",
            "primitiveTypes", "operators",       "numbers",      "strings",  "comments",
            "brackets",       "delimiters",      "angleBrackets" };

    for (const QString& categoryName : categories) {
        if (syntaxHighlighting.contains(categoryName)) {
            QJsonObject category = syntaxHighlighting.value(categoryName).toObject();
            loadCategory(categoryName, category);

            // Special case for primitive types.
            // Any extra types defined in the JSON will also be added.
            if (categoryName == "primitiveTypes") {
                loadPrimTypes(category);
            }
        }
    }

    return true;
}

int UsdSyntaxHighlighter::getFontWeight(const QString& fontWeight) const
{
    if (fontWeight.toLower() == "bold") {
        return QFont::Bold;
    }
    return QFont::Normal;
}

void UsdSyntaxHighlighter::highlightBlock(const QString& text)
{
    // Apply all highlighting rules
    for (const HighlightingRule& rule : qAsConst(_highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule._pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule._format);
        }
    }
}

} // namespace UsdLayerEditor
