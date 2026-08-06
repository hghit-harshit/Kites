#pragma once
#include <QPalette>
#include <QWidget>
#include <QString>
#include <QColor>
#include <QVector>
#include <QJsonObject>
#include <array>
#include <QTextCharFormat>
#include "utils/to_index.h"
namespace Kites
{

enum class ThemeAppearance
{
    Light,
    Dark
};

#define ENUM_FROM_STRING_LIST \
X(R32Instruction, "R32Instruction")\
X(R64Instruction, "R64Instruction")\
X(MInstruction, "MInstruction")\
X(FInstruction, "FInstruction")\
X(DInstruction, "DInstruction")\
X(PseudoInstruction, "PseudoInstruction")\

enum class SyntaxStyle
{
    #define X(enumValue, stringValue) enumValue,
    ENUM_FROM_STRING_LIST
    //wow this magic!!
    #undef X
    Register,
    Comment,
    Label,
    NumberLiteral,
    SyntaxStyleCount
};


struct ThemeData
{
    QString id;
    QString name;
    ThemeAppearance appearance;

    QPalette palette;

    QColor iconColor;
    QColor textColor;
    QColor currentLineColor;
    QColor surfaceColor;
    QColor mutedForegroundColor;

    QColor cacheHitColor;
    QColor cacheMissColor;

    QString stylesheet; // fully rendered QSS (template + this theme's colors)

    std::array<QTextCharFormat, toIndex(SyntaxStyle::SyntaxStyleCount)> syntaxFormats;
};

/**
 * @brief ThemeManager is a singleton class that manages the application's theme
 * catalogue (built-in + user-installed JSON themes) and the active theme/palette.
 */
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    ~ThemeManager() = default;

    static ThemeManager &getInstance();

    QString currentThemeId() const;
    void setTheme(const QString &id);
    QVector<ThemeData> availableThemes() const;

    // Editor theme: syntax colors + editor surface can differ from the global
    // UI theme (like Zed's separate UI/syntax theming). Empty id = follow global.
    QString currentEditorThemeId() const; // resolved id (never empty)
    QString configuredEditorThemeId() const; // raw setting ("" = follow global)
    void setEditorTheme(const QString &id);
    QColor getEditorBackgroundColor() const;
    QColor getEditorForegroundColor() const;
    QColor getEditorMutedForegroundColor() const;
    QColor getEditorErrorColor() const; // for squiggly diagnostics; reuses the theme's cacheMiss red

    // Validates, copies into the user theme directory and installs the theme.
    // Returns true on success; on failure `errorOut` is populated.
    bool installTheme(const QString &sourceFilePath, QString &errorOut);

    QColor getIconColor()const;
    QColor getTextColor()const;
    QColor getCurrentLineColor()const;
    QTextCharFormat getSyntaxFormat(SyntaxStyle style)const;
signals:
    void themeChangedSignal(const QString &themeId);
    // emitted whenever the effective editor theme changes (either directly or
    // because the global theme changed while the editor follows it)
    void editorThemeChangedSignal(const QString &themeId);

private:
    ThemeManager();

    QString userThemesDirectory() const;
    void loadBuiltinThemes();
    void loadUserThemes();
    bool parseThemeFile(const QString &path, ThemeData &themeOut, QString &errorOut) const;
    void applyTheme(const ThemeData &theme);
    QString renderStylesheet(const QJsonObject &colors) const;
    int indexOfTheme(const QString &id) const;

    const ThemeData *editorTheme() const;

    QString m_currentThemeId;
    QString m_editorThemeId; // empty = follow global
    QVector<ThemeData> m_themes;
    QString m_qssTemplate;

};
}//namespace Kites
