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
#ifndef USDSYNTAXHIGHLIGHTER_H
#define USDSYNTAXHIGHLIGHTER_H

#include <QtCore/QRegularExpression>
#include <QtGui/QFont>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>

class QTextDocument;

namespace UsdLayerEditor {

/**
 * @brief Syntax highlighter for USD (Universal Scene Description) files.
 *
 * This class provides syntax highlighting for USD layers based on
 * the USD specification. It highlights keywords, data types, primitives, comments,
 * strings, and numbers using appropriate colors.
 */
class UsdSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent The parent QTextDocument to apply highlighting to
     */
    explicit UsdSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    // Structure to hold highlighting rules
    struct HighlightingRule
    {
        QRegularExpression _pattern;
        QTextCharFormat    _format;
    };

    // Initialize highlighting rules and formats
    void setupHighlightingRules();

    /**
     * @brief Load syntax highlighting configuration from JSON file
     * @param configPath Path to the JSON configuration file
     * @return true if configuration was loaded successfully, false otherwise
     */
    bool loadConfigFromJson(const QString& configPath = QString());

    /**
     * @brief Create text format with specified color
     * @param color The color in hex format (e.g., "#93C763")
     * @param fontWeight The font weight (default is QFont::Normal)
     * @return QTextCharFormat with the specified color
     */
    QTextCharFormat createFormat(const QString& color, int fontWeight = QFont::Normal);

    /**
     * @brief Get font weight from string value
     * @param fontWeight String representation of font weight ("normal", "bold")
     * @return QFont weight value
     */
    int getFontWeight(const QString& fontWeight) const;

    void addRule(const QString& pattern, const QTextCharFormat& format);

    QVector<HighlightingRule> _highlightingRules;
};

} // namespace UsdLayerEditor

#endif // USDSYNTAXHIGHLIGHTER_H
